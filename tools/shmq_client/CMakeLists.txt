# this is cmake version I am developing with
CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

PROJECT (SHM_CLIENT)

# need to set the common include paths for boost libraries
# include boost test
# include boost concept
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_SOURCE_DIR}/../../kernel_modules/)
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_SOURCE_DIR}/../../click-2.0.1/include/)

# include actual tcp probe's header files
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_SOURCE_DIR}/src)


ADD_SUBDIRECTORY (src)
