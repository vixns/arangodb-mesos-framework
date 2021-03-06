cmake_minimum_required (VERSION 3.0)
project (arangodb-mesos-framework)
include(FindPackageHandleStandardArgs)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

if (CMAKE_VERSION VERSION_LESS "3.1")
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
  endif()
else()
  set (CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

add_executable(
  arangodb-mesos-framework
  src/framework.cpp
)

add_library(
  libarangodb-mesos STATIC
	src/ArangoManager.cpp 
	src/ArangoScheduler.cpp 
	src/ArangoState.cpp 
	src/Caretaker.cpp 
	src/CaretakerStandalone.cpp 
	src/CaretakerCluster.cpp 
	src/Global.cpp 
	src/HttpServer.cpp 
	src/arangodb.pb.cc 
	src/utils.cpp 
	3rdParty/pbjson/src/pbjson.cpp
)

if(NOT MESOS_SOURCE_PATH)
  set(MESOS_SOURCE_PATH ${CMAKE_CURRENT_BINARY_DIR}/mesos)
endif()

find_path(MESOS_FOUND_SOURCE_PATH
  NAMES build/3rdparty/libprocess/3rdparty/stout/include/Makefile
  PATHS ${MESOS_SOURCE_PATH}
  NO_CMAKE_SYSTEM_PATH
)

if(NOT MESOS_FOUND_SOURCE_PATH)
  message(FATAL_ERROR "Please provide -DMESOS_SOURCE_PATH or link the mesos source directory to ${CMAKE_BINARY_DIR}/mesos")
endif ()

find_package(apr REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(apr REQUIRED)
find_package(svn REQUIRED)
find_package(SASL2 REQUIRED)
find_package(Libunwind REQUIRED)
find_package(Libmicrohttpd REQUIRED)
find_package(gflags)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/pbjson/src
  ${MESOS_FOUND_SOURCE_PATH}/3rdparty/libprocess/include
  ${MESOS_FOUND_SOURCE_PATH}/3rdparty/libprocess/3rdparty/stout/include
  ${MESOS_FOUND_SOURCE_PATH}/include
  ${MESOS_FOUND_SOURCE_PATH}/src
  ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/libprocess/3rdparty/stout/include
  SYSTEM ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/libprocess/3rdparty/boost-1.53.0
  ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/libprocess/3rdparty/glog-0.3.3/src
  ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/libprocess/include
  ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/zookeeper-3.4.5/src/c/include
  ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/zookeeper-3.4.5/src/c/generated
  ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/libprocess/3rdparty/protobuf-2.5.0/src
  ${MESOS_FOUND_SOURCE_PATH}/build/src
  ${MESOS_FOUND_SOURCE_PATH}/build/include
  ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/libprocess/3rdparty/picojson-1.3.0/
  ${Libmicrohttpd_INCLUDE_DIR}
  ${CURL_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(
  arangodb-mesos-framework
  libarangodb-mesos
)

find_file(MESOS_LIB_LEVELDB
  libleveldb.a
  PATHS ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/leveldb ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/leveldb-1.4
)

set(LIBS
  ${MESOS_FOUND_SOURCE_PATH}/build/src/.libs/libmesos_no_3rdparty.a
  ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/libprocess/.libs/libprocess.a
  ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/libprocess/3rdparty/glog-0.3.3/.libs/libglog.a
  ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/libprocess/3rdparty/protobuf-2.5.0/src/.libs/libprotobuf.a
  ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/zookeeper-3.4.5/src/c/.libs/libzookeeper_mt.a
  ${MESOS_LIB_LEVELDB}
  ${MESOS_FOUND_SOURCE_PATH}/build/3rdparty/libprocess/3rdparty/libev-4.15/.libs/libev.a
  ${CURL_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${apr_LIBRARIES}
  ${svn_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${SASL2_LIBRARIES}
  ${Libunwind_LIBRARIES}
  ${Libmicrohttpd_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
)

if(gflags_LIBRARY)
  set(LIBS ${LIBS} ${gflags_LIBRARIES})
endif()

target_link_libraries(
  libarangodb-mesos
  ${LIBS}
)

add_custom_command(
  OUTPUT ${CMAKE_SOURCE_DIR}/src/arangodb.pb.cc ${CMAKE_SOURCE_DIR}/src/arangodb.pb.h
  COMMAND ${CMAKE_BINARY_DIR}/mesos/build/3rdparty/libprocess/3rdparty/protobuf-2.5.0/src/protoc -I${CMAKE_SOURCE_DIR}/src -I${CMAKE_BINARY_DIR}/mesos/include --cpp_out=${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/arangodb.proto
  DEPENDS ${CMAKE_SOURCE_DIR}/src/arangodb.proto
)
