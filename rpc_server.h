#pragma once

#include <array>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <libwebsockets.h>
#include <cjson/cJSON.h>
#include <rdk_rpc.pb.h>
#include <protos/rpc_server.pb.h>
#include <lib/rpc_channel.h>

#include <stdint.h>

class RpcServer
{
public:
  RpcServer(const rdk::rpc::RpcServerConfiguration &conf);
  ~RpcServer();

  void Run();
  void RegisterLocalHostedService(google::protobuf::Service* service);
  void RegisterLocalHostedService(const std::string& shared_object_name, const std::string& service_name);

public:
  static int lws_event_handler(lws *wsi, lws_callback_reasons reason, void *user, void *in, size_t len);
private:  
  struct Message {  
    Message();
    ~Message();
    lws *wsi;
    rdk::rpc::ProtocolDataUnit pdu;
    cJSON *json_object;
    std::vector<uint8_t> buffer;  
  };

  struct SessionData {
    lws *wsi;    
    std::mutex mutex;
  };

  void EventHandler(lws *wsi, SessionData* session_data, lws_callback_reasons reason, void *in, size_t len);
  
  void OnConnectionEstablished(lws *wsi, SessionData* session_data);
  void OnConnectionClosed(lws *wsi, SessionData* session_data);
  void OnMessageReceived(lws *wsi, SessionData *session_data, void *in, size_t len);
  void OnClientWriteable(lws *wsi, SessionData* session_data);
  void WorkerThreadRun();
  void ProcessJsonRequest(const cJSON *json_object);
  void ProcessProtobufRequest(const rdk::rpc::ProtocolDataUnit &pdu);
  void SendErrorResponse(lws *wsi, int error_code, const char *error_text);
  bool IsRunning() const;
  void OnWaitCancelled();

private:
  std::array<lws_protocols, 3> m_ws_protocols;  
  lws_context* m_ws_ctx;
  rdk::rpc::RpcServerConfiguration m_conf;
  bool m_running;
  mutable std::mutex m_mutex;
  std::condition_variable m_cond;
  rdk::rpc::InProcChannel m_inpro_channel;
  std::vector<std::thread> m_worker_threads;
  std::queue<std::unique_ptr<Message>> m_request_queue;
  std::map<lws *, std::queue<std::unique_ptr<Message>>> m_response_queue;
  std::vector< void *> m_open_shared_libraries;
};