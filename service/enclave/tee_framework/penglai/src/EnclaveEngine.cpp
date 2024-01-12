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
#include <EnclaveEngine.h>
#include <cstring>
#include "run_enclave.h"
using namespace demoparam;
using namespace enclaveelf;
using namespace penglaienclave;
EnclaveEngine::EnclaveEngine()
{
}

EnclaveEngine::EnclaveEngine(EnclaveFile *enclave_file,
                             Enclave *p_enclave,
                             elf_args *u_elffile,
                             enclave_args *u_param)
{
    this->p_enclave = p_enclave;
    this->u_elffile = u_elffile;
    this->u_param = u_param;

// Register Interface
#ifdef PENGLAI_ENGINE
    printf("register penglai enclave interface\n");
    _enclave_init = std::bind(&PLenclave_init, std::placeholders::_1);
    _enclave_finalize = std::bind(&PLenclave_finalize, std::placeholders::_1);
    _enclave_create = std::bind(&PLenclave_create, std::placeholders::_1,
                                std::placeholders::_2, std::placeholders::_3);
    _enclave_run = std::bind(&PLenclave_run, std::placeholders::_1);
    _enclave_stop = std::bind(&PLenclave_stop, std::placeholders::_1);
    _enclave_resume = std::bind(&PLenclave_resume, std::placeholders::_1);
    _enclave_attest = std::bind(&PLenclave_attest, std::placeholders::_1,
                                std::placeholders::_2);
    _enclave_destroy = std::bind(&PLenclave_destroy, std::placeholders::_1);
#else
    // Default use penglai enclave
    _enclave_init = std::bind(&PLenclave_init, std::placeholders::_1);
    _enclave_finalize = std::bind(&PLenclave_finalize, std::placeholders::_1);
    _enclave_create = std::bind(&PLenclave_create, std::placeholders::_1,
                                std::placeholders::_2, std::placeholders::_3);
    _enclave_run = std::bind(&PLenclave_run, std::placeholders::_1);
    _enclave_stop = std::bind(&PLenclave_stop, std::placeholders::_1);
    _enclave_resume = std::bind(&PLenclave_resume, std::placeholders::_1);
    _enclave_attest = std::bind(&PLenclave_attest, std::placeholders::_1,
                                std::placeholders::_2);
    _enclave_destroy = std::bind(&PLenclave_destroy, std::placeholders::_1);
#endif

#ifdef OTHER_ENCLAVE
    _enclave_init = std::bind(&, std::placeholders::_1);
    _enclave_finalize = std::bind(&, std::placeholders::_1);
    _enclave_create = std::bind(&, std::placeholders::_1, std::placeholders::_2,
                                std::placeholders::_3);
    _enclave_run = std::bind(&, std::placeholders::_1);
    _enclave_stop = std::bind(&, std::placeholders::_1);
    _enclave_resume = std::bind(&, std::placeholders::_1);
    _enclave_attest =
        std::bind(&, std::placeholders::_1, std::placeholders::_2);
    _enclave_destroy = std::bind(&, std::placeholders::_1);
#endif
}

EnclaveEngine::~EnclaveEngine()
{
}

void EnclaveEngine::enclave_init()
{
    _enclave_init(p_enclave);
}

void EnclaveEngine::enclave_finalize()
{
    _enclave_finalize(p_enclave);
}

int EnclaveEngine::enclave_create()
{
    return _enclave_create(p_enclave, u_elffile, u_param);
}

int EnclaveEngine::enclave_run()
{
    return _enclave_run(p_enclave);
}

int EnclaveEngine::enclave_stop()
{
    return _enclave_stop(p_enclave);
}

int EnclaveEngine::enclave_resume()
{
    return _enclave_resume(p_enclave);
}

int EnclaveEngine::enclave_destroy()
{
    return _enclave_destroy(p_enclave);
}

int EnclaveEngine::enclave_attest(uintptr_t nonce)
{
    return _enclave_attest(p_enclave, nonce);
}

void set_return_enclave(penglaienclave::Enclave *return_enclave, penglaienclave::Enclave *enclave)
{
    return_enclave->global_eid = enclave->global_eid;
    return_enclave->eid = enclave->eid;
    return_enclave->fd = enclave->fd;
    return_enclave->type = REMOTE_ENCLAVE;
    return_enclave->user_param.eid = enclave->user_param.eid;
    return_enclave->user_param.elf_ptr = enclave->user_param.elf_ptr;
    return_enclave->user_param.elf_size = enclave->user_param.elf_size;
    return_enclave->user_param.stack_size = enclave->user_param.stack_size;
    return_enclave->user_param.untrusted_mem_ptr =
        enclave->user_param.untrusted_mem_ptr;
    return_enclave->user_param.untrusted_mem_size =
        enclave->user_param.untrusted_mem_size;
    return_enclave->user_param.ocall_buf_size =
        enclave->user_param.ocall_buf_size;
    return_enclave->user_param.resume_type = enclave->user_param.resume_type;
}

static Enclave enclave_create_remote(EnclaveFile enclave_file)
{
    char *filename = new char[enclave_file.filename_size];
    int index = 0;
    for (; index < enclave_file.filename_size; ++index) {
        filename[index] = enclave_file.content[index];
    }
    FILE *fp = nullptr;
    fp = fopen(filename, "w+");
    for (; index < enclave_file.filename_size + enclave_file.file_size;
         ++index) {
        (void)fputc(enclave_file.content[index], fp);
    }
    (void)fclose(fp);

    Enclave return_enclave;
    struct elf_args *enclaveFile =
        static_cast<elf_args *>(malloc(sizeof(struct elf_args)));
    if (enclaveFile == nullptr) {
        return return_enclave;
    }
    elf_args_init(enclaveFile, filename);
    if (!elf_valid(enclaveFile)) {
        printf("error when initializing enclaveFile\n");
    } else {
        struct Enclave *enclave =
            static_cast<Enclave *>(malloc(sizeof(struct Enclave)));
        if (enclave == nullptr) {
            return return_enclave;
        }
        struct enclave_args *params =
            static_cast<enclave_args *>(malloc(sizeof(struct enclave_args)));
        if (params == nullptr) {
            return return_enclave;
        }
        PLenclave_init(enclave);
        enclave_param_init(params);

        params->untrusted_mem_size = DEFAULT_UNTRUSTED_SIZE;
        params->untrusted_mem_ptr = 0;

        if (PLenclave_create(enclave, enclaveFile, params) < 0) {
            printf("host: failed to create enclave\n");
        } else {
            set_return_enclave(&return_enclave, enclave);
        }
    }
    return return_enclave;
}

static Enclave enclave_attest_remote(Enclave enclave)
{
    Enclave return_enclave;
    return_enclave.global_eid = enclave.global_eid;
    return_enclave.eid = enclave.eid;
    return_enclave.fd = enclave.fd;
    return_enclave.type = LOCAL_ENCLAVE;
    return_enclave.user_param.eid = enclave.user_param.eid;
    return_enclave.user_param.elf_ptr = enclave.user_param.elf_ptr;
    return_enclave.user_param.elf_size = enclave.user_param.elf_size;
    return_enclave.user_param.stack_size = enclave.user_param.stack_size;
    return_enclave.user_param.untrusted_mem_ptr =
        enclave.user_param.untrusted_mem_ptr;
    return_enclave.user_param.untrusted_mem_size =
        enclave.user_param.untrusted_mem_size;
    return_enclave.user_param.ocall_buf_size =
        enclave.user_param.ocall_buf_size;
    return_enclave.user_param.resume_type = enclave.user_param.resume_type;

    PLenclave_attest(&return_enclave, NONCE);

    return_enclave.type = REMOTE_ENCLAVE;
    return return_enclave;
}

static Enclave enclave_run_remote(Enclave enclave)
{
    Enclave return_enclave;
    return_enclave.global_eid = enclave.global_eid;
    return_enclave.eid = enclave.eid;
    return_enclave.fd = enclave.fd;
    return_enclave.type = LOCAL_ENCLAVE;
    return_enclave.user_param.eid = enclave.user_param.eid;
    return_enclave.user_param.elf_ptr = enclave.user_param.elf_ptr;
    return_enclave.user_param.elf_size = enclave.user_param.elf_size;
    return_enclave.user_param.stack_size = enclave.user_param.stack_size;
    return_enclave.user_param.untrusted_mem_ptr =
        enclave.user_param.untrusted_mem_ptr;
    return_enclave.user_param.untrusted_mem_size =
        enclave.user_param.untrusted_mem_size;
    return_enclave.user_param.ocall_buf_size =
        enclave.user_param.ocall_buf_size;
    return_enclave.user_param.resume_type = enclave.user_param.resume_type;

    run_enclave_with_args(&return_enclave);

    return_enclave.type = REMOTE_ENCLAVE;
    return return_enclave;
}

static Enclave enclave_finalize_remote(Enclave enclave)
{
    Enclave return_enclave;
    return_enclave.global_eid = enclave.global_eid;
    return_enclave.eid = enclave.eid;
    return_enclave.fd = enclave.fd;
    return_enclave.type = LOCAL_ENCLAVE;
    return_enclave.user_param.eid = enclave.user_param.eid;
    return_enclave.user_param.elf_ptr = enclave.user_param.elf_ptr;
    return_enclave.user_param.elf_size = enclave.user_param.elf_size;
    return_enclave.user_param.stack_size = enclave.user_param.stack_size;
    return_enclave.user_param.untrusted_mem_ptr =
        enclave.user_param.untrusted_mem_ptr;
    return_enclave.user_param.untrusted_mem_size =
        enclave.user_param.untrusted_mem_size;
    return_enclave.user_param.ocall_buf_size =
        enclave.user_param.ocall_buf_size;
    return_enclave.user_param.resume_type = enclave.user_param.resume_type;

    PLenclave_finalize(&return_enclave);

    return_enclave.type = REMOTE_ENCLAVE;
    return return_enclave;
}

Enclave EnclaveEngine::enclave_operation_remote(EnclaveFile enclave_file)
{
    switch (enclave_file.operation) {
        case CREATE_REMOTE_ENCLAVE:
            return enclave_create_remote(enclave_file);
        case ATTEST_REMOTE_ENCLAVE:
            return enclave_attest_remote(enclave_file.pl_enclave);
        case RUN_REMOTE_ENCLAVE:
            return enclave_run_remote(enclave_file.pl_enclave);
        case FINALIZE_REMOTE_ENCLAVE:
            return enclave_finalize_remote(enclave_file.pl_enclave);
        default:
            return enclave_file.pl_enclave;
    }
}