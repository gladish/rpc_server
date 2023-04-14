#pragma once

#include <stdexcept>

#define RPCLOG_DEBUG(FORMAT, ...) rdk::rpc::log_printf(rdk::rpc::LogLevel::Debug, FORMAT, ## __VA_ARGS__)
#define RPCLOG_INFO(FORMAT, ...) rdk::rpc::log_printf(rdk::rpc::LogLevel::Info, FORMAT, ## __VA_ARGS__)
#define RPCLOG_WARN(FORMAT, ...) rdk::rpc::log_printf(rdk::rpc::LogLevel::Warn, FORMAT, ## __VA_ARGS__)
#define RPCLOG_ERROR(FORMAT, ...) rdk::rpc::log_printf(rdk::rpc::LogLevel::Error, FORMAT, ## __VA_ARGS__)
#define RPCLOG_FATAL(FORMAT, ...) rdk::rpc::log_printf(rdk::rpc::LogLevel::Fatal, FORMAT, ## __VA_ARGS__)

namespace rdk {
namespace rpc {

enum class LogLevel
{
  None  = 0,
  Debug = 1,
  Info  = 2,
  Warn  = 3,
  Error = 4,
  Fatal = 5
};

void log_printf(rdk::rpc::LogLevel level, const char *format, ...)
  __attribute__((format (printf, 2, 3)));

void throw_error(int code, const char *format, ...)
  __attribute__((format (printf, 2, 3)));

  
class Error : public std::runtime_error {
public:
  Error(const char *what, int code) 
    : std::runtime_error(what)
    , m_code(code) { }
  inline int code() const
    { return m_code; }
private:
  int m_code;
};

} }