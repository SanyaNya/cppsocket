# C/C++ flags for GCC/Clang/MSVC
# Debug: disabled optimization, warnings, sanitizers
# Release: max optimization and LTO, warnings disabled

if(MSVC)
  add_compile_options(
    $<$<CONFIG:Debug>:/Od>
    $<$<CONFIG:Debug>:/Zi>
    $<$<CONFIG:Debug>:/fsanitize=address>
    $<$<CONFIG:Release>:/w>
    $<$<CONFIG:Release>:/O2>
    $<$<CONFIG:Release>:/GL>)

  add_link_options(
    $<$<CONFIG:Release>:/LTCG>)

else()
  add_compile_options(
    $<$<CONFIG:Debug>:-pedantic>
    $<$<CONFIG:Debug>:-Wall>
    $<$<CONFIG:Debug>:-Wextra>
    $<$<CONFIG:Debug>:-Wsign-conversion>
    $<$<CONFIG:Debug>:-Wno-unused-but-set-parameter>
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Debug>:-g3>
    $<$<CONFIG:Debug>:-fsanitize=address>
    $<$<CONFIG:Debug>:-fsanitize=undefined>
    $<$<CONFIG:Release>:-w>
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-flto>)

add_link_options(
    $<$<CONFIG:Debug>:-fsanitize=address>
    $<$<CONFIG:Debug>:-fsanitize=undefined>
    $<$<CONFIG:Release>:-flto>)
endif()
