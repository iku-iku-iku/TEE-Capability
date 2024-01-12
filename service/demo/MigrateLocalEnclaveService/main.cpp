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
#include <bitset>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "DDSServer.h"
#include "Softbus.h"

#include <cstdlib>
#include <pthread.h>
#include "EnclaveEngine.h"

using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;
using std::cout;
using std::endl;
using namespace enclaveelf;
using namespace demoparam;
using namespace penglaienclave;

enum class E_SIDE {
    CLIENT,
    SERVER
};

static void printHex(unsigned char *c, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        printf("0x%02X, ", c[i]);
        if ((i % FOUR) == THREE)
            printf(" ");

        if ((i % SIXTEEN) == FIFTEEN)
            printf("\n");
    }
    if ((i % SIXTEEN) != 0)
        printf("\n");
}

struct args {
    char *in;
    int i;
};

static int check_args(E_SIDE *side, int argc, char **argv, int samples)
{
    if (argc > ONE) {
        if (strcmp(argv[ONE], "client") == 0) {
            *side = E_SIDE::CLIENT;
        } else if (strcmp(argv[ONE], "server") == 0) {
            *side = E_SIDE::SERVER;
        } else {
            cout << "Argument 1 needs to be client OR server" << endl;
            return 0;
        }
        if (argc > THREE) {
            std::istringstream iss(argv[THREE]);
            if (!(iss >> samples)) {
                cout << "Problem reading samples number,using "
                        "default 10000 "
                        "samples "
                     << endl;
                samples = SAMPLES;
            }
        }
        return 1;
    } else {
        cout << "Client Server Test needs 1 arguments: (client/server)" << endl;
        return 0;
    }
}

int main(int argc, char **argv)
{
    cout << "Starting " << endl;
    E_SIDE side;
    int samples = SAMPLES;

    if (!check_args(&side, argc, argv, samples)) {
        return 0;
    };

    if (side == E_SIDE::SERVER) {
        SoftbusServer server;
        server.publish_service("Enclave_operation_remote",
                               EnclaveEngine::enclave_operation_remote);
        server.run();
    }
    if (side == E_SIDE::CLIENT) {
        struct elf_args *enclaveFile =
            static_cast<elf_args *>(malloc(sizeof(struct elf_args)));

        char *eappfile = argv[TWO];
        elf_args_init(enclaveFile, eappfile);
        if (!elf_valid(enclaveFile)) {
            printf("error when initializing enclaveFile\n");
        }

        struct Enclave *enclave =
            static_cast<Enclave *>(malloc(sizeof(struct Enclave)));

        struct enclave_args *params =
            static_cast<enclave_args *>(malloc(sizeof(struct enclave_args)));
        if (enclaveFile == nullptr || params == nullptr || enclave == nullptr) {
            return -1;
        }
        enclave->type = REMOTE_ENCLAVE;

        enclave_param_init(params);
        EnclaveEngine engine =
            EnclaveEngine(nullptr, enclave, enclaveFile, params);
        params->untrusted_mem_size = DEFAULT_UNTRUSTED_SIZE;
        params->untrusted_mem_ptr = 0;
        engine.enclave_init();
        if (engine.enclave_create() < 0) {
            printf("host: failed to create enclave\n");
        } else {
            enclave->type = REMOTE_ENCLAVE;
            engine.enclave_attest(NONCE);
            if (enclave->type != REMOTE_ENCLAVE) {
                cout << "enclave type!=REMOTE_ENCLAVE" << endl;
            }

            engine.enclave_run();
            if (enclave->type != REMOTE_ENCLAVE) {
                cout << "enclave type!=REMOTE_ENCLAVE" << endl;
            }
        }
        enclave->type = REMOTE_ENCLAVE;
        engine.enclave_finalize();
    }

    cout << "EVERYTHING STOPPED FINE" << endl;
}
