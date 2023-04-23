#include "rpc_server.h"
#include "lib/rpc_log.h"

#include <sstream>
#include <stdexcept>
#include <thread>

#include <dlfcn.h>
#include <errno.h>
#include <google/protobuf/util/json_util.h>

const char* lws_reason_to_string(lws_callback_reasons reason);

using ServiceConstructor = google::protobuf::Service* (*)();

RpcServer::Message::Message()
  : wsi(nullptr)
  , json_object(nullptr)
{
}

RpcServer::Message::~Message()
{
  if (json_object)
    cJSON_Delete(json_object);
}

RpcServer::RpcServer(const rdk::rpc::RpcServerConfiguration& conf)
  : m_conf(conf)
  , m_running(false)
  , m_mutex()
{
  lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);

  m_ws_protocols[0] = { "http", lws_callback_http_dummy, 0, 0, 0, NULL, 0 };
  m_ws_protocols[1] = { "json-rpc", &RpcServer::lws_event_handler, sizeof(SessionData), 128, 0, this, 0 };
  m_ws_protocols[2] = { NULL, NULL, 0, 0, 0, NULL, 0 };

  lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);

  lws_context_creation_info ws_info = {};
  ws_info.port = 10001;
  ws_info.protocols = &m_ws_protocols[0];
  ws_info.options = LWS_SERVER_OPTION_VALIDATE_UTF8 | LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

  m_ws_ctx = lws_create_context(&ws_info);
  if (!m_ws_ctx) {
    throw std::runtime_error("failed to create lws context");
  }
}

RpcServer::~RpcServer()
{
  if (m_ws_ctx)
    lws_context_destroy(m_ws_ctx);

  std::for_each(std::begin(m_open_shared_libraries), std::end(m_open_shared_libraries), [](void* handle) {
    dlclose(handle);
  });
}

void RpcServer::WorkerThreadRun()
{
  while (IsRunning()) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock);

    if (!m_running)
      break;

    std::unique_ptr<Message> req = std::move(m_request_queue.front());
    m_request_queue.pop();
    lock.unlock();
    if (req->json_object) {
      try {
        ProcessJsonRequest(req->json_object);
      } catch (const rdk::rpc::Error& err) {
        SendErrorResponse(req->wsi, err.code(), err.what());
      }
    } else
      ProcessProtobufRequest(req->pdu);
  }
}

void RpcServer::SendErrorResponse(lws* wsi, int error_code, const char* error_text)
{
  std::unique_ptr<Message> res = std::make_unique<Message>();
  res->wsi = wsi;

  res->json_object = cJSON_CreateObject();
  cJSON_AddStringToObject(res->json_object, "jsonrpc", "2.0");
  cJSON* error = cJSON_CreateObject();
  cJSON_AddStringToObject(error, "message", error_text);
  cJSON_AddNumberToObject(error, "code", error_code);
  cJSON_AddItemToObject(res->json_object, "error", error);

  std::unique_lock<std::mutex> lock(m_mutex);
  auto itr = m_response_queue.find(wsi);
  if (itr == std::end(m_response_queue)) {
    lock.unlock();
    RPCLOG_INFO("discarding response. client %p has been disconnected", res->wsi);
  } else {
    itr->second.push(std::move(res));

    lock.unlock();
    // this will cause an LWS_CALLBACK_EVENT_WAIT_CANCELLED event
    lws_cancel_service(m_ws_ctx);
  }
}

void RpcServer::ProcessJsonRequest(const cJSON* json_object)
{
  const cJSON* method = cJSON_GetObjectItem(json_object, "method");
  if (!method)
    rdk::rpc::throw_error(-EINVAL, "missing method field in json text");

  const char* p = strrchr(method->valuestring, '.');
  if (!p)
    rdk::rpc::throw_error(-EINVAL, "invalid method name '%s'", method->valuestring);

  std::string service_name(method->valuestring, p - method->valuestring);
  std::string method_name(p + 1);

  auto service = m_inpro_channel.FindServiceByName(service_name);
  if (!service)
    rdk::rpc::throw_error(-ENOENT, "service not found '%s'", service_name.c_str());

  auto service_descriptor = service->GetDescriptor();
  if (!service_descriptor)
    rdk::rpc::throw_error(-ENOENT, "service descriptor not present '%s'", service_name.c_str());

  auto method_descriptor = service_descriptor->FindMethodByName(method_name);
  if (!method_descriptor)
    rdk::rpc::throw_error(-ENOENT, "method not found'%s'", method_name.c_str());

  std::unique_ptr<google::protobuf::Message> req { service->GetRequestPrototype(method_descriptor).New() };
  std::unique_ptr<google::protobuf::Message> res { service->GetResponsePrototype(method_descriptor).New() };

  const cJSON* params = cJSON_GetObjectItem(json_object, "params");
  google::protobuf::util::JsonStringToMessage(params->valuestring, req.get());

  service->CallMethod(
    method_descriptor,
    nullptr,
    req.get(),
    res.get(),
    nullptr);
}

void RpcServer::ProcessProtobufRequest(const rdk::rpc::ProtocolDataUnit& pdu)
{
}

bool RpcServer::IsRunning() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_running;
}

void RpcServer::Run()
{
  for (int i = 0; i < m_conf.service_size(); ++i) {
    const auto& service = m_conf.service(i);
    RegisterLocalHostedService(service.library_name(), service.service_name());
  }

  m_running = true;

  for (int i = 0; i < m_conf.num_worker_threads(); ++i)
    m_worker_threads.emplace_back(&RpcServer::WorkerThreadRun, this);

  bool interrupted = false;
  while (!interrupted) {
    if (lws_service(m_ws_ctx, 10))
      interrupted = true;
  }
}

int RpcServer::lws_event_handler(lws* wsi, lws_callback_reasons reason, void* user, void* in, size_t len)
{
  const lws_protocols* protocols = lws_get_protocol(wsi);
  if (protocols) {
    RpcServer* server = reinterpret_cast<RpcServer*>(protocols->user);
    if (reason == LWS_CALLBACK_ESTABLISHED)
      new (user) SessionData();
    server->EventHandler(wsi, reinterpret_cast<SessionData*>(user), reason, in, len);
  }
  return 0;
}

void RpcServer::OnConnectionEstablished(lws* wsi, SessionData* session_data)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  auto itr = m_response_queue.find(wsi);
  if (itr == std::end(m_response_queue)) {
    m_response_queue.insert(std::make_pair(wsi, std::queue<std::unique_ptr<Message>>()));
    lock.unlock();
    RPCLOG_INFO("created response queue for:%p", wsi);
  }
}

void RpcServer::OnConnectionClosed(lws* wsi, SessionData* session_data)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  auto itr = m_response_queue.find(wsi);
  if (itr != std::end(m_response_queue)) {
    m_response_queue.erase(itr);
    lock.unlock();
    RPCLOG_INFO("destroyed response queue for:%p", wsi);
  }
}

void RpcServer::OnMessageReceived(lws* wsi, SessionData* session_data, void* in, size_t len)
{
  // TODO: handle remotely runninng services
  std::unique_ptr<Message> req = std::make_unique<Message>();
  req->wsi = wsi;

  if (lws_frame_is_binary(wsi)) {
    req->pdu.ParseFromArray(in, len);
  } else {
    req->json_object = cJSON_ParseWithOpts(reinterpret_cast<const char*>(in), nullptr, false);
    if (!req->json_object)
      rdk::rpc::throw_error(-EINVAL, "invalid json text");
  }

  std::lock_guard<std::mutex> lock(m_mutex);
  m_request_queue.push(std::move(req));
  m_cond.notify_one();
}

void RpcServer::OnClientWriteable(lws* wsi, SessionData* session_data)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  auto itr = m_response_queue.find(wsi);
  if (itr == std::end(m_response_queue)) {
    lock.unlock();
    RPCLOG_INFO("supurious writeable event (no client entry) for:%p", wsi);
    return;
  }

  std::queue<std::unique_ptr<Message>>& response_queue = itr->second;
  if (response_queue.empty()) {
    lock.unlock();
    RPCLOG_INFO("supurious writeable event (empty queue) for:%p", wsi);
    return;
  }

  std::unique_ptr<Message> res = std::move(response_queue.front());
  response_queue.pop();
  if (!response_queue.empty())
    lws_callback_on_writable(wsi);
  lock.unlock();

  // TODO: optimize the output better. too many copies
  if (res->json_object) {
    char* json_text = cJSON_Print(res->json_object);
    char* buff = new char[LWS_PRE + strlen(json_text) + 1];
    strcpy(&buff[LWS_PRE], json_text);
    lws_write(wsi, reinterpret_cast<unsigned char*>(&buff[LWS_PRE]), strlen(json_text), LWS_WRITE_TEXT);
  }
}

void RpcServer::OnWaitCancelled()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  for (const auto& itr : m_response_queue) {
    if (!itr.second.empty()) {
      // this will trigger LWS_CALLBACK_SERVER_WRITEABLE for each client (itr.first)
      lws_callback_on_writable(itr.first);
    }
  }
}

void RpcServer::EventHandler(lws* wsi, SessionData* session_data, lws_callback_reasons reason, void* in, size_t len)
{
  switch (reason) {
  case LWS_CALLBACK_ESTABLISHED:
    OnConnectionEstablished(wsi, session_data);
    break;

  case LWS_CALLBACK_CLOSED:
    OnConnectionClosed(wsi, session_data);
    break;

  case LWS_CALLBACK_RECEIVE:
    OnMessageReceived(wsi, session_data, in, len);
    break;

  case LWS_CALLBACK_SERVER_WRITEABLE:
    OnClientWriteable(wsi, session_data);
    break;

  case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
    OnWaitCancelled();
    break;

  default:
    break;
  }
}

void RpcServer::RegisterLocalHostedService(const std::string& shared_object_name, const std::string& service_name)
{
  // if shared_object_name doesn't begin with "lib" then prepend it
  std::string name = shared_object_name;
  if (name.find("lib") != 0) {
    name = "lib" + shared_object_name;
  }

  void* lib = dlopen(name.c_str(), RTLD_NOW | RTLD_GLOBAL);
  if (!lib) {
    std::stringstream message;
    message << "Failed to load shared object: " << name;
    message << " error: " << dlerror();
    throw std::runtime_error(message.str());
  }

  // replace all '.' with '_' in service_name
  std::string t = service_name;
  std::replace(t.begin(), t.end(), '.', '_');

  std::string constructor_name = t + "_New";
  void* constructor = dlsym(lib, constructor_name.c_str());
  if (!constructor) {
    std::stringstream message;
    message << "Failed to find constructor: " << constructor_name;
    message << " in " << name.c_str();
    throw std::runtime_error(message.str());
  }

  auto service_constructor = reinterpret_cast<ServiceConstructor>(constructor);
  auto service = service_constructor();
  this->RegisterLocalHostedService(service);

  m_open_shared_libraries.push_back(lib);
}

void RpcServer::RegisterLocalHostedService(google::protobuf::Service* service)
{
  RPCLOG_INFO("registering locally hosted service %s", service->GetDescriptor()->full_name().c_str());
  m_inpro_channel.RegisterService(service);
}