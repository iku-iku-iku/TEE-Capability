# Communication Machanism

This directory contains different implementation of communication. 

## Current Implementation

We have two implementation as follows:
1. `DDS`: use fast-dds for communication. 
2. `dsoftbus`: use OpenHarmony's distributed softbus framework to communication.

## How to Add Another Implmentation

If you want to add another implementation, you can implement two proxy classes for SoftbusServer and SoftbusClient.

### SoftbusServer
`SoftbusServer` has a template method called `run` for which you must pass a communication proxy class with following interfaces:
1. `void init()`: you can initialize the communication context here.
2. `bool publish_service(const std::string& service_name)`: you can publish the service here. return true if publish successfully.
3. `void serve(const SoftbusServer* softbus_server)`: you can loop here to receive requests from client.

### SoftbusClient
`SoftbusClient` is a class template:

```cpp
template <typename ClientProxy = DDSClient> 
class SoftbusClient;
```

ClientProxy must implements following interfaces:
1. `void init(const std::string& service_name)`: you can initialize the communication context for the service you want to request here.
2. `void call_service(Serialization* ds, int enclave_id)`: the `ds` contains the parameter for the enclave, and the `enclave_id` indicated which enclave you want to invoke. `ds` is a serialization of stuff like `service_name | arg0 | arg1 | ... | argn`.
3. `bool isReady()`: returns whether the communication is ready to use.

## How to Use Your Own Implementation
Take dsoftbus as an example, we have implemented `DSoftbusServer` and `DSoftbusClient`, then we can use like:
```cpp
SoftbusServer server;
server.publish_service("PLenclave_operation_remote", PLenclave_operation_remote);
server.run<DSoftbusServer>();
```

and 

```cpp
SoftbusClient<DSoftbusClient> client;
int result = client.call_service<int>("PLenclave_operation_remote", -1, enclave_name);
```
