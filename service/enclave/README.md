## Enclave Architecture

### Code Structure

The main current Enclave interfaces are in the . /encalve/interfaces directory, and there can be multiple specific enclave implementations in tee_framework as well. You can also replace the penglai enclave with other enclaves by registering changes to the corresponding interfaces in enclave.h.

```bash
/enclave/
├── README.md
├── interface
│   └── enclave.h
└── tee_framework
    └── penglai
        ├── include
        │   ├── elf.h
        │   ├── enclave_demo
        │   ├── param.h
        │   └── penglai-enclave.h
        └── src
            ├── elf.cpp
            ├── enclave.cpp
            ├── enclave_demo
            ├── param.cpp
            └── penglai-enclave.cpp
```



### Enclave Interface

The interface that currently manages the enclave:

```c++
void enclave_init();
void enclave_finalize();
int enclave_create();
int enclave_run();
int enclave_stop();
int enclave_resume();
int enclave_destroy();
int enclave_attest(uintptr_t nonce);
```

You don't have to implement all the interfaces, but at least you need to support creating, running, and stopping Enclaves.

### How to use other enclave implementations

1. Create a directory in tee_framewore identical to the penglai directory, including header files and implementations. You need to implement the enclave interface that corresponds to the Enclave Interface in directory.

2. Register the interface you implemented in the previous step to . /enclave/tee_framework/new_enclave/src/enclave.cpp at the Register Interface tag in the EnclaveEngine constructor like penglai.

3. Add your header and source paths in demo CMakeLists.

3. The CMake compile command specifies the parameter ENCLAVE when compiling, e.g. *-DENCLAVE=PL* when using the penglai enclave. You can replace PL with a new label of your own.

   ```
   cmake -DTHIRDPARTY=ON -DENCLAVE=PL -DCOMPILE_EXAMPLES=ON ..
   ```

4. Build and rerun the demo
