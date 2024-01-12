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
#ifndef ELF
#define ELF

#include <stdbool.h>
extern "C"
{
    namespace enclaveelf {
    struct elf_args {
        int fd;
        unsigned long size;
        void *ptr;
        char *filename;
    };

    void elf_args_init(struct elf_args *elf_args, char *filename);
    void elf_args_destroy(struct elf_args *elf_args);
    bool elf_valid(struct elf_args *elf_args);
    }
}
#endif
