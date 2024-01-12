# Migrate Local Enclave Service

After compiling, the executable file `MigrateLocalEnclaveService` will be generated. If you want to run the test, you need to open two different consoles to simulate the server and client. The server needs to enable TEE.
This project is used to deploy the local enclave on the remote device and completes subsequent operations.

- In the server console, run `./MigrateLocalEnclaveService server` to start the server;
- In the client console, run `./MigrateLocalEnclaveService client enclave_file`, where `enclave_file` is the executable file of Enclave. Afterwards, the enclave will be passed to the server, and the server will be responsible for parsing and executing. The client can obtain the metadata of the enclave and control its verification, operation, and destruction processes. These are imperceptible to the person developing the enclave application.

Currently, this project relies on riscv's environment, and the enclave is Penglai Enclave. 
