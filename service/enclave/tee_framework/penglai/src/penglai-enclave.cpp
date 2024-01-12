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
#include "penglai-enclave.h"
#include <unistd.h>
using namespace penglaienclave;
using namespace demoparam;
void penglaienclave::PLenclave_init(struct Enclave *pl_enclave)
{
    if (pl_enclave->type == REMOTE_ENCLAVE) {
        printf("REMOTE_ENCLAVE plenclave init done\n");
        return;
    }
    pl_enclave->elffile = nullptr;
    pl_enclave->eid = -1;
    pl_enclave->fd = open(PENGLAI_ENCLAVE_DEV_PATH, O_RDWR);
    if (pl_enclave->fd < 0) {
        fprintf(stderr, "LIB: cannot open enclave dev\n");
    }
    printf("plenclave init done\n");
}

static void set_enclaveFile_Enclave(EnclaveFile *enclaveFile,
                                    struct Enclave *pl_enclave)
{
    enclaveFile->pl_enclave.global_eid = pl_enclave->global_eid;
    enclaveFile->pl_enclave.eid = pl_enclave->eid;
    enclaveFile->pl_enclave.fd = pl_enclave->fd;
    enclaveFile->pl_enclave.type = LOCAL_ENCLAVE;
    enclaveFile->pl_enclave.user_param.eid = pl_enclave->user_param.eid;
    enclaveFile->pl_enclave.user_param.elf_ptr = pl_enclave->user_param.elf_ptr;
    enclaveFile->pl_enclave.user_param.elf_size =
        pl_enclave->user_param.elf_size;
    enclaveFile->pl_enclave.user_param.stack_size =
        pl_enclave->user_param.stack_size;
    enclaveFile->pl_enclave.user_param.untrusted_mem_ptr =
        pl_enclave->user_param.untrusted_mem_ptr;
    enclaveFile->pl_enclave.user_param.untrusted_mem_size =
        pl_enclave->user_param.untrusted_mem_size;
    enclaveFile->pl_enclave.user_param.ocall_buf_size =
        pl_enclave->user_param.ocall_buf_size;
    enclaveFile->pl_enclave.user_param.resume_type =
        pl_enclave->user_param.resume_type;
}

static void set_Enclave_Enclave(Enclave *result,
                                    struct Enclave *pl_enclave)
{
    pl_enclave->global_eid = result->global_eid;
    pl_enclave->eid = result->eid;
    pl_enclave->fd = result->fd;
    pl_enclave->type = REMOTE_ENCLAVE;
    pl_enclave->user_param.eid = result->user_param.eid;
    pl_enclave->user_param.elf_ptr = result->user_param.elf_ptr;
    pl_enclave->user_param.elf_size = result->user_param.elf_size;
    pl_enclave->user_param.stack_size = result->user_param.stack_size;
    pl_enclave->user_param.untrusted_mem_ptr =
        result->user_param.untrusted_mem_ptr;
    pl_enclave->user_param.untrusted_mem_size =
        result->user_param.untrusted_mem_size;
    pl_enclave->user_param.ocall_buf_size = result->user_param.ocall_buf_size;
    pl_enclave->user_param.resume_type = result->user_param.resume_type;
}

void penglaienclave::PLenclave_finalize(struct Enclave *pl_enclave)
{
    if (pl_enclave->type == REMOTE_ENCLAVE) {
        int global_eid = pl_enclave->global_eid;
        EnclaveFile enclaveFile;
        enclaveFile.operation = FINALIZE_REMOTE_ENCLAVE;

        set_enclaveFile_Enclave(&enclaveFile, pl_enclave);

        printf("[@%s] call_service PLenclave_finalize\n", __func__);
        SoftbusClient<> softbus_client;
        ;
        Enclave result = softbus_client.call_service<Enclave>(
            "Enclave_operation_remote", global_eid, enclaveFile);
        printf("[@%s] call_service success\n", __func__);

        set_Enclave_Enclave(&result, pl_enclave);

        return;
    }
    if (pl_enclave->fd >= 0)
        close(pl_enclave->fd);
}

int penglaienclave::PLenclave_create(struct Enclave *pl_enclave,
                                     struct enclaveelf::elf_args *u_elffile,
                                     struct enclave_args *u_param)
{
    if (pl_enclave->type == REMOTE_ENCLAVE) {
        EnclaveFile enclaveFile;
        enclaveFile.operation = CREATE_REMOTE_ENCLAVE;
        int index = 0;
        for (; index < strlen(u_elffile->filename); ++index) {
            enclaveFile.content[index] = u_elffile->filename[index];
        }
        enclaveFile.content[index] = '\0';
        ++index;
        enclaveFile.filename_size = strlen(u_elffile->filename) + 1;

        FILE *fp = nullptr;
        fp = fopen(u_elffile->filename, "r");
        int flag;
        while (!feof(fp)) {
            flag = fgetc(fp);
            enclaveFile.content[index] = flag;
            ++index;
        }
        (void)fclose(fp);
        enclaveFile.file_size = index - enclaveFile.filename_size;

        printf("[@%s] call_service PLenclave_create\n", __func__);
        SoftbusClient<> softbus_client;
        ;
        Enclave result = softbus_client.call_service<Enclave>(
            "Enclave_operation_remote", 0, enclaveFile);
        printf("[@%s] call_service success\n", __func__);

        set_Enclave_Enclave(&result, pl_enclave);

        return 0;
    }

    int ret = 0;
    if (!u_elffile) {
        fprintf(stderr, "LIB: elffile is not existed\n");
        return -1;
    }

    pl_enclave->elffile = u_elffile;

    pl_enclave->user_param.elf_ptr = (unsigned long)u_elffile->ptr;
    pl_enclave->user_param.elf_size = u_elffile->size;
    pl_enclave->user_param.stack_size = u_param->stack_size;
    pl_enclave->user_param.untrusted_mem_ptr = u_param->untrusted_mem_ptr;
    pl_enclave->user_param.untrusted_mem_size = u_param->untrusted_mem_size;
    pl_enclave->user_param.ocall_buf_size = 0;
    pl_enclave->user_param.resume_type = 0;
    if (pl_enclave->user_param.elf_ptr == 0 ||
        pl_enclave->user_param.elf_size <= 0) {
        fprintf(stderr, "LIB: ioctl create enclave: elf_ptr is nullptr\n");
        return -1;
    }

    ret = ioctl(pl_enclave->fd, PENGLAI_ENCLAVE_IOC_CREATE_ENCLAVE,
                &(pl_enclave->user_param));
    if (ret < 0) {
        fprintf(stderr, "LIB: ioctl create enclave is failed\n");
        return -1;
    }

    pl_enclave->eid = pl_enclave->user_param.eid;
    return 0;
}

int penglaienclave::PLenclave_run(struct Enclave *pl_enclave)
{
    if (pl_enclave->type == REMOTE_ENCLAVE) {
        int global_eid = pl_enclave->global_eid;
        EnclaveFile enclaveFile;
        enclaveFile.operation = RUN_REMOTE_ENCLAVE;

        set_enclaveFile_Enclave(&enclaveFile, pl_enclave);

        printf("[@%s] call_service PLenclave_run\n", __func__);
        SoftbusClient<> softbus_client;
        ;
        Enclave result = softbus_client.call_service<Enclave>(
            "Enclave_operation_remote", global_eid, enclaveFile);
        printf("[@%s] call_service success\n", __func__);

        set_Enclave_Enclave(&result, pl_enclave);

        return 0;
    }
    int ret = 0;

    ret = ioctl(pl_enclave->fd, PENGLAI_ENCLAVE_IOC_RUN_ENCLAVE,
                &(pl_enclave->user_param));
    if (ret < 0) {
        fprintf(stderr, "LIB: ioctl run enclave is failed \n");
        return -1;
    }

    return ret;
}

int penglaienclave::PLenclave_stop(struct Enclave *pl_enclave)
{
    if (pl_enclave->type == REMOTE_ENCLAVE) {
        return 0;
    }

    int ret = 0;
    (void)fprintf(stderr, "[@%s] beign\n", __func__);
    ret = ioctl(pl_enclave->fd, PENGLAI_ENCLAVE_IOC_STOP_ENCLAVE,
                &(pl_enclave->user_param));
    if (ret < 0) {
        fprintf(stderr, "LIB: ioctl stop enclave is failed with ret:%d\n", ret);
        return -1;
    }
    return 0;
}

int penglaienclave::PLenclave_attest(struct Enclave *pl_enclave,
                                     uintptr_t nonce)
{
    if (pl_enclave->type == REMOTE_ENCLAVE) {
        int global_eid = pl_enclave->global_eid;
        EnclaveFile enclaveFile;
        enclaveFile.operation = ATTEST_REMOTE_ENCLAVE;

        set_enclaveFile_Enclave(&enclaveFile, pl_enclave);

        SoftbusClient<> softbus_client;
        ;
        Enclave result = softbus_client.call_service<Enclave>(
            "Enclave_operation_remote", global_eid, enclaveFile);

        set_Enclave_Enclave(&result, pl_enclave);

        return 0;
    }
    int ret = 0;
    pl_enclave->attest_param.eid = pl_enclave->eid;
    pl_enclave->attest_param.nonce = nonce;
    ret = ioctl(pl_enclave->fd, PENGLAI_ENCLAVE_IOC_ATTEST_ENCLAVE,
                &(pl_enclave->attest_param));
    if (ret < 0) {
        fprintf(stderr, "LIB: ioctl attest enclave is failed ret %d \n", ret);
        return -1;
    }

    return 0;
}

int penglaienclave::PLenclave_resume(struct Enclave *pl_enclave)
{
    if (pl_enclave->type == REMOTE_ENCLAVE) {
        return 0;
    }
    int ret = 0;
    ret = ioctl(pl_enclave->fd, PENGLAI_ENCLAVE_IOC_RESUME_ENCLAVE,
                &(pl_enclave->user_param));
    if (ret < 0) {
        fprintf(stderr, "LIB: ioctl resume enclave is failed \n");
        return -1;
    }
    return ret;
}

int penglaienclave::PLenclave_destroy(struct Enclave *pl_enclave)
{
    if (pl_enclave->type == REMOTE_ENCLAVE) {
        return 0;
    }
    int ret = 0;
    ret = ioctl(pl_enclave->fd, PENGLAI_ENCLAVE_IOC_DESTROY_ENCLAVE,
                &(pl_enclave->user_param));
    if (ret < 0) {
        fprintf(stderr, "LIB: ioctl destory enclave is failed \n");
        return -1;
    }
    return 0;
}

int penglaienclave::PLenclave_debug_print(struct Enclave *pl_enclave)
{
    if (pl_enclave->type == REMOTE_ENCLAVE) {
        return 0;
    }
    int ret = 0;

    ret = ioctl(pl_enclave->fd, PENGLAI_ENCLAVE_IOC_DEBUG_PRINT,
                &(pl_enclave->user_param));
    if (ret < 0) {
        fprintf(stderr, "LIB: ioctl debug print is failed \n");
        return -1;
    }

    return 0;
}
