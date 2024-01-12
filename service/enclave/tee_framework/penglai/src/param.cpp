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
#include "param.h"
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <sys/mman.h>
using namespace demoparam;
inline int memset_s(void *dest, size_t destSize, int ch, size_t count)
{
    if (dest == nullptr || ch < 0) {
        return EINVAL;
    }
    if (destSize < count) {
        return ERANGE;
    }
    memset(dest, ch, count);
    return 0;
}

static pthread_mutex_t mutex;
static unsigned long current_untrusted_ptr = DEFAULT_UNTRUSTED_PTR;

void demoparam::enclave_param_init(struct enclave_args *enclave_args)
{
    enclave_args->stack_size = DEFAULT_STACK_SIZE;
    enclave_args->untrusted_mem_ptr = DEFAULT_UNTRUSTED_PTR;
    enclave_args->untrusted_mem_size = DEFAULT_UNTRUSTED_SIZE;
}

void demoparam::enclave_param_destroy(struct enclave_args *enclave_args)
{
}

char *demoparam::alloc_untrusted_mem(struct enclave_args *enclave_args,
                                     unsigned long size)
{
    size = (size + FOUR * TWO_TO_TEN) & (~(FOUR * TWO_TO_TEN));

    if (pthread_mutex_lock(&mutex) != 0) {
        fprintf(stderr, "LIB: Param: set_untrusted_mem get lock is failed \n");
        return nullptr;
    }

    unsigned long ptr = (unsigned long)mmap(
        (char *)current_untrusted_ptr, DEFAULT_UNTRUSTED_SIZE + size,
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (ptr == -1) {
        pthread_mutex_unlock(&mutex);
        (void)fprintf(stderr, "LIB: Param: failed to map %lu bytes memory\n",
                      size + DEFAULT_UNTRUSTED_SIZE);
        return nullptr;
    }

    if (ptr != current_untrusted_ptr) {
        pthread_mutex_unlock(&mutex);
        fprintf(stderr, "LIB: Param: failed to map at 0x%lu\n",
                current_untrusted_ptr);
        return nullptr;
    }

    // DEFAULT UNTRUSTED MEM IS RESERVED FOR SYSCALL
    enclave_args->untrusted_mem_ptr =
        current_untrusted_ptr + DEFAULT_UNTRUSTED_SIZE;
    enclave_args->untrusted_mem_size = size;
    current_untrusted_ptr += (size + DEFAULT_UNTRUSTED_SIZE);
    pthread_mutex_unlock(&mutex);
    memset_s((char *)ptr, DEFAULT_UNTRUSTED_SIZE + size, 0,
             DEFAULT_UNTRUSTED_SIZE + size);
    return (char *)(enclave_args->untrusted_mem_ptr);
}
