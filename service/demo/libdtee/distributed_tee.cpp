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
#include "DDSClient.h"
#include "DDSServer.h"
#include "Serialization.h"
#include "Softbus.h"
#include <bitset>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <distributed_tee.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <EnclaveEngine.h>
#include <cstdlib>
#include <face_recognition_shared_param.h>
#include <pthread.h>

using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;
using namespace enclaveelf;
using namespace demoparam;
using namespace penglaienclave;
using std::cout;
using std::endl;

enum class E_SIDE { CLIENT, SERVER };

static void printHex(unsigned char *c, int n) {
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
  void *in;
  int i;
};

static int PLenclave_operation_remote(std::string enclave_name) {
  // PLenclave_remote(enclave_name);
  return 3;
}

static int check_args(E_SIDE *side, int argc, char **argv, int samples) {
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

DistributedTeeContext *
init_distributed_tee_context(DistributedTeeConfig config) {
  DistributedTeeContext *context = new DistributedTeeContext{};
  if (config.side == SIDE::Server) {
    context->server = new TeeServer;
  } else {
    context->client = new TeeClient;
  }
  return context;
}

void destroy_distributed_tee_config(DistributedTeeContext *context) {
  delete context->server;
  delete context->client;
  delete context;
}

#define DECL_FUNC_TYPE(FuncName, FuncRet, ...)                                 \
  typedef FuncRet (*FuncName)(__VA_ARGS__);                                    \
  static void proxy_##FuncName(Serialization *serialization, const char *data, \
                               int len) {}                                     \
  static void publish_##FuncName(DistributedTeeContext *context) {             \
    context->server->add_service_to_func(#FuncName, proxy_##FuncName);         \
  }

void tee_server_run(DistributedTeeContext *context) { context->server->run(); }
// DECL_FUNC_TYPE(PLenclave_operation_remote_func, int, std::string)
