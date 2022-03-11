
add_library(uv STATIC IMPORTED GLOBAL)
set_target_properties(uv PROPERTIES
  IMPORTED_LOCATION ${linux_lib_dir}/libuv/libuv_a.a
  INTERFACE_INCLUDE_DIRECTORIES ${platform_spec_path}/include/libuv
)

add_library(libz STATIC IMPORTED GLOBAL)
set_target_properties(libz PROPERTIES
  IMPORTED_LOCATION ${linux_lib_dir}/libz/libz.a
  INTERFACE_INCLUDE_DIRECTORIES ${platform_spec_path}/include/libz
)

add_library(v8 STATIC IMPORTED GLOBAL)
set_target_properties(v8 PROPERTIES
  IMPORTED_LOCATION ${linux_lib_dir}/v8/libv8_monolith.a
)

set(se_libs_name v8 uv)