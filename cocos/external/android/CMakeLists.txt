
add_library(uv STATIC IMPORTED GLOBAL)
set_target_properties(uv PROPERTIES
  IMPORTED_LOCATION ${platform_spec_path}/libuv.a
  INTERFACE_INCLUDE_DIRECTORIES ${platform_spec_path}/include/uv
)


add_library(z STATIC IMPORTED GLOBAL)
set_target_properties(z PROPERTIES
  IMPORTED_LOCATION ${platform_spec_path}/libz.a
  INTERFACE_INCLUDE_DIRECTORIES ${platform_spec_path}/include/zlib
)

add_library(android_platform STATIC
  ${CMAKE_ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c
)
target_include_directories(android_platform PUBLIC
  ${CMAKE_ANDROID_NDK}/sources/android/cpufeatures
  ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue
)

## Settings from  ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/Android.mk
# set_property(TARGET android_platform APPEND_STRING PROPERTY LINK_FLAGS "-u ANativeActivity_onCreate")
# target_link_libraries(android_platform PUBLIC
#   android log dl
# )

set(se_libs_name)

if(USE_SE_V8)

    add_library(v8_monolith STATIC IMPORTED GLOBAL)
    set_target_properties(v8_monolith PROPERTIES
      IMPORTED_LOCATION ${platform_spec_path}/v8/libv8_monolith.a
    )

    if(ANDROID_ABI STREQUAL "arm64-v8a" OR ANDROID_ABI STREQUAL "x86_64")
      set_property(TARGET v8_monolith
        APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS V8_COMPRESS_POINTERS
      )
    endif()

    add_library(v8_inspector STATIC IMPORTED GLOBAL)
    set_target_properties(v8_inspector PROPERTIES
      IMPORTED_LOCATION ${platform_spec_path}/v8/libinspector.a
      INTERFACE_INCLUDE_DIRECTORIES ${platform_spec_path}/include/v8
    )
    set(se_libs_name v8_monolith v8_inspector)
endif()

if(USE_SE_V8 AND USE_V8_DEBUGGER)
    list(APPEND CC_EXTERNAL_LIBS
        v8_inspector uv
    )
endif()

