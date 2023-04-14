
add_library(
    rdk_rpc SHARED 
    "rdk_rpc.proto" 
    rpc_channel.cc
    rpc_controller.cc
    rpc_log.cc)
    
target_include_directories(rdk_rpc PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")

protobuf_generate(
    TARGET rdk_rpc        
    IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    IMPORT_DIRS "/usr/include"
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")