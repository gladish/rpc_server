#pragma once

#include <google/protobuf/service.h>

namespace rdk {
namespace rpc {

class InProcChannel : public google::protobuf::RpcChannel {
public:
  void CallMethod(
    google::protobuf::MethodDescriptor const* method,
    google::protobuf::RpcController* controller,
    google::protobuf::Message const* request,
    google::protobuf::Message* response,
    google::protobuf::Closure* done) override;

  void RegisterService(google::protobuf::Service* service);

  google::protobuf::Service* FindServiceByName(const std::string &service_name);
  
private:
  std::map< std::string, google::protobuf::Service* > m_services;
};


} }
