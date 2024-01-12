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

#ifndef FASTRTPS_SOFTBUSCLIENT_H
#define FASTRTPS_SOFTBUSCLIENT_H

#include <functional>
#include <string>
#include "DSoftbusCommon.h"

class DSoftbusClient {
public:
    bool init(std::string service_name = "")
    {
        int ret = CreateSessionServerInterface();
        if (ret) {
            printf("CreateSessionServer fail, ret=%d\n", ret);
            return ret;
        }

        ret = DiscoveryInterface();
        if (ret) {
            printf("DiscoveryInterface fail, ret=%d\n", ret);
        }

        is_ready_ = true;
    }

    Serialization *call_service(Serialization *param, int enclave_id = -1)
    {
        std::vector<char> test_vector;
        for (int i = 0; i < param->size(); i++) {
            test_vector.push_back(param->data()[i]);
        }

        SendBytesInterface((char *)test_vector.data(), (int)test_vector.size());
        return param;
    }

    bool isReady()
    {
        return is_ready_;
    }

private:
    bool is_ready_ = false;
};

#endif // FASTRTPS_SOFTBUSCLIENT_H
