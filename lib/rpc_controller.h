#pragma once

#include <google/protobuf/service.h>

#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <string>

namespace rdk {
namespace rpc {


class Controller : public google::protobuf::RpcController {
public:  
  void Reset() override;
  bool Failed() const override;
  std::string ErrorText() const override;
  void StartCancel() override;
  void SetFailed(const std::string& reason) override;  
  bool IsCanceled() const override;
  void NotifyOnCancel(google::protobuf::Closure *callback) override;

private:
  std::string m_error_text;
  bool m_is_canceled { false };
};

} }