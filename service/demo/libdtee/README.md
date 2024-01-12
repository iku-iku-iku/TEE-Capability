# Call Remote Enclave Service

After compiling, the executable file `CallRemoteEnclaveService will be generated. If you want to run the test, you need to open two different consoles to simulate the server and client. The server needs to enable TEE.
This project is used to directly call the security service provided by the remote device.

- In the server console, run `./CallRemoteEnclaveService server` to start the server;
- In the client console, run `./CallRemoteEnclaveService client enclave_file`, where `enclave_file` is the remote executable file of Enclave. After that, the remote enclave will be called directly and the return value after execution will be obtained.

Currently, this project relies on riscv's environment, and the enclave is Penglai Enclave. 
