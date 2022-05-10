add_library(dbgcore SHARED IMPORTED GLOBAL)

set_target_properties(dbgcore PROPERTIES
  IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/libs/dbgcore.dll
)

add_library(dbghelp SHARED IMPORTED GLOBAL)

set_target_properties(dbghelp PROPERTIES
  IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/libs/dbghelp.dll
)

add_library(dbgeng SHARED IMPORTED GLOBAL)

set_target_properties(dbgeng PROPERTIES
  IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/libs/dbgeng.dll
)


add_library(zlib SHARED IMPORTED GLOBAL)

set_target_properties(zlib PROPERTIES
  IMPORTED_IMPLIB ${CMAKE_CURRENT_LIST_DIR}/libs/zlib.lib
  IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/libs/zlib1.dll
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include/zlib
)

add_library(uv SHARED IMPORTED GLOBAL)

set_target_properties(uv PROPERTIES
  IMPORTED_IMPLIB ${CMAKE_CURRENT_LIST_DIR}/libs/libuv.lib
  IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/libs/libuv.dll
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include/uv
)


add_library(websockets SHARED IMPORTED GLOBAL)

set_target_properties(websockets PROPERTIES
  IMPORTED_IMPLIB ${CMAKE_CURRENT_LIST_DIR}/libs/websockets.lib
  IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/libs/websockets.dll
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include/websockets
)

set(se_libs_name)

if(USE_SE_V8)

    add_library(v8 SHARED IMPORTED GLOBAL)
    set_target_properties(v8 PROPERTIES
      IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/libs/v8.dll
      IMPORTED_IMPLIB ${CMAKE_CURRENT_LIST_DIR}/libs/v8.dll.lib
      INTERFACE_COMPILE_DEFINITIONS V8_COMPRESS_POINTERS
      INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include/v8
    )
    target_include_directories(v8 INTERFACE
      ${CMAKE_CURRENT_LIST_DIR}/include/v8
    )


    add_library(v8_libbase SHARED IMPORTED GLOBAL)
    set_target_properties(v8_libbase PROPERTIES
      IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/libs/v8_libbase.dll
      IMPORTED_IMPLIB ${CMAKE_CURRENT_LIST_DIR}/libs/v8_libbase.dll.lib
    )

    add_library(v8_libplatform SHARED IMPORTED GLOBAL)
    set_target_properties(v8_libplatform PROPERTIES
      IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/libs/v8_libplatform.dll
      IMPORTED_IMPLIB ${CMAKE_CURRENT_LIST_DIR}/libs/v8_libplatform.dll.lib
    )

    target_link_libraries(v8 INTERFACE v8_libbase v8_libplatform)

    set_target_properties(v8 PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include/v8
    )

    set(se_libs_name v8 uv)
elseif(NOT CC_EDITOR)
   message(FATAL_ERROR "Only V8 is supported!")
endif()

