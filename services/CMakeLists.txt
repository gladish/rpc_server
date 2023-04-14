
add_library(bluetooth_service SHARED "bluetooth_service.proto" bluetooth_service.cc)
target_include_directories(bluetooth_service PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")

protobuf_generate(
    TARGET bluetooth_service        
    IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    IMPORT_DIRS "/usr/include"
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")