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
#include "elf.h"
#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
using namespace enclaveelf;
void enclaveelf::elf_args_init(struct elf_args *elf_args, char *filename)
{
    int fs;
    struct stat stat_buf;
    elf_args->fd = open(filename, O_RDONLY);
    if (elf_args->fd < 0) {
        fprintf(stderr, "LIB: the elf file:%s is not existed\n", filename);
        return;
    }

    fs = fstat(elf_args->fd, &stat_buf);
    if (fs != 0) {
        fprintf(stderr, "LIB: fstat is failed \n");
        return;
    }

    elf_args->size = stat_buf.st_size;
    if (elf_args->size <= 0) {
        fprintf(stderr, "LIB: the elf file'size  is zero\n");
        return;
    }

    elf_args->ptr =
        mmap(nullptr, elf_args->size, PROT_READ, MAP_PRIVATE, elf_args->fd, 0);
    if (elf_args->ptr == MAP_FAILED) {
        fprintf(stderr, "LIB: can not mmap enough memory for elf file");
        return;
    }

    elf_args->filename = filename;

    return;
}

void enclaveelf::elf_args_destroy(struct elf_args *elf_args)
{
    close(elf_args->fd);
    munmap(elf_args->ptr, elf_args->size);
}

bool enclaveelf::elf_valid(struct elf_args *elf_args)
{
    return (elf_args->fd >= 0 && elf_args->size > 0 &&
            elf_args->ptr != nullptr);
}
