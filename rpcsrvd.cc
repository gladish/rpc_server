#include "rpc_server.h"

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include <fstream>
#include <getopt.h>
#include <sstream>
#include <stdexcept>

void read_server_config_from_file(rdk::rpc::RpcServerConfiguration& conf, const char* config_file);

int main(int argc, char* argv[])
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  const char* config_file = nullptr;

  while (true) {
    static struct option long_options[] = {
      { "config", required_argument, 0, 'c' },
      { 0, 0, 0, 0 }
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "c:", long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 'c':
      config_file = optarg;
      break;
    default:
      break;
    }
  }

  rdk::rpc::RpcServerConfiguration conf;
  read_server_config_from_file(conf, config_file);

  RpcServer ws_server(conf);
  ws_server.Run();

  google::protobuf::ShutdownProtobufLibrary();

  return 0;
}

void read_server_config_from_file(rdk::rpc::RpcServerConfiguration& conf, const char* config_file)
{
  std::ifstream infile(config_file);
  if (!infile.is_open()) {
    std::stringstream message;
    message << "Faile to open config file:" << config_file;
    throw std::runtime_error(message.str());
  }

  google::protobuf::io::IstreamInputStream input(&infile);
  if (!google::protobuf::TextFormat::Parse(&input, &conf))
    throw std::runtime_error("Failed to parse configuration file");
}