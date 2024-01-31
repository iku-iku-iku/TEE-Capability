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

auto get_enclave_version_mapping()
    -> std::unordered_map<std::string, std::string> {
  std::unordered_map<std::string, std::string> map;
  std::ifstream ifs(MAPPING_FILE);
  if (ifs.is_open()) {
    std::string line;
    while (std::getline(ifs, line)) {
      std::string val;
      std::getline(ifs, val);
      map[line] = std::move(val);
    }
  }
  return map;
}

void write_enclave_version_mapping(
    const std::unordered_map<std::string, std::string> &map) {
  std::ofstream ofs(MAPPING_FILE);
  for (const auto &[key, val] : map) {
    ofs << key << std::endl << val << std::endl;
  }
}

void get_enclave_version(const std::string &enclave_name,
                         std::string &enclave_version) {
  auto map = get_enclave_version_mapping();
  auto it = map.find(enclave_name);
  if (it != map.end()) {
    enclave_version = it->second;
  } else {
    enclave_version = "";
  }
}

void put_enclave_version_mapping(const std::string &enclave_name,
                                 const std::string &version) {
  auto map = get_enclave_version_mapping();
  map[enclave_name] = version;
  write_enclave_version_mapping(map);
}

int enclave_receiver(std::vector<char> enclave_file, std::string enclave_name,
                     std::string enclave_version) {
  std::string old_version;
  get_enclave_version(enclave_name, old_version);

  float new_version;
  try {
    new_version = std::stof(enclave_version);
  } catch (std::invalid_argument e) {
    return -1;
  }
  if (old_version.empty()) {
    put_enclave_version_mapping(enclave_name, enclave_version);
  } else {
    float old;
    try {
      old = std::stof(old_version);
    } catch (std::invalid_argument e) {
      return -1;
    }
    if (new_version > old) {
      put_enclave_version_mapping(enclave_name, enclave_version);
    } else {
      return 0;
    }
  }
  printf("RECEIVED NEW ENCLAVE FILE: %s:%f\n", enclave_name.c_str(),
         new_version);
  std::string enclave_filename = enclave_name + ENCLAVE_FILE_EXTENSION;
  if (!write_file(enclave_filename.c_str(), enclave_file)) {
    return -1;
  }
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
    if (config.mode == MODE::ComputeNode) {
      publish_secure_function(context, enclave_receiver);
    }
  } else {
    context->client = new TeeClient;

    if (config.mode == MODE::Migrate) {
      std::vector<char> bytes;
      if (read_file("enclave.signed.so", bytes)) {
        const std::string &enclave_name = config.name;
        std::cout << "BEGIN MIGRATED ENCLAVE" << std::endl;
        g_current_dtee_context->client->call_service<int>(
            "enclave_receiver", g_current_dtee_context->enclave_id, bytes,
            enclave_name, config.version);
        int enclave_id = g_current_dtee_context->client->get_client_proxy()
                             .get_result()
                             .m_enclave_id;
        // the enclave now resides in the node with *enclave_id*
        g_current_dtee_context->enclave_id = enclave_id;
        std::cout << "END MIGRATED ENCLAVE" << std::endl;
      }
    }
  }
  return context;
}

void destroy_distributed_tee_context(DistributedTeeContext *context) {
  delete context->client;
  for (const auto &s : context->servers) {
    delete s;
  }
  delete context;
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

int distributed_tee_ecall_enclave(void *enclave, uint32_t function_id,
                                  const void *input_buffer,
                                  size_t input_buffer_size, void *output_buffer,
                                  size_t output_buffer_size, void *ms,
                                  const void *ocall_table) {
  printf("HOOKED: FUNCTION ID: %d, INPUT BUFFER SIZE: %lu, OUTPUT BUFFER SIZE: "
         "%lu\n",
         function_id, input_buffer_size, output_buffer_size);
  /*   using EcallEnclaveFunc = decltype(distributed_tee_ecall_enclave) *; */
  /* #define FORWARD_ECALL_ENCLAVE \ */
  /*   return reinterpret_cast<EcallEnclaveFunc>( \ */
  /*       g_current_dtee_context->ecall_enclave)( \ */
  /*       enclave, function_id, input_buffer, input_buffer_size, output_buffer,
   * \ */
  /*       output_buffer_size, ms, ocall_table); */
  /**/
  /*   if (g_current_dtee_context->config.side == SIDE::Server) { */
  /*     FORWARD_ECALL_ENCLAVE */
  /*   } */
  /**/
  /*   if (g_current_dtee_context->config.mode == MODE::Transparent && */
  /*       exist_local_tee()) { */
  /*     printf("using local tee\n"); */
  /*     FORWARD_ECALL_ENCLAVE */
  /*   } */
  /**/
  /*   if (g_current_dtee_context->config.mode == MODE::Normal) { */
  /*     FORWARD_ECALL_ENCLAVE */
  /*   } */
  /**/
  char *input_buffer_ = (char *)input_buffer;
  char *output_buffer_ = (char *)output_buffer;
  std::vector<char> in_buf(input_buffer_, input_buffer_ + input_buffer_size);
  std::vector<char> out_buf(output_buffer_,
                            output_buffer_ + output_buffer_size);
  const std::string &client_name = g_current_dtee_context->config.name;
  auto res =
      g_current_dtee_context->client->call_service<PackedMigrateCallResult>(
          "_Z_task_handler", g_current_dtee_context->enclave_id, client_name,
          function_id, in_buf, out_buf);

  for (int i = 0; i < output_buffer_size; i++) {
    output_buffer_[i] = res.out_buf[i];
  }

  std::cout << "RES: " << res.res << std::endl;

  return res.res;
}

DistributedTeeContext *g_current_dtee_context;
// DECL_FUNC_TYPE(PLenclave_operation_remote_func, int, std::string)
