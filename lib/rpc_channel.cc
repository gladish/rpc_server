#include "rpc_channel.h"
#include "rdk_rpc.pb.h"

#include <google/protobuf/message.h>

#include <atomic>
#include <iostream>
#include <memory>
#include <sstream>

#include <dlfcn.h>

using ServiceConstructor = google::protobuf::Service* (*)();


google::protobuf::Service* rdk::rpc::InProcChannel::FindServiceByName(const std::string &service_name)
{  
  auto itr = m_services.find(service_name);
  return (itr != std::end(m_services))
    ? itr->second
    : nullptr;      
}

void rdk::rpc::InProcChannel::RegisterService(google::protobuf::Service* service)
{  
  m_services.insert(std::make_pair(service->GetDescriptor()->full_name(), service));  
}

void rdk::rpc::InProcChannel::CallMethod(
    const google::protobuf::MethodDescriptor *method,
    google::protobuf::RpcController *controller,
    const google::protobuf::Message *request,
    google::protobuf::Message *response,
    google::protobuf::Closure *done)
{
  auto service = FindServiceByName(method->service()->full_name());
  if (!service) {
    std::stringstream buff;
    buff << "Service not found: ";
    buff << method->service()->full_name();
    controller->SetFailed(buff.str());
    return;
  }
  else
    service->CallMethod(method, controller, request, response, done);
}