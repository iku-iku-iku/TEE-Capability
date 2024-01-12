#pragma once

#include "fastrtps/TopicDataType.h"
#include "fastrtps/rtps/common/all_common.h"
#include "string.h"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#define MARK_LEN sizeof(uint16_t)
#define NET_CALL_TIMEOUT 100
#define CALL_SERVER_TIMEOUT 1000
#define HISTORY_DEPTH 1000
#define MAX_SAMPLES 1500
#define ALLOC_SAMPLES 1000
#define TIMEOUT_THOUSAND 1000
#define TIMEOUT_HUNDRED 100
#define RETRY_COUNT 10

inline int memcpy_s(void *dest, size_t destSize, const void *src,
                    size_t count) {
  if (dest == nullptr || src == nullptr)
    return EINVAL;
  if (destSize < count)
    return ERANGE;
  memmove(dest, src, count);
  return 0;
}

inline int memset_s(void *dest, size_t destSize, int ch, size_t count) {
  if (dest == nullptr || ch < 0) {
    return EINVAL;
  }
  if (destSize < count) {
    return ERANGE;
  }
  memset(dest, ch, count);
  return 0;
}

class StreamBuffer : public std::vector<char> {
public:
  StreamBuffer() { m_curpos = 0; }
  StreamBuffer(const char *in, size_t len) {
    m_curpos = 0;
    insert(begin(), in, in + len);
  }
  ~StreamBuffer() {}

  void reset() { m_curpos = 0; }
  const char *data() { return &(*this)[0]; }
  const char *current() { return &(*this)[m_curpos]; }
  void offset(int k) { m_curpos += k; }
  bool is_eof() { return (m_curpos >= size()); }
  void input(char *in, size_t len) { insert(end(), in, in + len); }
  int findc(char c) {
    iterator itr = find(begin() + m_curpos, end(), c);
    if (itr != end()) {
      return itr - (begin() + m_curpos);
    }
    return -1;
  }

private:
  // current byte stream position
  unsigned int m_curpos;
};

class Serialization {
public:
  Serialization() { m_byteorder = LittleEndian; }
  ~Serialization() {}

  Serialization(StreamBuffer dev, int byteorder = LittleEndian) {
    m_byteorder = byteorder;
    m_iodevice = dev;
  }

public:
  enum ByteOrder { BigEndian, LittleEndian };

public:
  void reset() { m_iodevice.reset(); }
  int size() { return m_iodevice.size(); }
  void skip_raw_date(int k) { m_iodevice.offset(k); }
  const char *data() { return m_iodevice.data(); }
  void byte_orser(char *in, int len) {
    if (m_byteorder == BigEndian) {
      std::reverse(in, in + len);
    }
  }
  void write_raw_data(char *in, int len) {
    m_iodevice.input(in, len);
    m_iodevice.offset(len);
  }
  const char *current() { return m_iodevice.current(); }
  void clear() {
    m_iodevice.clear();
    reset();
  }

  template <typename T> void output_type(T &t);

  template <typename T> void input_type(T t);

  // return x bytes of data after the current position
  void get_length_mem(char *p, int len) {
    if (memcpy_s(p, len, m_iodevice.current(), len) != 0) {
      return;
    }
    m_iodevice.offset(len);
  }

public:
  template <typename Tuple, std::size_t Id>
  void getv(Serialization &ds, Tuple &t) {
    ds >> std::get<Id>(t);
  }

  template <typename Tuple, std::size_t... I>
  Tuple get_tuple(std::index_sequence<I...>) {
    Tuple t;
    (void)std::initializer_list<int>{((getv<Tuple, I>(*this, t)), 0)...};
    return t;
  }

  template <typename T> Serialization &operator>>(T &i) {
    output_type(i);
    return *this;
  }

  template <typename T> Serialization &operator<<(T i) {
    input_type(i);
    return *this;
  }

private:
  int m_byteorder;
  StreamBuffer m_iodevice;
};

namespace clientserver {
#define THREE 3
#define GUID_SIZE 16
#define GUID_PREFIX_SIZE 12
#define GUID_ENTITYID_SIZE 4

#define MAX_TRANSFER_VECTOR_SIZE 200000

#define NOTIFICATION_MESSAGE 0
#define NORMAL_MESSAGE 1

#define ENCLAVE_UNRELATED (-1)
#define ENCLAVE_UNKNOWN 0

class Operation {
public:
  eprosima::fastrtps::rtps::GUID_t m_guid;
  int m_type; // 0: notification message, 1: normal message
  int m_vector_size;
  int m_enclave_id =
      ENCLAVE_UNRELATED; // -1: not related to enclave, 0: the enclave_id is
                         // unknown, positive number: the id of the enclave,
                         // which needs to be bound to the corresponding
                         // server.
  std::vector<char> m_vector;
  Operation() : m_vector_size(0) {}

  ~Operation() {}
};

class OperationDataType : public eprosima::fastrtps::TopicDataType {
public:
  OperationDataType() {
    setName("Operation");
    m_typeSize = GUID_SIZE + THREE * sizeof(int) +
                 MAX_TRANSFER_VECTOR_SIZE * sizeof(char);
    m_isGetKeyDefined = false;
  }

  ~OperationDataType() {}

  bool serialize(void *data,
                 eprosima::fastrtps::rtps::SerializedPayload_t *payload);
  bool deserialize(eprosima::fastrtps::rtps::SerializedPayload_t *payload,
                   void *data);
  std::function<uint32_t()> getSerializedSizeProvider(void *data);
  bool getKey(void *, eprosima::fastrtps::rtps::InstanceHandle_t *, bool) {
    return false;
  }

  void *createData();
  void deleteData(void *data);
};

class Result {
public:
  eprosima::fastrtps::rtps::GUID_t m_guid;
  int m_type; // 0: notification message, 1: normal message
  int m_vector_size;
  int m_enclave_id = ENCLAVE_UNRELATED;
  std::vector<char> m_vector;
  Result() : m_vector_size(0) {}

  ~Result() {}
};

class ResultDataType : public eprosima::fastrtps::TopicDataType {
public:
  ResultDataType() {
    setName("Result");
    m_typeSize = GUID_SIZE + THREE * sizeof(int) +
                 MAX_TRANSFER_VECTOR_SIZE * sizeof(char);
    m_isGetKeyDefined = false;
  }

  ~ResultDataType() {}

  bool serialize(void *data,
                 eprosima::fastrtps::rtps::SerializedPayload_t *payload);
  bool deserialize(eprosima::fastrtps::rtps::SerializedPayload_t *payload,
                   void *data);
  std::function<uint32_t()> getSerializedSizeProvider(void *data);
  bool getKey(void *, eprosima::fastrtps::rtps::InstanceHandle_t *, bool) {
    return false;
  }

  void *createData();
  void deleteData(void *data);
};
} // namespace clientserver

class DDSClient {
public:
  DDSClient();

  virtual ~DDSClient();

  eprosima::fastdds::dds::Publisher *mp_operation_pub;

  eprosima::fastdds::dds::DataWriter *mp_operation_writer;

  eprosima::fastdds::dds::Subscriber *mp_result_sub;

  eprosima::fastdds::dds::DataReader *mp_result_reader;

  eprosima::fastdds::dds::Topic *mp_operation_topic;

  eprosima::fastdds::dds::Topic *mp_result_topic;

  eprosima::fastdds::dds::DomainParticipant *mp_participant;

  bool init(std::string service_name = "");

  Serialization *call_service(Serialization *param, int enclave_id = -1);

  bool isReady();

private:
  std::string m_guid;

  clientserver::Operation m_operation;

  clientserver::Result m_result;

  void resetResult();

  eprosima::fastdds::dds::TypeSupport mp_resultdatatype;

  eprosima::fastdds::dds::TypeSupport mp_operationdatatype;

  class OperationListener : public eprosima::fastdds::dds::DataWriterListener {
  public:
    OperationListener(DDSClient *up) : mp_up(up) {}

    ~OperationListener() override {}

    DDSClient *mp_up;

    void on_publication_matched(
        eprosima::fastdds::dds::DataWriter *writer,
        const eprosima::fastdds::dds::PublicationMatchedStatus &info) override;
  } m_operationsListener;

  class ResultListener : public eprosima::fastdds::dds::DataReaderListener {
  public:
    ResultListener(DDSClient *up) : mp_up(up) {}

    ~ResultListener() override {}

    DDSClient *mp_up;

    void on_data_available(eprosima::fastdds::dds::DataReader *reader) override;

    void on_subscription_matched(
        eprosima::fastdds::dds::DataReader *reader,
        const eprosima::fastdds::dds::SubscriptionMatchedStatus &info) override;
  } m_resultsListener;

  bool m_isReady;

  int m_operationMatched;

  int m_resultMatched;

  void create_participant(std::string pqos_name);
};

class SoftbusServer;

class DDSServer {
  friend class OperationListener;
  friend class ResultListener;

public:
  DDSServer();

  virtual ~DDSServer();

  void init() {}

  bool publish_service(std::string service_name);

  // Serve indefinitely.
  void serve();

  void serve(class SoftbusServer *_server);

  std::string m_guid;
  uint32_t m_n_served;

  class OperationListener : public eprosima::fastdds::dds::DataReaderListener {
  public:
    class SoftbusServer *server;

    OperationListener(DDSServer *up) : mp_up(up) {}

    ~OperationListener() override {}

    DDSServer *mp_up;

    void on_data_available(eprosima::fastdds::dds::DataReader *reader) override;

    clientserver::Operation m_operation;

    clientserver::Result m_result;
  };

  class ResultListener : public eprosima::fastdds::dds::DataWriterListener {
  public:
    ResultListener(DDSServer *up) : mp_up(up) {}

    ~ResultListener() override {}

    DDSServer *mp_up;
  };

  OperationListener m_operationsListener;
  ResultListener m_resultsListener;

  bool m_isReadyDetect = false;
  int m_operationMatchedDetect = 0;
  int m_resultMatchedDetect = 0;

  bool isReadyDetect() {
    if (m_operationMatchedDetect >= 1 && m_resultMatchedDetect >= 1) {
      m_isReadyDetect = true;
    } else {
      m_isReadyDetect = false;
    }
    return m_isReadyDetect;
  }

  class OperationDetectListener
      : public eprosima::fastdds::dds::DataWriterListener {
  public:
    OperationDetectListener(DDSServer *up) : mp_up(up) {}

    ~OperationDetectListener() override {}

    DDSServer *mp_up;

    void on_publication_matched(
        eprosima::fastdds::dds::DataWriter * /* writer */,
        const eprosima::fastdds::dds::PublicationMatchedStatus &info) {
      if (info.current_count_change == 1) {
        mp_up->m_operationMatchedDetect++;
      } else if (info.current_count_change == -1) {
        mp_up->m_resultMatchedDetect--;
      } else {
        std::cout << info.current_count_change
                  << " is not a valid value for "
                     "PublicationMatchedStatus current count change"
                  << std::endl;
      }
      mp_up->isReadyDetect();
    }
  };

  class ResultDetectListener
      : public eprosima::fastdds::dds::DataReaderListener {
  public:
    ResultDetectListener(DDSServer *up) : mp_up(up) {}

    ~ResultDetectListener() override {}

    DDSServer *mp_up;

    void on_data_available(eprosima::fastdds::dds::DataReader * /* reader */) {}

    void on_subscription_matched(
        eprosima::fastdds::dds::DataReader * /* reader */,
        const eprosima::fastdds::dds::SubscriptionMatchedStatus &info) {
      if (info.current_count_change == 1) {
        mp_up->m_resultMatchedDetect++;
      } else if (info.current_count_change == -1) {
        mp_up->m_resultMatchedDetect--;
      } else {
        std::cout << info.current_count_change
                  << " is not a valid value for "
                     "SubscriptionMatchedStatus current count change"
                  << std::endl;
      }
      mp_up->isReadyDetect();
    }
  };

  OperationDetectListener m_operationsDetectListener;
  ResultDetectListener m_resultsDetectListener;

private:
  eprosima::fastdds::dds::Subscriber *mp_operation_sub;

  eprosima::fastdds::dds::DataReader *mp_operation_reader;

  eprosima::fastdds::dds::Publisher *mp_result_pub;

  eprosima::fastdds::dds::DataWriter *mp_result_writer;

  eprosima::fastdds::dds::Topic *mp_operation_topic;

  eprosima::fastdds::dds::Topic *mp_result_topic;

  eprosima::fastdds::dds::DomainParticipant *mp_participant;

  eprosima::fastdds::dds::TypeSupport mp_resultdatatype;

  eprosima::fastdds::dds::TypeSupport mp_operationdatatype;

  eprosima::fastdds::dds::TypeSupport mp_resultdatatype_detect;

  eprosima::fastdds::dds::TypeSupport mp_operationdatatype_detect;

  bool detect_ribbon(std::string service_name);
  bool create_ribbon(std::string service_name);
  void create_participant(std::string pqos_name);
  void sendMessageToRibbon(eprosima::fastdds::dds::DataWriter *writer_detect,
                           eprosima::fastdds::dds::DataReader *reader_detect);
};

class DDSRouter {
  friend class OperationListener;
  friend class ResultListener;

public:
  std::string service_name;
  static int call_times;
  int next_enclave_id = 1;
  std::unordered_map<int, int> enclave_id_to_server_index;

  DDSRouter(std::string _service_name);
  virtual ~DDSRouter();
  bool init();

  bool add_server(std::string server_guid);
  bool call_server(std::vector<char> &param, std::vector<char> &result,
                   int &enclave_id);

  class OperationListener : public eprosima::fastdds::dds::DataReaderListener {
  public:
    OperationListener(DDSRouter *up) : mp_up(up) {}

    ~OperationListener() override {}

    DDSRouter *mp_up;

    void on_data_available(eprosima::fastdds::dds::DataReader * /* reader */) {
      mp_up->call_times++;
      eprosima::fastdds::dds::SampleInfo m_sampleInfo;
      clientserver::Operation m_operation;
      clientserver::Result m_result;
      mp_up->mp_operation_reader->take_next_sample((char *)&m_operation,
                                                   &m_sampleInfo);
      if (m_sampleInfo.valid_data) {
        m_result.m_guid = m_operation.m_guid;
        int operation_type = m_operation.m_type;
        if (operation_type == NOTIFICATION_MESSAGE) {
          std::vector<char> register_guid_vector = m_operation.m_vector;
          std::string temp_guid;
          temp_guid.insert(temp_guid.begin(), register_guid_vector.begin(),
                           register_guid_vector.end());
          mp_up->add_server(temp_guid);
          m_result.m_type = NOTIFICATION_MESSAGE;
          mp_up->mp_result_writer->write((char *)&m_result);
        } else if (operation_type == NORMAL_MESSAGE) {
          std::vector<char> result_vector;
          mp_up->call_server(m_operation.m_vector, result_vector,
                             m_operation.m_enclave_id);
          m_result.m_type = NORMAL_MESSAGE;
          m_result.m_vector = result_vector;
          m_result.m_vector_size = result_vector.size();
          m_result.m_enclave_id = m_operation.m_enclave_id;
          mp_up->mp_result_writer->write((char *)&m_result);
        }
      }
    }
  };

  class ResultListener : public eprosima::fastdds::dds::DataWriterListener {
  public:
    ResultListener(DDSRouter *up) : mp_up(up) {}

    ~ResultListener() override {}

    DDSRouter *mp_up;
  };

  OperationListener m_operationsListener;
  ResultListener m_resultsListener;

  class OperationServerListener
      : public eprosima::fastdds::dds::DataWriterListener {
  public:
    OperationServerListener(DDSRouter *up) : mp_up(up) {}

    ~OperationServerListener() override {}

    DDSRouter *mp_up;

    void on_publication_matched(
        eprosima::fastdds::dds::DataWriter * /* writer */,
        const eprosima::fastdds::dds::PublicationMatchedStatus & /* info */) {}
  };

  class ResultServerListener
      : public eprosima::fastdds::dds::DataReaderListener {
  public:
    ResultServerListener(DDSRouter *up) : mp_up(up) {}

    ~ResultServerListener() override {}

    DDSRouter *mp_up;

    void on_data_available(eprosima::fastdds::dds::DataReader * /* reader */) {}

    void on_subscription_matched(
        eprosima::fastdds::dds::DataReader * /* reader */,
        const eprosima::fastdds::dds::SubscriptionMatchedStatus & /* info */) {}
  };

  std::vector<OperationServerListener *> m_operationsServerListenerList;
  std::vector<ResultServerListener *> m_resultsServerListenerList;

private:
  eprosima::fastdds::dds::Subscriber *mp_operation_sub;
  eprosima::fastdds::dds::DataReader *mp_operation_reader;
  eprosima::fastdds::dds::Publisher *mp_result_pub;
  eprosima::fastdds::dds::DataWriter *mp_result_writer;
  eprosima::fastdds::dds::Topic *mp_operation_topic;
  eprosima::fastdds::dds::Topic *mp_result_topic;
  eprosima::fastdds::dds::DomainParticipant *mp_participant;
  eprosima::fastdds::dds::TypeSupport mp_resultdatatype;
  eprosima::fastdds::dds::TypeSupport mp_operationdatatype;

  // The following parameters are used to save some information of the real
  // server
  std::vector<eprosima::fastdds::dds::Topic *> mp_operation_topic_server_list;
  std::vector<eprosima::fastdds::dds::Topic *> mp_result_topic_server_list;
  std::vector<eprosima::fastdds::dds::DataReader *>
      mp_result_reader_server_list;
  std::vector<eprosima::fastdds::dds::DataWriter *>
      mp_operation_writer_server_list;
  std::vector<int> server_status_list; // 1:running; 2:busy; 3:closed
  std::vector<std::string> guid_server_list;
  int index = 0; // The next index of server to be called
  int server_num = 0;

  void create_participant(std::string pqos_name);
  bool first_add_server(std::string server_guid,
                        eprosima::fastdds::dds::DataReader *&result_reader,
                        eprosima::fastdds::dds::DataWriter *&operation_writer,
                        eprosima::fastdds::dds::Topic *&operation_topic,
                        eprosima::fastdds::dds::Topic *&result_topic);
  bool
  non_first_add_server(std::string server_guid,
                       eprosima::fastdds::dds::DataReader *&result_reader,
                       eprosima::fastdds::dds::DataWriter *&operation_writer,
                       eprosima::fastdds::dds::Topic *&operation_topic,
                       eprosima::fastdds::dds::Topic *&result_topic);
  void adjust_index();
};

template <typename T> struct type_xx {
  typedef T type;
};

template <> struct type_xx<void> {
  typedef int8_t type;
};

template <typename T> class value_t {
public:
  typedef typename type_xx<T>::type type;
  typedef std::string msg_type;
  typedef uint16_t code_type;

  value_t() {
    code_ = 0;
    msg_.clear();
  }
  bool valid() { return (code_ == 0 ? true : false); }
  int error_code() { return code_; }
  std::string error_msg() { return msg_; }
  type val() { return val_; }

  void set_val(const type &val) { val_ = val; }
  void set_code(code_type code) { code_ = code; }
  void set_msg(msg_type msg) { msg_ = msg; }

  friend Serialization &operator>>(Serialization &in, value_t<T> &d) {
    in >> d.code_ >> d.msg_;
    if (d.code_ == 0) {
      in >> d.val_;
    }
    return in;
  }
  friend Serialization &operator<<(Serialization &out, value_t<T> d) {
    out << d.code_ << d.msg_ << d.val_;
    return out;
  }

private:
  code_type code_;
  msg_type msg_;
  type val_;
};

class SoftbusServer {
public:
  SoftbusServer() {}

  template <typename ServerProxy = DDSServer> // ServerProxy must implement:
                                              // init, serve, publish_service
  void run() {
    ServerProxy server;
    server.init();
    server.publish_service(service_names[0]);
    server.serve(this);
  }

  // call function template class using tuple as parameter
  template <typename Function, typename Tuple, std::size_t... Index>
  decltype(auto) invoke_impl(Function &&func, Tuple &&t,
                             std::index_sequence<Index...>) {
    return func(std::get<Index>(std::forward<Tuple>(t))...);
  }

  template <typename Function, typename Tuple>
  decltype(auto) invoke(Function &&func, Tuple &&t) {
    constexpr auto size =
        std::tuple_size<typename std::decay<Tuple>::type>::value;
    return invoke_impl(std::forward<Function>(func), std::forward<Tuple>(t),
                       std::make_index_sequence<size>{});
  }

  // used when the return value is void
  template <typename R, typename F, typename ArgsTuple>
  typename std::enable_if<std::is_same<R, void>::value,
                          typename type_xx<R>::type>::type
  call_helper(F f, ArgsTuple args) {
    invoke(f, args);
    return 0;
  }

  template <typename R, typename F, typename ArgsTuple>
  typename std::enable_if<!std::is_same<R, void>::value,
                          typename type_xx<R>::type>::type
  call_helper(F f, ArgsTuple args) {
    return invoke(f, args);
  }

  template <typename R, typename... Params>
  void service_proxy_(std::function<R(Params... ps)> service,
                      Serialization *serialization, const char *data, int len) {
    using args_type = std::tuple<typename std::decay<Params>::type...>;

    Serialization param_serialization(StreamBuffer(data, len));
    constexpr auto N =
        std::tuple_size<typename std::decay<args_type>::type>::value;
    args_type args =
        param_serialization.get_tuple<args_type>(std::make_index_sequence<N>{});

    typename type_xx<R>::type r = call_helper<R>(service, args);
    (*serialization) << r;
  }

  template <typename R, typename... Params>
  void service_proxy_(R (*service)(Params...), Serialization *serialization,
                      const char *data, int len) {
    service_proxy_(std::function<R(Params...)>(service), serialization, data,
                   len);
  }

  template <typename S>
  void service_proxy(S service, Serialization *serialization, const char *data,
                     int len) {
    service_proxy_(service, serialization, data, len);
  }

  template <typename S>
  void publish_service(std::string service_name, S service) {
    service_to_func[service_name] = std::bind(
        &SoftbusServer::service_proxy<S>, this, service, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3);
    service_names.push_back(service_name);
  }

  Serialization *call_(std::string name, const char *data, int len) {
    Serialization *ds = new Serialization();
    if (service_to_func.find(name) == service_to_func.end()) {
      return ds;
    }
    auto fun = service_to_func[name];
    fun(ds, data, len);
    ds->reset();
    return ds;
  }

  typedef void (*ServiceFunc)(Serialization *, const char *, int);
  void add_service_to_func(std::string service_name, ServiceFunc func) {
    service_to_func[service_name] = func;
  }

private:
  std::map<std::string, std::function<void(Serialization *, const char *, int)>>
      service_to_func;
  std::vector<std::string> service_names;
};

template <typename ClientProxy = DDSClient> // ClientProxy must implement: init,
                                            // call_service, isReady
class SoftbusClient {
public:
  SoftbusClient() {}

  ~SoftbusClient() {}

  template <typename Arg>
  void package_params(Serialization &ds, const Arg &arg) {
    ds << arg;
  }

  template <typename Arg, typename... Args>
  void package_params(Serialization &ds, const Arg &arg, const Args &...args) {
    ds << arg;
    package_params(ds, args...);
  }

  template <typename V> V net_call(Serialization &ds, int enclave_id = -1) {
    while (!m_client.isReady()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(NET_CALL_TIMEOUT));
    }

    Serialization *result = m_client.call_service(&ds, enclave_id);
    V val;
    (*result) >> val;
    return val;
  }

  template <typename V, typename... Params>
  V call_service(std::string service_name, int enclave_id, Params... params) {
    if (service_list.count(service_name) <= 0) {
      m_client.init(service_name);
      service_list.insert(service_name);
    }

    Serialization ds;
    ds << service_name;
    package_params(ds, params...);
    return net_call<V>(ds, enclave_id);
  }

private:
  ClientProxy m_client;
  std::set<std::string> service_list;
};

template <typename T> inline void Serialization::output_type(T &t) {
  int len = sizeof(T);
  char *d = new char[len];
  if (!m_iodevice.is_eof()) {
    memcpy_s(d, len, m_iodevice.current(), len);
    m_iodevice.offset(len);
    byte_orser(d, len);
    t = *reinterpret_cast<T *>(&d[0]);
  }
  delete[] d;
}

template <> inline void Serialization::output_type(std::string &in) {
  char *d = new char[MARK_LEN];
  memcpy_s(d, MARK_LEN, m_iodevice.current(), MARK_LEN);
  byte_orser(d, MARK_LEN);
  int len = *reinterpret_cast<uint16_t *>(&d[0]);
  m_iodevice.offset(MARK_LEN);
  delete[] d;
  if (len == 0)
    return;
  in.insert(in.begin(), m_iodevice.current(), m_iodevice.current() + len);
  m_iodevice.offset(len);
}

template <typename T> inline void Serialization::input_type(T t) {
  int len = sizeof(T);
  char *d = new char[len];
  const char *p = reinterpret_cast<const char *>(&t);
  memcpy_s(d, len, p, len);
  byte_orser(d, len);
  m_iodevice.input(d, len);
  delete[] d;
}

template <> inline void Serialization::input_type(std::string in) {
  // store the string length first
  uint16_t len = in.size();
  char *p = reinterpret_cast<char *>(&len);
  byte_orser(p, sizeof(uint16_t));
  m_iodevice.input(p, sizeof(uint16_t));
  // store string content
  if (len <= 0)
    return;
  char *d = new char[len];
  memcpy_s(d, len, in.c_str(), len);
  m_iodevice.input(d, len);
  delete[] d;
}

template <> inline void Serialization::input_type(const char *in) {
  input_type<std::string>(std::string(in));
}

typedef SoftbusServer TeeServer;
typedef SoftbusClient<> TeeClient;

struct DistributedTeeContext {
  enum class SIDE { Client, Server } side;
  TeeServer *server;
  TeeClient *client;
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
void init_distributed_tee_framework(DistributedTeeContext *context);
#define publish_secure_function(context, func)                                 \
  { Z_publish_secure_function(context, #func, func); }
#define call_remote_secure_function(context, func, ...)                        \
  Z_call_remote_secure_function(context, #func, func, __VA_ARGS__)
void tee_server_run(DistributedTeeContext *context);
