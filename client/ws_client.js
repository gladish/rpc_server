var WebSocket = require("websocket");

const request_state = {
  PENDING: 'pending'
};

class JsonRpcClient
{
  constructor(uri, protocols) {
    this._uri = uri;
    this._websocket = null;
    this._client = null;
    this._next_request_id = 1;
    this._outstanding_requests = {};
    this._json_rpc_version = "2.0";
    this._sub_protocols = protocols;
  }

  open() {
    let promise = new Promise((resolve, reject) => {
      const self = this;
      self._client = new WebSocket.client(); // self._uri, self._sub_protocols);
      self._client.on("connect", function(con) {
        console.log("connected");
	self._websocket = con;
	self._websocket.on("message", function(e) {
	  self._onIncomingMessage(e);
	 });
        resolve(con);
      });
      self._client.on("connectFailed", function(e) {
        reject(e);
      });
      self._client.connect(self._uri, self._sub_protocols);
    });
    return promise;
  }

  sendRequest(method_name, method_params, user_data) {
    const self = this;
    const request_id = self._next_request_id++;
    const request_message = {
      jsonrpc: self._json_rpc_version,
      id: request_id,
      method: method_name,
      params: method_params
    };

    let async_ctx = {};
    async_ctx.state = request_state.PENDING;
    async_ctx.user_data = user_data;
    async_ctx.resolve = null;
    async_ctx.reject = null;

    let promise = new Promise((resolve, reject) => {
      async_ctx.resolve = resolve;
      async_ctx.reject = reject;
    });

    self._outstanding_requests[request_id] = async_ctx;
    self.send(request_message);

    return promise;
  }

  notify(event_data) {
    send(event_data);
  }

  send(message) {
    const self = this;
    const json_text = JSON.stringify(message);
    console.log(">>> " + json_text);
    self._websocket.send(json_text);
  }

  _onIncomingMessage(e) {
    const self = this;
    const data = e.utf8Data;
    console.log("<<< " + data);
    const json = JSON.parse(data);
    if (json.id in self._outstanding_requests) {
      const async_ctx = self._outstanding_requests[json.id];
      if (async_ctx.state == request_state.PENDING) {
        const e_res = {};
        e_res.response = json;
        e_res.user_data = async_ctx.user_data;
        async_ctx.resolve(e_res);
      }
      delete self._outstanding_requests[json.id];
    }
    else {
      // TODO: we just got a message with an id that isn't in the 
      // outstanding requests map
    }
  }
}

async function main() {
  clnt = new JsonRpcClient("ws://127.0.0.1:10001", "json-rpc");
  await clnt.open();
  await clnt.sendRequest("org.rdk.bluetooth.BluetoothService.StartScan", {});
  //await clnt.sendRequest("org.rdk.Bluetooth.1.register", {"event":"onConnectionRequest", "id": "bt_event"});
  //await clnt.sendRequest("org.rdk.Bluetooth.1.register", {"event":"onDiscoveredDevice", "id": "bt_event"});
  //await clnt.sendRequest("org.rdk.Bluetooth.1.register", {"event":"onPairingRequest", "id": "bt_event"});
  //await clnt.sendRequest("org.rdk.Bluetooth.1.register", {"event":"onRequestFailed", "id": "bt_event"});
  //await clnt.sendRequest("org.rdk.Bluetooth.1.register", {"event":"onStatusChanged", "id": "bt_event"});
  //await clnt.sendRequest("org.rdk.Bluetooth.1.register", {"event":"onDeviceFound", "id": "bt_event"});
  //await clnt.sendRequest("org.rdk.Bluetooth.1.register", {"event":"onDeviceLost", "id": "bt_event"});
}

main()
