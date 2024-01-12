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
#ifndef RUN_ENCLAVE_PARAMES_H
#define RUN_ENCLAVE_PARAMES_H
#include "EnclaveEngine.h"
#include "face_recognition_shared_param.h"

void run_enclave_with_args(struct penglaienclave::Enclave *enclave)
{
    uint8_t *untrusted_mem_extent = nullptr;
    face_recognition_shared_param_t face_recognition_shared_param;

    char face_information_list[HUNDRED] = {0};
    for (unsigned long i = 0; i < face_information.size(); i++) {
        face_information_list[i] = face_information[i];
    }
    memset_s(face_recognition_shared_param.face_information,
             sizeof(face_recognition_shared_param.face_information), 0,
             sizeof(face_recognition_shared_param.face_information));
    memcpy_s(&face_recognition_shared_param.face_information,
             sizeof(face_information_list), face_information_list,
             sizeof(face_information_list));
    face_recognition_shared_param.face_information_size =
        face_information.size();

    untrusted_mem_extent = (uint8_t *)malloc(DEFAULT_UNTRUSTED_SIZE);
    memcpy_s(untrusted_mem_extent, sizeof(face_recognition_shared_param),
             &face_recognition_shared_param,
             sizeof(face_recognition_shared_param));

    // trans args by untrusted mem
    enclave->user_param.untrusted_mem_ptr = (unsigned long)untrusted_mem_extent;
    enclave->user_param.untrusted_mem_size = DEFAULT_UNTRUSTED_SIZE;

    EnclaveEngine engine = EnclaveEngine(nullptr, enclave, nullptr, nullptr);
    engine.enclave_run();

    // handle outputs
    face_recognition_shared_param_t *face_recognition_shared_param_after_run =
        (face_recognition_shared_param_t *)untrusted_mem_extent;
    char face_recognition_result_list[HUNDRED];
    memset_s(face_recognition_result_list, sizeof(face_recognition_result_list),
             0, sizeof(face_recognition_result_list));
    memcpy_s(
        &face_recognition_result_list,
        sizeof(
            face_recognition_shared_param_after_run->face_recognition_result),
        face_recognition_shared_param_after_run->face_recognition_result,
        sizeof(
            face_recognition_shared_param_after_run->face_recognition_result));
    std::string face_recognition_result_str;
    for (int i = 0;
         i <
         face_recognition_shared_param_after_run->face_recognition_result_size;
         i++) {
        face_recognition_result_str.push_back(face_recognition_result_list[i]);
    }
    face_recognition_result = face_recognition_result_str;
}

#endif