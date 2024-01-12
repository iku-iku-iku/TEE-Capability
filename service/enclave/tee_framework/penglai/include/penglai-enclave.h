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
#ifndef PL_ENCLAVE
#define PL_ENCLAVE

#include <fcntl.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "elf.h"
#include "param.h"

#include "DDSServer.h"
#include "Softbus.h"

#define PENGLAI_ENCLAVE_DEV_PATH "/dev/penglai_enclave_dev"

#define RETURN_USER_EXIT_ENCL 0
#define RETURN_USER_FOR_OCALL 1

#define LOCAL_ENCLAVE 0
#define REMOTE_ENCLAVE 1

#define CREATE_REMOTE_ENCLAVE 1
#define ATTEST_REMOTE_ENCLAVE 2
#define RUN_REMOTE_ENCLAVE 3
#define FINALIZE_REMOTE_ENCLAVE 4

namespace penglaienclave {
struct Enclave {
    int global_eid = -1;
    int eid;
    int fd;
    int type; // 0: local creation, 1: remote creation
    struct demoparam::penglai_enclave_user_param user_param;
    struct demoparam::penglai_enclave_attest_param attest_param;
    struct enclaveelf::elf_args *elffile;

    friend Serialization &operator>>(Serialization &in, Enclave &d)
    {
        in >> d.global_eid >> d.eid >> d.fd >> d.type;
        in >> d.user_param.eid >> d.user_param.elf_ptr >>
            d.user_param.elf_size >> d.user_param.stack_size;
        in >> d.user_param.untrusted_mem_ptr >>
            d.user_param.untrusted_mem_size >> d.user_param.ocall_buf_size >>
            d.user_param.resume_type;
        return in;
    }

    friend Serialization &operator<<(Serialization &out, Enclave d)
    {
        out << d.global_eid << d.eid << d.fd << d.type;
        out << d.user_param.eid << d.user_param.elf_ptr << d.user_param.elf_size
            << d.user_param.stack_size;
        out << d.user_param.untrusted_mem_ptr << d.user_param.untrusted_mem_size
            << d.user_param.ocall_buf_size << d.user_param.resume_type;
        return out;
    }
};

struct EnclaveFile {
    int operation;
    int filename_size = 0;
    int file_size = 0;
    int content[30000];
    Enclave pl_enclave;

    friend Serialization &operator>>(Serialization &in, EnclaveFile &d)
    {
        in >> d.operation >> d.filename_size >> d.file_size;
        in >> d.pl_enclave.global_eid >> d.pl_enclave.eid >> d.pl_enclave.fd >>
            d.pl_enclave.type;
        in >> d.pl_enclave.user_param.eid >> d.pl_enclave.user_param.elf_ptr >>
            d.pl_enclave.user_param.elf_size >>
            d.pl_enclave.user_param.stack_size;
        in >> d.pl_enclave.user_param.untrusted_mem_ptr >>
            d.pl_enclave.user_param.untrusted_mem_size >>
            d.pl_enclave.user_param.ocall_buf_size >>
            d.pl_enclave.user_param.resume_type;
        for (int i = 0; i < d.filename_size + d.file_size; ++i) {
            in >> d.content[i];
        }
        return in;
    }

    friend Serialization &operator<<(Serialization &out, EnclaveFile d)
    {
        out << d.operation << d.filename_size << d.file_size;
        out << d.pl_enclave.global_eid << d.pl_enclave.eid << d.pl_enclave.fd
            << d.pl_enclave.type;
        out << d.pl_enclave.user_param.eid << d.pl_enclave.user_param.elf_ptr
            << d.pl_enclave.user_param.elf_size
            << d.pl_enclave.user_param.stack_size;
        out << d.pl_enclave.user_param.untrusted_mem_ptr
            << d.pl_enclave.user_param.untrusted_mem_size
            << d.pl_enclave.user_param.ocall_buf_size
            << d.pl_enclave.user_param.resume_type;
        for (int i = 0; i < d.filename_size + d.file_size; ++i) {
            out << d.content[i];
        }
        return out;
    }
};

void PLenclave_init(struct Enclave *pl_enclave);
void PLenclave_finalize(struct Enclave *pl_enclave);
int PLenclave_create(struct Enclave *pl_enclave,
                     struct enclaveelf::elf_args *u_elffile,
                     demoparam::enclave_args *u_param);
int PLenclave_run(struct Enclave *pl_enclave);
int PLenclave_stop(struct Enclave *pl_enclave);
int PLenclave_resume(struct Enclave *pl_enclave);
int PLenclave_destroy(struct Enclave *pl_enclave);
int PLenclave_attest(struct Enclave *pl_enclave, uintptr_t nonce);
int PLenclave_debug_print(struct Enclave *pl_enclave);
} // namespace penglaienclave
#endif
