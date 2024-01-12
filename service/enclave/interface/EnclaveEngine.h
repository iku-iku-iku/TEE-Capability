/*
 * Copyright (c) 2023 IPADS, Shanghai Jiao Tong University.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef EnclaveEngine_H
#define EnclaveEngine_H

#include "string.h"
#include "penglai-enclave.h"

#include <functional>

#define SAMPLES 10000
#define ONE 1
#define TWO 2
#define THREE 3
#define FOUR 4
#define FIFTEEN 15
#define SIXTEEN 16
#define HUNDRED 100

constexpr unsigned long int NONCE = 12345;

class EnclaveEngine {
public:
    EnclaveEngine();
    EnclaveEngine(penglaienclave::EnclaveFile *enclave_file,
                   penglaienclave::Enclave *p_enclave,
                   enclaveelf::elf_args *u_elffile,
                   demoparam::enclave_args *u_param);
    ~EnclaveEngine();

    void enclave_init();
    void enclave_finalize();
    int enclave_create();
    int enclave_run();
    int enclave_stop();
    int enclave_resume();
    int enclave_destroy();
    int enclave_attest(uintptr_t nonce);
    static penglaienclave::Enclave enclave_operation_remote(penglaienclave::EnclaveFile enclave_file);

private:
    struct penglaienclave::EnclaveFile *enclave_file;
    struct penglaienclave::Enclave *p_enclave;
    struct enclaveelf::elf_args *u_elffile;
    struct demoparam::enclave_args *u_param;

private:

    typedef std::function<void(penglaienclave::Enclave *)> FunctionPtr_1;
    typedef std::function<int(penglaienclave::Enclave *, enclaveelf::elf_args *, demoparam::enclave_args *)>
        FunctionPtr_2;
    typedef std::function<int(penglaienclave::Enclave *)> FunctionPtr_3;
    typedef std::function<int(penglaienclave::Enclave *, uintptr_t)> FunctionPtr_4;

    FunctionPtr_1 _enclave_init;
    FunctionPtr_1 _enclave_finalize;

    FunctionPtr_2 _enclave_create;
    
    FunctionPtr_3 _enclave_run;
    FunctionPtr_3 _enclave_stop;
    FunctionPtr_3 _enclave_resume;
    FunctionPtr_3 _enclave_destroy;
    FunctionPtr_4 _enclave_attest;
};

#endif