/*
 * message_box.c -- the shipped one button box, data/guis/ok.gui.
 *
 * A Message label, an Ok button and the root accelerators
 * "#Enter#Ok#Esc#Ok". The original shows every refusal a player has to
 * act on through this file, so the save and load dialogs and the
 * loading screen share one of these.
 *
 * The text is what makes it modal, not the art. A message whose dialog
 * will not load still has to be read before the screen underneath
 * takes another press, or a broken install turns a refusal into a
 * button that does nothing.
 */

#include "tak_message_box.h"

#include "tak_gui.h"
#include "tak_gui_render.h"
#include "tak_util.h"

#include <string.h>

static struct {
    GUIDialog   dialog;
    int         has_dialog;
    GUIRuntime *rt;
    char        text[256];
} mb;

int MessageBox_IsOpen(void) { return mb.text[0] != '\0'; }

const char *MessageBox_Text(void) { return mb.text; }

void MessageBox_Close(void) {
    if (mb.rt) { GUIRuntime_Destroy(mb.rt); mb.rt = NULL; }
    if (mb.has_dialog) { GUIDialog_Free(&mb.dialog); mb.has_dialog = 0; }
    mb.text[0] = '\0';
}

int MessageBox_Open(const char *text) {
    MessageBox_Close();
    strncpy(mb.text, text ? text : "", sizeof(mb.text) - 1);
    mb.text[sizeof(mb.text) - 1] = '\0';
    if (!mb.text[0]) return -1;
    if (GUIDialog_Load(&mb.dialog, "data/guis/ok.gui") != 0) return -1;
    mb.has_dialog = 1;
    mb.rt = GUIRuntime_Create(&mb.dialog);
    if (!mb.rt) { GUIDialog_Free(&mb.dialog); mb.has_dialog = 0; return -1; }
    GUIRuntime_SetWidgetText(mb.rt, "Message", mb.text);
    GUIRuntime_SetWidgetText(mb.rt, "HelpText", "");
    return 0;
}

void MessageBox_Render(void) {
    if (mb.rt) GUIRuntime_Render(mb.rt);
}

int MessageBox_Tick(int mx, int my, int mouse_down,
                    int enter_edge, int esc_edge) {
    if (!MessageBox_IsOpen()) return 0;
    char clicked[64];
    int got = mb.rt ? GUIRuntime_Update(mb.rt, mx, my, mouse_down,
                                        clicked, sizeof(clicked))
                    : 0;
    MessageBox_Render();
    if (got || enter_edge || esc_edge) {
        MessageBox_Close();
        return 1;
    }
    return 0;
}
