#pragma once

#include "DDSServer.h"
#include "Softbus.h"

typedef SoftbusServer TeeServer;
typedef SoftbusClient<> TeeClient;
enum class SIDE { Client, Server };

struct DistributedTeeConfig {
  SIDE side;
};

struct DistributedTeeContext {
  TeeServer *server = nullptr;
  TeeClient *client = nullptr;
};

template <typename Func>
void Z_publish_secure_function(DistributedTeeContext *context, std::string name,
                               Func &&func) {
  context->server->publish_service(name, std::forward<Func>(func));
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
template <typename Func, typename... Args>
typename return_type<Func>::type
Z_call_remote_secure_function(DistributedTeeContext *context, std::string name,
                              Func &&func, Args &&...args) {
  return context->client->call_service<typename return_type<Func>::type>(
      name, ENCLAVE_UNRELATED, std::forward<Args>(args)...);
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
