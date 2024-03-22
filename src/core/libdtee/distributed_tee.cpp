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
#include <distributed_tee.h>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include "DDSClient.h"
#include "DDSServer.h"
#include "Serialization.h"
#include "Softbus.h"

#include <cstdlib>
#include <pthread.h>

#include <fstream>
#include <vector>
using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

extern "C"
{
    std::vector<char> (*get_report_func)(const char *enclave_path);
    bool (*is_report_valid_func)(void *report, const char *enclave_path);
    std::pair<std::string, std::string> (*make_key_pair_func)();
    std::string (*make_shared_key_func)(std::string pri_key,
                                        std::string in_pub_key);
    int (*key_exchange_func)(char *in_key,
                             int in_key_len,
                             char *out_key,
                             int out_key_len,
                             char *out_sealed_shared_key,
                             int out_sealed_shared_key_len);

    void (*sm4_encrypt)(const unsigned char *key,
                        unsigned char *buf,
                        int buf_len);
    void (*sm4_decrypt)(const unsigned char *key,
                        unsigned char *buf,
                        int buf_len);
}
enum class E_SIDE {
    CLIENT,
    SERVER
};

bool write_file(const char *path, const std::vector<char> &bytes)
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write(bytes.data(), bytes.size());
    return !file.fail();
}

auto get_enclave_version_mapping()
    -> std::unordered_map<std::string, std::string>
{
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
    const std::unordered_map<std::string, std::string> &map)
{
    std::ofstream ofs(MAPPING_FILE);
    for (const auto &[key, val] : map) {
        ofs << key << std::endl << val << std::endl;
    }
}

void get_enclave_version(const std::string &enclave_name,
                         std::string &enclave_version)
{
    auto map = get_enclave_version_mapping();
    auto it = map.find(enclave_name);
    if (it != map.end()) {
        enclave_version = it->second;
    } else {
        enclave_version = "";
    }
}

void put_enclave_version_mapping(const std::string &enclave_name,
                                 const std::string &version)
{
    auto map = get_enclave_version_mapping();
    map[enclave_name] = version;
    write_enclave_version_mapping(map);
}

const char *g_forced_enclave_path;

std::vector<char> enclave_receiver(std::string enclave_name,
                                   std::string enclave_version,
                                   std::string in_pub_key,
                                   std::vector<char> enclave_file)
{
    printf("OK name: %s version: %s filesize: %lu\n", enclave_name.c_str(),
           enclave_version.c_str(), enclave_file.size());
    std::string old_version;
    get_enclave_version(enclave_name, old_version);

    std::string enclave_filename = enclave_name + ENCLAVE_FILE_EXTENSION;
    float new_version;
    try {
        new_version = std::stof(enclave_version);
    } catch (std::invalid_argument e) {
        return {};
    }
    if (old_version.empty()) {
        put_enclave_version_mapping(enclave_name, enclave_version);
        if (!write_file(enclave_filename.c_str(), enclave_file)) {
            printf("ERROR\n");
            return {};
        }
        printf("RECEIVED NEW ENCLAVE FILE: %s(%luB):%f\n", enclave_name.c_str(),
               enclave_file.size(), new_version);
    } else {
        float old;
        try {
            old = std::stof(old_version);
        } catch (std::invalid_argument e) {
            return {};
        }
        if (new_version > old) {
            put_enclave_version_mapping(enclave_name, enclave_version);
            if (!write_file(enclave_filename.c_str(), enclave_file)) {
                printf("ERROR\n");
                return {};
            }
            printf("RECEIVED NEW ENCLAVE FILE: %s(%luB):%f\n",
                   enclave_name.c_str(), enclave_file.size(), new_version);
        }
    }

    if (get_report_func == 0) {
        printf("WARNING: get_report_func is not set\n");
        return {};
    }
    std::vector<char> res;
    std::vector<char> pub;
    pub.resize(64);

    std::vector<char> sealed_shared_key;
    if (key_exchange_func) {
        printf("KEY EXCHANGE\n");
        char sealed_shared_key_buf[256];
        g_forced_enclave_path = enclave_filename.c_str();
        int sealed_shared_key_len = key_exchange_func(
            (char *)in_pub_key.c_str(), 64, (char *)pub.data(), 64,
            sealed_shared_key_buf, sizeof(sealed_shared_key_buf));
        sealed_shared_key = {sealed_shared_key_buf,
                             sealed_shared_key_buf + sealed_shared_key_len};
        g_forced_enclave_path = 0;
    }
    // if (make_key_pair_func) {
    //     std::tie(pri, pub) = make_key_pair_func();
    //     if (make_shared_key_func) {
    //         auto shared = make_shared_key_func(pri, in_pub_key);
    //         for (const auto c : shared) {
    //             printf("%02x ", c);
    //         }
    //         puts("");
    //     }
    // }

    auto ret_key = std::string(pub.begin(), pub.end());
    res.insert(res.end(), ret_key.begin(), ret_key.end());

    auto ret_report = get_report_func(enclave_filename.c_str());
    uint32_t report_len = ret_report.size();
    res.insert(res.end(), (char *)&report_len, (char *)&report_len + 4);
    res.insert(res.end(), ret_report.begin(), ret_report.end());

    uint32_t sealed_shared_key_len = sealed_shared_key.size();
    res.insert(res.end(), (char *)&sealed_shared_key_len,
               (char *)&sealed_shared_key_len + 4);
    res.insert(res.end(), sealed_shared_key.begin(), sealed_shared_key.end());
    return res;
}

bool read_file(const char *path, std::vector<char> &bytes)
{
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

DistributedTeeContext *init_distributed_tee_context(DistributedTeeConfig config)
{
    DistributedTeeContext *context = new DistributedTeeContext{};
    g_current_dtee_context = context;
    context->config = config;
    if (config.side == SIDE::Server) {
        if (config.mode == MODE::ComputeNode) {
            publish_secure_function(context, enclave_receiver);
        }
    } else {
        context->client = new TeeClient;

        if (config.mode == MODE::Migrate ||
            config.mode == MODE::Transparent && !exist_local_tee()) {
            std::vector<char> bytes;
            if (read_file("enclave.signed.so", bytes)) {
                const std::string &enclave_name = config.name;
                printf("BEGIN MIGRATED ENCLAVE (%luB)", bytes.size());

                std::string pri, pub;
                if (make_key_pair_func) {
                    std::tie(pri, pub) = make_key_pair_func();
                }

                auto key_and_report =
                    g_current_dtee_context->client
                        ->call_service<std::vector<char>>(
                            "enclave_receiver",
                            g_current_dtee_context->enclave_id, enclave_name,
                            config.version, pub, bytes);

                if (key_and_report.size() != 0) {
                    char *buf = key_and_report.data();
                    const auto ret_pub_key = std::string(buf, buf + 64);
                    buf += 64;

                    if (make_shared_key_func) {
                        context->shared_key =
                            make_shared_key_func(pri, ret_pub_key);
                    }

                    uint32_t report_len = *(uint32_t *)buf;
                    buf += 4;

                    auto report = std::vector<char>(buf, buf + report_len);
                    buf += report_len;

                    uint32_t sealed_shared_key_len = *(uint32_t *)buf;
                    buf += 4;

                    context->sealed_shared_key = {buf,
                                                  buf + sealed_shared_key_len};
                    buf += sealed_shared_key_len;

                    if (is_report_valid_func != 0 &&
                        is_report_valid_func(report.data(),
                                             "enclave.signed.so")) {
                        printf("REPORT VALID\n");
                    } else {
                        printf("REPORT INVALID\n");
                    }
                } else {
                    printf("ERROR: NO KEY AND REPORT\n");
                    exit(-1);
                }
                int enclave_id = g_current_dtee_context->client
                                     ->get_client_proxy("enclave_receiver")
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

void destroy_distributed_tee_context(DistributedTeeContext *context)
{
    delete context->client;
    for (const auto &s : context->servers) {
        delete s;
    }
    delete context;
}

#define DECL_FUNC_TYPE(FuncName, FuncRet, ...)                                 \
    typedef FuncRet (*FuncName)(__VA_ARGS__);                                  \
    static void proxy_##FuncName(Serialization *serialization,                 \
                                 const char *data, int len)                    \
    {                                                                          \
    }                                                                          \
    static void publish_##FuncName(DistributedTeeContext *context)             \
    {                                                                          \
        context->server->add_service_to_func(#FuncName, proxy_##FuncName);     \
    }

void tee_server_run(DistributedTeeContext *context)
{
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

int distributed_tee_ecall_enclave(void *enclave,
                                  uint32_t function_id,
                                  const void *input_buffer,
                                  size_t input_buffer_size,
                                  void *output_buffer,
                                  size_t output_buffer_size,
                                  void *ms,
                                  const void *ocall_table)
{
    printf(
        "HOOKED: FUNCTION ID: %d, INPUT BUFFER SIZE: %lu, OUTPUT BUFFER SIZE: "
        "%lu\n",
        function_id, input_buffer_size, output_buffer_size);
    auto encrypt = [](std::vector<char> &in) {
        std::vector<char> res;
        int sealed_key_len = g_current_dtee_context->sealed_shared_key.size();
        printf("sealed key len = %d\n", sealed_key_len);
        int len = in.size();
        size_t aligned = (len + 15) / 16 * 16;

        in.resize(aligned);
        sm4_encrypt((unsigned char *)g_current_dtee_context->shared_key.c_str(),
                    (unsigned char *)in.data(), aligned);
        res.insert(res.end(), in.begin(), in.end());
        res.insert(res.end(), (char *)&len, (char *)&len + 4);

        res.insert(res.end(), g_current_dtee_context->sealed_shared_key.begin(),
                   g_current_dtee_context->sealed_shared_key.end());
        res.insert(res.end(), (char *)&sealed_key_len,
                   (char *)&sealed_key_len + 4);

        in.swap(res);
    };
    auto decrypt = [](std::vector<char> &out, int origin_data_len) {
        char *p = out.data() + out.size();
        *(char *)--p = 1;

        printf("decrypting... origin_data_len = %d\n", origin_data_len);
        sm4_decrypt((unsigned char *)g_current_dtee_context->shared_key.c_str(),
                    (unsigned char *)out.data(),
                    (origin_data_len + 15) / 16 * 16);
    };
    char *input_buffer_ = (char *)input_buffer;
    char *output_buffer_ = (char *)output_buffer;
    std::vector<char> in_buf(input_buffer_, input_buffer_ + input_buffer_size);
    std::vector<char> out_buf(output_buffer_,
                              output_buffer_ + output_buffer_size);

    encrypt(in_buf);

    int origin_out_data_len = out_buf.size();
    encrypt(out_buf);
    out_buf.push_back(0); // decrypted flag
    const std::string &client_name = g_current_dtee_context->config.name;
    auto res =
        g_current_dtee_context->client->call_service<PackedMigrateCallResult>(
            "_Z_task_handler", g_current_dtee_context->enclave_id, client_name,
            function_id, in_buf, out_buf);

    decrypt(res.out_buf, origin_out_data_len);
    for (int i = 0; i < output_buffer_size; i++) {
        output_buffer_[i] = res.out_buf[i];
    }

    std::cout << "RES: " << res.res << std::endl;

    return res.res;
}

DistributedTeeContext *g_current_dtee_context;
// DECL_FUNC_TYPE(PLenclave_operation_remote_func, int, std::string)
