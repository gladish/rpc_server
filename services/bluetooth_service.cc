#include "bluetooth_service.pb.h"

using namespace org::rdk::bluetooth;

class BluetoothController : public google::protobuf::RpcController {
};

class BluetoothService : public org::rdk::bluetooth::BluetoothService {  
public:
  virtual void StartScan(BluetoothController* controller, const StartScanRequest* request,
    StartScanResponse* response, google::protobuf::Closure* done)
  {
    done->Run();
  }
 
};

extern "C" {
  google::protobuf::Service* org_rdk_bluetooth_BluetoothService_New()
  {   
    return new ::BluetoothService();
  }
}
