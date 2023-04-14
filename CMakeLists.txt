
project(RDK_Service_Manager)
cmake_minimum_required(VERSION 3.5)
find_package(Protobuf REQUIRED)
find_package(Libwebsockets REQUIRED)
find_package (Threads)

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")


add_executable(rpcsrvd
    rpcsrvd.cc
    utils.cc
    rpc_server.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/protos/rpc_server.proto)
add_dependencies(rpcsrvd rdk_rpc)

protobuf_generate(
    TARGET rpcsrvd        
    IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    IMPORT_DIRS "/usr/include"
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")

target_include_directories(rpcsrvd PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(rpcsrvd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rpcsrvd protobuf::libprotobuf ${LIBWEBSOCKETS_LIBRARIES})
target_link_libraries(rpcsrvd cjson)
target_link_libraries(rpcsrvd rdk_rpc)
target_link_libraries(rpcsrvd pthread)


add_subdirectory(lib)
add_subdirectory(services)