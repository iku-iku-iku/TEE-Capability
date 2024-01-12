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
#include "eapp.h"
#include "face_recognition_shared_param.h"
#include "ocall.h"
#include "print.h"

int EAPP_ENTRY main()
{
    unsigned long *args;
    EAPP_RESERVE_REG;
    eapp_print("%s is running\n", "face recognition");

    face_recognition_shared_param_t face_recognition_shared_param;

    if (sizeof(face_recognition_shared_param_t) > DEFAULT_UNTRUSTED_SIZE) {
        eapp_print("Size to copy is larger than untrusted mem size \n");
        EAPP_RETURN(0);
    }

    // get arguements from untrusted mem to safe memory
    memcpy_s(&face_recognition_shared_param,
             sizeof(face_recognition_shared_param_t),
             (const char *)DEFAULT_UNTRUSTED_PTR,
             sizeof(face_recognition_shared_param_t));

    char face_information_list[100];
    memset_s(face_information_list, sizeof(face_information_list), 0,
             sizeof(face_information_list));
    memcpy_s(&face_information_list,
             sizeof(face_recognition_shared_param.face_information),
             face_recognition_shared_param.face_information,
             sizeof(face_recognition_shared_param.face_information));
    int face_information_size =
        face_recognition_shared_param.face_information_size;

    // enclave handle arguements
    char face_recognition_result_list[100] = {'J', 'a', 'c', 'k'};
    int face_recognition_result_size = 4;
    eapp_print("face name = %s\n", "Jack");
    size_t size = sizeof(face_recognition_shared_param.face_recognition_result);
    memset_s(face_recognition_shared_param.face_recognition_result, size, 0,
             size);
    memcpy_s(&face_recognition_shared_param.face_recognition_result,
             sizeof(face_recognition_result_list), face_recognition_result_list,
             sizeof(face_recognition_result_list));
    face_recognition_shared_param.face_recognition_result_size =
        face_recognition_result_size;

    // trans output
    memcpy_s((char *)DEFAULT_UNTRUSTED_PTR,
             sizeof(face_recognition_shared_param_t),
             &face_recognition_shared_param,
             sizeof(face_recognition_shared_param_t));

    EAPP_RETURN(0);
}