cmake_minimum_required(VERSION 3.8)

project(ccfileutils CXX)

set(fileutils_source
    include/FileUtils.h
    src/FileUtils.cpp
)

if(APPLE) 
list(APPEND fileutils_source
    include/FileUtils-apple.h
    src/FileUtils-apple.mm
)
elseif(WINDOWS)
list(APPEND fileutils_source
    include/FileUtils-win32.h
    include/Utils-win32.h
    src/FileUtils-win32.cpp
    src/Utils-win32.cpp
)
elseif(LINUX)
list(APPEND fileutils_source
    include/FileUtils-linux.h
    src/FileUtils-linux.cpp
)
endif()


add_library(ccfileutils ${fileutils_source})

target_include_directories(ccfileutils PUBLIC
    include/
)

target_link_libraries(ccfileutils PUBLIC
    ccbase
)

if(WINDOWS)
cc_win32_definations(ccfileutils)
endif()


if(APPLE)
target_link_libraries(ccfileutils PUBLIC
   "-framework Foundation"
   "-framework SystemConfiguration"
)
endif()