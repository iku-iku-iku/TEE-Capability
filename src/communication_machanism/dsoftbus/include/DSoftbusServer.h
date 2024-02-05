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

#ifndef FASTRTPS_DSOFTBUSSERVER_H
#define FASTRTPS_DSOFTBUSSERVER_H

#include <iostream>
#include <string>
#include "DSoftbusCommon.h"

class DSoftbusServer {
    void init()
    {
        int ret = CreateSessionServerInterface();
        if (ret) {
            std::cerr << "CreateSessionServer fail, ret = " << ret << std::endl;
        }
    }

    bool publish_service(std::string service_name)
    {
        int ret = PublishServiceInterface();
        if (ret) {
            std::cerr << "PublishService fail, ret = " << ret << std::endl;
        }

        return true;
    }

    // Serve indefinitely.
    void serve()
    {
        std::cout << "Enter a number to stop the server: ";
        int aux;
        std::cin >> aux;

        UnPublishServiceInterface();
        RemoveSessionServerInterface();
    }
};

#endif // FASTRTPS_DSOFTBUSSERVER_H
