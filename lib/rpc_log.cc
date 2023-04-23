#include "rpc_log.h"
#include "rpc_controller.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <sys/time.h>

rdk::rpc::LogLevel curent_log_level = rdk::rpc::LogLevel::Info;

void rdk::rpc::log_printf(rdk::rpc::LogLevel level, const char* format, ...)
{
  const char* level_strings[] = {
    "none",
    "debug",
    "info",
    "warn",
    "error",
    "fatal"
  };

  if (level < curent_log_level)
    return;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  printf("[%5s] %ld.%04ld : ", level_strings[static_cast<int>(level)], tv.tv_sec, (tv.tv_usec / 1000));

  va_list ptr;
  va_start(ptr, format);
  vprintf(format, ptr);
  va_end(ptr);

  printf("\n");

  if (level == rdk::rpc::LogLevel::Fatal) {
    printf("Fatal error, exiting\n");
    fflush(stdout);
    exit(1);
  }
}

void rdk::rpc::throw_error(int code, const char* format, ...)
{
  va_list args;
  va_start(args, format);

  char buff[256];
  vsnprintf(buff, sizeof(buff), format, args);
  va_end(args);

  throw rdk::rpc::Error(buff, code);
}