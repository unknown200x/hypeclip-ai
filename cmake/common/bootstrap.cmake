# Minimal bootstrap shim.
#
# In a real checkout this directory is populated from obsproject/obs-plugintemplate
# (the `cmake/common` and `cmake/<platform>` trees) which define helpers such as
# `set_target_properties_plugin`, dependency download/extract logic and packaging.
#
# We provide a no-op fallback so the tree is self-documenting and so host/CI
# unit-test builds (HYPECLIP_BUILD_TESTS, which never touch libobs) can configure
# without the full template. Replace this file by copying the upstream template's
# cmake/ directory over it before producing release artifacts.

if(NOT COMMAND set_target_properties_plugin)
  function(set_target_properties_plugin target)
    cmake_parse_arguments(PARSE_ARGV 1 _ "" "" "PROPERTIES")
    if(_PROPERTIES)
      set_target_properties(${target} PROPERTIES ${_PROPERTIES})
    endif()
    set_target_properties(${target} PROPERTIES PREFIX "")
  endfunction()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
  add_compile_options(/W3 /permissive- /Zc:__cplusplus /MP)
  # Highlight engine is real-time: prefer fast FP, never break stream.
  add_compile_options($<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/fp:fast>)
endif()
