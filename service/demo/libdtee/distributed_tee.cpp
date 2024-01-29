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
#include <random>
#include <sstream>
#include <string>

#include <EnclaveEngine.h>
#include <cstdlib>
#include <face_recognition_shared_param.h>
#include <pthread.h>

#include <fstream>
#include <vector>
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
bool write_file(const char *path, const std::vector<char> &bytes) {
  std::ofstream file(path, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  file.write(bytes.data(), bytes.size());
  return !file.fail();
}

void record_mapping(const std::string &key, const std::string &val) {
  std::ofstream ofs("mapping", std::ios::app);
  ofs << key << std::endl << val << std::endl;
}

std::string get_value_with_key(const std::string &key) {
  std::ifstream ifs("mapping");
  std::string line;
  while (std::getline(ifs, line)) {
    if (line == key) {
      std::getline(ifs, line);
      return line;
    } else {
      std::getline(ifs, line);
    }
  }
  return "invalid";
}

std::string generate_random_string() {
  const std::string charset =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, charset.size() - 1);

  std::stringstream result;
  for (int i = 0; i < 16; ++i) {
    result << charset[dis(gen)];
  }

  return result.str();
}

int enclave_receiver(std::vector<char> enclave_file, std::string client_name) {
  std::cout << "RECEIVED ENCLAVE FILE FROM " << client_name << std::endl;
  std::string enclave_filename = generate_random_string() + ".signed.so";
  if (!write_file(enclave_filename.c_str(), enclave_file)) {
    return -1;
  }
  record_mapping(client_name, enclave_filename);
  return 0;
}

bool read_file(const char *path, std::vector<char> &bytes) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  file.seekg(0, std::ios::end);
  std::streampos file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  if (file_size <= 0) {
    file.close();
    return false;
  }

  bytes.resize(static_cast<size_t>(file_size));
  file.read(bytes.data(), file_size);
  file.close();

  return static_cast<size_t>(file_size) == bytes.size();
}

DistributedTeeContext *
init_distributed_tee_context(DistributedTeeConfig config) {
  DistributedTeeContext *context = new DistributedTeeContext{};
  g_current_dtee_context = context;
  context->config = config;
  if (config.side == SIDE::Server) {
    if (config.mode == MODE::COMPUTE_NODE) {
      publish_secure_function(context, enclave_receiver);
    }
  } else {
    context->client = new TeeClient;

    if (config.mode == MODE::MIGRATE) {
      std::vector<char> bytes;
      if (read_file("enclave.signed.so", bytes)) {
        const std::string &client_name = config.name;
        std::cout << "BEGIN MIGRATED ENCLAVE" << std::endl;
        g_current_dtee_context->client->call_service<int>(
            "enclave_receiver", ENCLAVE_UNRELATED, bytes, client_name);
        std::cout << "END MIGRATED ENCLAVE" << std::endl;
      }
    }
  }
  return context;
}

void destroy_distributed_tee_config(DistributedTeeContext *context) {
  delete context->client;
  delete context;
  for (const auto &s : context->servers) {
    delete s;
  }
}

#define DECL_FUNC_TYPE(FuncName, FuncRet, ...)                                 \
  typedef FuncRet (*FuncName)(__VA_ARGS__);                                    \
  static void proxy_##FuncName(Serialization *serialization, const char *data, \
                               int len) {}                                     \
  static void publish_##FuncName(DistributedTeeContext *context) {             \
    context->server->add_service_to_func(#FuncName, proxy_##FuncName);         \
  }

void tee_server_run(DistributedTeeContext *context) {
  using ProxyType =
      std::remove_pointer_t<decltype(std::declval<SoftbusServer>().run())>;
  std::vector<std::unique_ptr<ProxyType>> proxys;
  for (const auto &s : context->servers) {
    if (s) {
      auto *proxy = s->run();
      proxys.emplace_back(proxy);
    }
  }
  std::cout << "Enter a number to stop the server: ";
  int aux;
  std::cin >> aux;
}

int remote_ecall_enclave(void *enclave, uint32_t function_id,
                         const void *input_buffer, size_t input_buffer_size,
                         void *output_buffer, size_t output_buffer_size,
                         void *ms, const void *ocall_table) {
  printf("HOOKED: FUNCTION ID: %d, INPUT BUFFER SIZE: %lu, OUTPUT BUFFER SIZE: "
         "%lu\n",
         function_id, input_buffer_size, output_buffer_size);
  char *input_buffer_ = (char *)input_buffer;
  char *output_buffer_ = (char *)output_buffer;
  std::vector<char> in_buf(input_buffer_, input_buffer_ + input_buffer_size);
  std::vector<char> out_buf(output_buffer_,
                            output_buffer_ + output_buffer_size);
  const std::string &client_name = g_current_dtee_context->config.name;
  auto res =
      g_current_dtee_context->client->call_service<PackedMigrateCallResult>(
          "_Z_task_handler", ENCLAVE_UNRELATED, client_name, function_id,
          in_buf, out_buf);

  for (int i = 0; i < output_buffer_size; i++) {
    output_buffer_[i] = res.out_buf[i];
  }

  return res.res;
}

DistributedTeeContext *g_current_dtee_context;
// DECL_FUNC_TYPE(PLenclave_operation_remote_func, int, std::string)
