# this is cmake version I am developing with
CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

PROJECT (TCP_PROBE)

# need to set the common include paths for boost libraries
# include boost test
# include boost concept
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/boost_concept_test_1_49_0/)
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/libev_4.15_gcc443/include/)

LINK_DIRECTORIES (${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/libev_4.15_gcc443/lib/)

# include actual tcp probe's header files
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_SOURCE_DIR}/src)

ADD_SUBDIRECTORY (src)
ADD_SUBDIRECTORY (tests)
