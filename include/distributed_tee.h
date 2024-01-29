#pragma once

#include "DDSServer.h"
#include "Softbus.h"

typedef SoftbusServer TeeServer;
typedef SoftbusClient<> TeeClient;
enum class SIDE { Client, Server };
enum class MODE { NORMAL, COMPUTE_NODE, MIGRATE };

struct DistributedTeeConfig {
  SIDE side;
  MODE mode;
  std::string name;
};

struct DistributedTeeContext {
  DistributedTeeConfig config;
  // each server is responsible for one service
  std::vector<TeeServer *> servers;
  TeeClient *client = nullptr;
};

template <typename Func>
void Z_publish_secure_function(DistributedTeeContext *context, std::string name,
                               Func &&func) {
  /* context->server->publish_service(name, std::forward<Func>(func)); */
  auto *server = new TeeServer;
  server->publish_service(name, std::forward<Func>(func));
  context->servers.push_back(server);
}

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

int remote_ecall_enclave(void *enclave, uint32_t function_id,
                         const void *input_buffer, size_t input_buffer_size,
                         void *output_buffer, size_t output_buffer_size,
                         void *ms, const void *ocall_table);

extern DistributedTeeContext *g_current_dtee_context;
template <typename Func, typename... Args>
typename return_type<Func>::type
Z_call_remote_secure_function(DistributedTeeContext *context, std::string name,
                              Func &&func, Args &&...args) {
  if (context->config.side == SIDE::Server) {
    // exit(-1);
    return func(std::forward<Args>(args)...);
  }

  if (context->config.mode == MODE::NORMAL) {
    return context->client->call_service<typename return_type<Func>::type>(
        name, ENCLAVE_UNRELATED, std::forward<Args>(args)...);
  }

  if (context->config.mode == MODE::MIGRATE) {
    return func(std::forward<Args>(args)...);
  }

  return {};
}

// interfaces:
DistributedTeeContext *
init_distributed_tee_context(DistributedTeeConfig config);
void destroy_distributed_tee_config(DistributedTeeContext *context);

#define publish_secure_function(context, func)                                 \
  { Z_publish_secure_function(context, #func, func); }
#define call_remote_secure_function(context, func, ...)                        \
  Z_call_remote_secure_function(context, #func, func, __VA_ARGS__)
void tee_server_run(DistributedTeeContext *context);
