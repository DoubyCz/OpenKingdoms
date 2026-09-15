# Own-trigonometry guard for files that write simulation state.
#
# A file here may use float, unlike the integer-only list in
# float_guard.cmake, but it must not reach for the platform's libm.
# IEEE 754 pins + - * / and sqrt to one correctly rounded answer, so
# those and floorf agree on every platform. It says nothing about a
# sine, a tangent or an arc tangent, and the three platforms measured
# three different answers. Those come from include/tak_trig.h instead.
#
# The check is a grep with no judgement in it: a name in the list below
# followed by an open bracket fails, comments included, because a rule
# a reader has to think about is a rule that lets a desync back in.
#
#   cmake -DFILES=<path>|<path>|... -P cmake/libm_guard.cmake

if(NOT DEFINED FILES)
    message(FATAL_ERROR "libm guard: pass -DFILES=<path>|<path>")
endif()

# Longer names first: the engine backtracks, but spelling it out keeps
# atan2f from being read as atan.
set(guard_fns
    "atan2|atanh|atan|asinh|asin|acosh|acos|sinh|sin|cosh|cos|tanh|tan")
set(guard_fns
    "${guard_fns}|expm1|exp2|exp|log10|log1p|log2|log|pow|cbrt|hypot|fmod|remainder")
# tak_sinf and friends are spared by the underscore before the name.
set(guard_re "(^|[^A-Za-z0-9_])(${guard_fns})(f|l)?\\(")

string(REPLACE "|" ";" guard_files "${FILES}")
set(guard_bad "")

foreach(f IN LISTS guard_files)
    if(NOT EXISTS "${f}")
        message(FATAL_ERROR
            "libm guard: ${f} is missing. Fix the list in src/CMakeLists.txt.")
    endif()
    file(STRINGS "${f}" hits REGEX "${guard_re}")
    if(hits)
        get_filename_component(name "${f}" NAME)
        foreach(line IN LISTS hits)
            message(STATUS "  ${name}: ${line}")
        endforeach()
        list(APPEND guard_bad "${name}")
    endif()
endforeach()

if(guard_bad)
    message(FATAL_ERROR
        "libm guard: these must call tak_sinf and friends, not libm: ${guard_bad}")
endif()

list(LENGTH guard_files guard_count)
message(STATUS "libm guard: ${guard_count} file(s) carry their own trigonometry")
