#include "distributed_tee.h"

#ifdef __TEE

extern "C" int ecall_proxy(const char *enclave_filename, uint32_t fid,
                           char *in_buf, int in_buf_size, char *out_buf,
                           int out_buf_size);
#else
inline int ecall_proxy(const char *enclave_filename, uint32_t fid, char *in_buf,
                       int in_buf_size, char *out_buf, int out_buf_size) {
  std::cout << "NO TEE" << std::endl;
  return 0;
}
#endif

std::string get_value_with_key(const std::string &key);

inline PackedMigrateCallResult _Z_task_handler(std::string client_name,
                                               uint32_t function_id,
                                               std::vector<char> in_buf,
                                               std::vector<char> out_buf) {
  printf("fid: %d, in_buf_len: %lu, out_buf_len: %lu\n", function_id,
         in_buf.size(), out_buf.size());
  std::string enclave_name = get_value_with_key(client_name);
  int res = ecall_proxy(enclave_name.c_str(), function_id, in_buf.data(),
                        in_buf.size(), out_buf.data(), out_buf.size());
  PackedMigrateCallResult result = {.res = res, .out_buf = out_buf};
  return result;
}

#define dtee_server_run(ctx)                                                   \
  if (ctx->config.side == SIDE::Server &&                                      \
      ctx->config.mode == MODE::COMPUTE_NODE) {                                \
    publish_secure_function(ctx, _Z_task_handler);                             \
  }                                                                            \
  tee_server_run(ctx);
