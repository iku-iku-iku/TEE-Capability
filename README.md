## SoftbusServer
SoftbusServer has a main method called `run` for which you must implement a communication proxy with following interfaces:
1. `void init()`: you can initialize the communication context here.
2. `bool publish_service(const std::string& service_name)`: you can publish the service here. return true if publish successfully.
3. `void serve(const SoftbusServer* softbus_server)`: you can loop here to receive requests from client.

## SoftbusClient
SoftbusClient has a template argument which implements the communication stuff.
```cpp
template <typename ClientProxy = DDSClient> // ClientProxy must implement: init, call, isReady
class SoftbusClient;
```

ClientProxy must implements three interfaces:
1. `void init(const std::string& service_name)`: you can initialize the communication context for the service you want to request here.
2. `void call_service(Serialization* ds, int enclave_id)`: the `ds` contains the parameter for the enclave, and the `enclave_id` indicated which enclave you want to invoke. `ds` is a serialization of stuff like `service_name | arg0 | arg1 | ... | argn`.
3. `bool isReady()`: returns whether the communication is ready to use.