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

#include "DDSClient.h"
#include "DDSServer.h"
#include "Serialization.h"
#include "Softbus.h"

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
typedef SoftbusServer TeeServer;
typedef SoftbusClient<> TeeClient;
struct DistributedTeeContext {
  enum class SIDE { Client, Server } side;
  TeeServer *server;
  TeeClient *client;
};

void init_distributed_tee_framework(DistributedTeeContext *context) {
  if (context->side == DistributedTeeContext::SIDE::Server) {
    context->server = new TeeServer;
  } else {
    context->client = new TeeClient;
  }
}
#define DECL_FUNC_TYPE(FuncName, FuncRet, ...)                                 \
  typedef FuncRet (*FuncName)(__VA_ARGS__);                                    \
  static void proxy_##FuncName(Serialization *serialization, const char *data, \
                               int len) {}                                     \
  static void publish_##FuncName(DistributedTeeContext *context) {             \
    context->server->add_service_to_func(#FuncName, proxy_##FuncName);         \
  }

// DECL_FUNC_TYPE(PLenclave_operation_remote_func, int, std::string)

template <typename Func>
void Z_publish_secure_function(DistributedTeeContext *context, std::string name,
                               Func &&func) {
  context->server->publish_service(name, std::forward<Func>(func));
}
#define publish_secure_function(context, func)                                 \
  { Z_publish_secure_function(context, #func, func); }

template <typename T> struct return_type;

template <typename R, typename... Args> struct return_type<R (*)(Args...)> {
  using type = R;
};

template <typename R, typename... Args> struct return_type<R(Args...)> {
  using type = R;
};

template <typename R, typename... Args> struct return_type<R (&)(Args...)> {
  using type = R;
};

template <typename R, typename... Args> struct return_type<R(Args...) const> {
  using type = R;
};

template <typename R, typename... Args>
struct return_type<std::function<R(Args...)>> {
  using type = R;
};
template <typename Func, typename... Args>
/* typename GetRetType<Func>::Type */
typename return_type<Func>::type
Z_call_remote_secure_function(DistributedTeeContext *context, std::string name,
                              Func &&func, Args &&...args) {
  return context->client->call_service<typename return_type<Func>::type>(
      name, ENCLAVE_UNRELATED, std::forward<Args>(args)...);
}

#define call_remote_secure_function(context, func, ...)                        \
  Z_call_remote_secure_function(context, #func, func, __VA_ARGS__)
void tee_server_run(DistributedTeeContext *context) { context->server->run(); }

int main(int argc, char **argv) {
  cout << "Starting " << endl;
  E_SIDE side;
  int samples = SAMPLES;

  if (!check_args(&side, argc, argv, samples)) {
    return 0;
  };

  DistributedTeeContext context;
  context.server = nullptr;
  context.client = nullptr;

  if (side == E_SIDE::SERVER) {
    context.side = DistributedTeeContext::SIDE::Server;
    init_distributed_tee_framework(&context);
    publish_secure_function(&context, PLenclave_operation_remote);
    /* publish_PLenclave_operation_remote_func(&context); */
    /* context.server->publish_service("PLenclave_operation_remote", */
    /*                                 PLenclave_operation_remote); */
    tee_server_run(&context);
  }
  if (side == E_SIDE::CLIENT) {
    context.side = DistributedTeeContext::SIDE::Client;
    init_distributed_tee_framework(&context);
    char *eappfile = argv[TWO];
    std::string enclave_name = eappfile;

    /* int result = context.client->call_service<int>( */
    /*     "PLenclave_operation_remote", ENCLAVE_UNRELATED, enclave_name); */
    int result = call_remote_secure_function(
        &context, PLenclave_operation_remote, enclave_name);
    printf("result is %d", result);
  }

  cout << "EVERYTHING STOPPED FINE" << endl;
}
