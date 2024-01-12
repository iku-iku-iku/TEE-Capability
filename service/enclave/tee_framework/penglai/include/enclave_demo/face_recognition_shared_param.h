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
#ifndef EVM_SHARED_PARAM_H
#define EVM_SHARED_PARAM_H

#include <stdint.h>
#include <stdlib.h>
#include <string>

static std::string face_information = "test";
static std::string face_recognition_result = "xxx";

typedef struct _face_recognition_shared_param_t {
    char face_information[100];
    int face_information_size;

    char face_recognition_result[100];
    int face_recognition_result_size;
} face_recognition_shared_param_t;

#endif
