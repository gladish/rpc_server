#include "rpc_controller.h"

void rdk::rpc::Controller::Reset()
{
  m_error_text.clear();
}

bool rdk::rpc::Controller::Failed() const 
{
  return !m_error_text.empty();
}

std::string rdk::rpc::Controller::ErrorText() const 
{
  return m_error_text;
}

void rdk::rpc::Controller::StartCancel() 
{

}

void rdk::rpc::Controller::SetFailed(const std::string& reason) 
{
  m_error_text = reason;
}  

bool rdk::rpc::Controller::IsCanceled() const 
{
  return m_is_canceled;
}

void rdk::rpc::Controller::NotifyOnCancel(google::protobuf::Closure *callback) 
{

}