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

#include "DDSServer.h"

#include "Softbus.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastrtps::rtps;
using namespace clientserver;
using namespace std;

int DDSRouter::call_times = 0;

DDSServer::DDSServer()
    : mp_operation_sub(nullptr), mp_result_pub(nullptr),
      mp_participant(nullptr), mp_resultdatatype(new ResultDataType()),
      mp_operationdatatype(new OperationDataType()),
      mp_resultdatatype_detect(new ResultDataType()),
      mp_operationdatatype_detect(new OperationDataType()), m_n_served(0),
      m_operationsListener(nullptr), m_resultsListener(nullptr),
      m_operationsDetectListener(nullptr), m_resultsDetectListener(nullptr) {
  m_operationsListener.mp_up = this;
  m_resultsListener.mp_up = this;
  m_operationsDetectListener.mp_up = this;
  m_resultsDetectListener.mp_up = this;
}

DDSServer::~DDSServer() {
  if (mp_operation_reader != nullptr) {
    mp_operation_sub->delete_datareader(mp_operation_reader);
  }
  if (mp_operation_sub != nullptr) {
    mp_participant->delete_subscriber(mp_operation_sub);
  }
  if (mp_operation_topic != nullptr) {
    mp_participant->delete_topic(mp_operation_topic);
  }
  if (mp_result_writer != nullptr) {
    mp_result_pub->delete_datawriter(mp_result_writer);
  }
  if (mp_result_pub != nullptr) {
    mp_participant->delete_publisher(mp_result_pub);
  }
  if (mp_result_topic != nullptr) {
    mp_participant->delete_topic(mp_result_topic);
  }
  DomainParticipantFactory::get_instance()->delete_participant(mp_participant);
}

void DDSServer::serve() {
  /* cout << "Enter a number to stop the server: "; */
  /* int aux; */
  /* std::cin >> aux; */
}

void DDSServer::serve(class SoftbusServer *_server) {
  m_operationsListener.server = _server;
  /* cout << "Enter a number to stop the server: "; */
  /* int aux; */
  /* std::cin >> aux; */
}

bool DDSServer::create_ribbon(std::string service_name) {
  DDSRouter *router = new DDSRouter(service_name);
  router->init();
  return true;
}

static DataWriterQos create_dataWriterQos() {
  DataWriterQos wqos;
  wqos.history().kind = KEEP_LAST_HISTORY_QOS;
  wqos.history().depth = HISTORY_DEPTH;
  wqos.resource_limits().max_samples = MAX_SAMPLES;
  wqos.resource_limits().allocated_samples = ALLOC_SAMPLES;
  wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
  return wqos;
}

static DataReaderQos create_dataReaderQos() {
  DataReaderQos rqos;
  rqos.history().kind = KEEP_LAST_HISTORY_QOS;
  rqos.history().depth = HISTORY_DEPTH;
  rqos.resource_limits().max_samples = MAX_SAMPLES;
  rqos.resource_limits().allocated_samples = ALLOC_SAMPLES;
  return rqos;
}

void DDSServer::sendMessageToRibbon(
    eprosima::fastdds::dds::DataWriter *writer_detect,
    eprosima::fastdds::dds::DataReader *reader_detect) {
  SampleInfo m_sampleInfo_register;

  clientserver::Operation m_operation_register;
  clientserver::Result m_result_register;
  m_operation_register.m_type = NOTIFICATION_MESSAGE;
  std::vector<char> register_guid_vector;
  for (std::size_t i = 0; i < m_guid.size(); i++) {
    register_guid_vector.push_back(m_guid[i]);
  }
  m_operation_register.m_vector = register_guid_vector;
  m_operation_register.m_vector_size = register_guid_vector.size();
  writer_detect->write((char *)&m_operation_register);
  do {
    m_result_register.m_guid = c_Guid_Unknown;
    reader_detect->wait_for_unread_message({RETRY_COUNT, 0});
    reader_detect->take_next_sample((char *)&m_result_register,
                                    &m_sampleInfo_register);
  } while (m_sampleInfo_register.instance_state !=
               eprosima::fastdds::dds::ALIVE_INSTANCE_STATE ||
           m_result_register.m_guid != m_operation_register.m_guid);
}

bool DDSServer::detect_ribbon(std::string service_name) {
  // The following parameters are used to detect whether there is a Ribbon for
  // this service
  eprosima::fastdds::dds::DataReader *mp_result_reader_detect;
  eprosima::fastdds::dds::DataWriter *mp_operation_writer_detect;
  eprosima::fastdds::dds::Topic *mp_operation_topic_detect;
  eprosima::fastdds::dds::Topic *mp_result_topic_detect;

  // REGISTER TYPES
  mp_resultdatatype_detect.register_type(mp_participant);
  mp_operationdatatype_detect.register_type(mp_participant);

  // CREATE THE PARAM TOPIC
  std::string mp_operation_topic_detect_name =
      service_name + std::string("_Param");
  mp_operation_topic_detect = mp_participant->create_topic(
      mp_operation_topic_detect_name, "Operation", TOPIC_QOS_DEFAULT);
  if (mp_operation_topic_detect == nullptr) {
    return false;
  }

  // CREATE THE DATAWRITER ON PARAM TOPIC
  DataWriterQos wqos = create_dataWriterQos();
  mp_operation_writer_detect = mp_result_pub->create_datawriter(
      mp_operation_topic_detect, wqos, &this->m_operationsDetectListener);
  if (mp_operation_writer_detect == nullptr) {
    return false;
  }

  // CREATE THE RESULT TOPIC
  std::string mp_result_topic_detect_name =
      service_name + std::string("_Result");
  mp_result_topic_detect = mp_participant->create_topic(
      mp_result_topic_detect_name, "Result", TOPIC_QOS_DEFAULT);
  if (mp_result_topic_detect == nullptr) {
    return false;
  }

  // CREATE THE DATAREADER ON RESULT TOPIC
  DataReaderQos rqos = create_dataReaderQos();
  mp_result_reader_detect = mp_operation_sub->create_datareader(
      mp_result_topic_detect, rqos, &this->m_resultsDetectListener);
  if (mp_result_reader_detect == nullptr) {
    return false;
  }

  // Try to send a message to Ribbon to detect if there is a ribbon
  int count = 0;
  while ((!isReadyDetect()) && (count < RETRY_COUNT)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_HUNDRED));
    count++;
  }
  if (!isReadyDetect()) { // no Ribbon
    create_ribbon(service_name);
  }

  // send a register message to Ribbon
  std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_THOUSAND));
  // isReadyDetect();

  sendMessageToRibbon(mp_operation_writer_detect, mp_result_reader_detect);

  return true;
}

void DDSServer::create_participant(std::string pqos_name) {
  DomainParticipantQos pqos;
  pqos.wire_protocol()
      .builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = true;
  pqos.wire_protocol().builtin.discovery_config.discoveryProtocol =
      eprosima::fastrtps::rtps::DiscoveryProtocol::SIMPLE;
  pqos.wire_protocol()
      .builtin.discovery_config.m_simpleEDP
      .use_PublicationReaderANDSubscriptionWriter = true;
  pqos.wire_protocol()
      .builtin.discovery_config.m_simpleEDP
      .use_PublicationWriterANDSubscriptionReader = true;
  pqos.wire_protocol().builtin.discovery_config.leaseDuration =
      eprosima::fastrtps::c_TimeInfinite;
  pqos.name(pqos_name);

  mp_participant =
      DomainParticipantFactory::get_instance()->create_participant(0, pqos);
}

bool DDSServer::publish_service(std::string service_name) {
  // Generate guid
  m_guid = get_random_guid();

  // Create core service listening Topic pair
  // CREATE THE SERVER PARTICIPANT
  std::string pqos_name = std::string("server_RTPSParticipant_") + m_guid;
  create_participant(pqos_name);
  if (mp_participant == nullptr) {
    return false;
  }

  // First monitor the corresponding Param and Result
  // REGISTER TYPES
  mp_resultdatatype.register_type(mp_participant);
  mp_operationdatatype.register_type(mp_participant);

  // CREATE THE PUBLISHER
  mp_result_pub = mp_participant->create_publisher(PUBLISHER_QOS_DEFAULT);
  if (mp_result_pub == nullptr) {
    return false;
  }

  // CREATE THE RESULT TOPIC
  std::string mp_result_topic_name =
      service_name + std::string("_Result_") + m_guid;
  mp_result_topic = mp_participant->create_topic(mp_result_topic_name, "Result",
                                                 TOPIC_QOS_DEFAULT);
  if (mp_result_topic == nullptr) {
    return false;
  }

  // CREATE THE DATAWRITER ON RESULT TOPIC
  DataWriterQos wqos = create_dataWriterQos();
  mp_result_writer = mp_result_pub->create_datawriter(mp_result_topic, wqos,
                                                      &m_resultsListener);
  if (mp_result_writer == nullptr) {
    return false;
  }

  // CREATE THE SUBSCRIBER
  mp_operation_sub = mp_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
  if (mp_operation_sub == nullptr) {
    return false;
  }

  // CREATE THE PARAM TOPIC
  std::string mp_operation_topic_name =
      service_name + std::string("_Param_") + m_guid;
  mp_operation_topic = mp_participant->create_topic(
      mp_operation_topic_name, "Operation", TOPIC_QOS_DEFAULT);
  if (mp_operation_topic == nullptr) {
    return false;
  }

  // CREATE THE DATAREADER ON PARAM TOPIC
  DataReaderQos rqos = create_dataReaderQos();
  mp_operation_reader = mp_operation_sub->create_datareader(
      mp_operation_topic, rqos, &m_operationsListener);
  if (mp_operation_reader == nullptr) {
    return false;
  }

  // Detect whether there is a Ribbon for this service
  return detect_ribbon(service_name);
}

void DDSServer::OperationListener::on_data_available(
    DataReader * /* reader */) {
  SampleInfo m_sampleInfo;
  // clear m_operation
  m_operation.m_vector.clear();
  mp_up->mp_operation_reader->take_next_sample((char *)&m_operation,
                                               &m_sampleInfo);
  if (m_sampleInfo.valid_data) {
    int operation_type = m_operation.m_type;
    if (operation_type != NOTIFICATION_MESSAGE) {
      ++mp_up->m_n_served;
      m_result.m_guid = m_operation.m_guid;

      std::vector<char> test_vector = m_operation.m_vector;
      Serialization ds(StreamBuffer(&test_vector[0], test_vector.size()));

      std::string funname;
      ds >> funname;

      Serialization *result =
          server->call_(funname, ds.current(), ds.size() - funname.size());

      std::vector<char> result_vector;
      for (int i = 0; i < result->size(); i++) {
        result_vector.push_back(result->data()[i]);
      }
      m_result.m_type = NORMAL_MESSAGE;
      m_result.m_vector_size = result->size();
      m_result.m_vector = result_vector;
      m_result.m_enclave_id = m_operation.m_enclave_id;
      mp_up->mp_result_writer->write((char *)&m_result);
    }
  }
}

DDSRouter::DDSRouter(std::string _service_name)
    : mp_operation_sub(nullptr), mp_result_pub(nullptr),
      mp_participant(nullptr), mp_resultdatatype(new ResultDataType()),
      mp_operationdatatype(new OperationDataType()),
      m_operationsListener(nullptr), m_resultsListener(nullptr) {
  service_name = _service_name;
  m_operationsListener.mp_up = this;
  m_resultsListener.mp_up = this;
}

DDSRouter::~DDSRouter() {
  if (mp_operation_reader != nullptr) {
    mp_operation_sub->delete_datareader(mp_operation_reader);
  }
  if (mp_operation_sub != nullptr) {
    mp_participant->delete_subscriber(mp_operation_sub);
  }
  if (mp_operation_topic != nullptr) {
    mp_participant->delete_topic(mp_operation_topic);
  }
  if (mp_result_writer != nullptr) {
    mp_result_pub->delete_datawriter(mp_result_writer);
  }
  if (mp_result_pub != nullptr) {
    mp_participant->delete_publisher(mp_result_pub);
  }
  if (mp_result_topic != nullptr) {
    mp_participant->delete_topic(mp_result_topic);
  }
  DomainParticipantFactory::get_instance()->delete_participant(mp_participant);
}

void DDSRouter::create_participant(std::string pqos_name) {
  DomainParticipantQos pqos;
  pqos.wire_protocol()
      .builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = true;
  pqos.wire_protocol().builtin.discovery_config.discoveryProtocol =
      eprosima::fastrtps::rtps::DiscoveryProtocol::SIMPLE;
  pqos.wire_protocol()
      .builtin.discovery_config.m_simpleEDP
      .use_PublicationReaderANDSubscriptionWriter = true;
  pqos.wire_protocol()
      .builtin.discovery_config.m_simpleEDP
      .use_PublicationWriterANDSubscriptionReader = true;
  pqos.wire_protocol().builtin.discovery_config.leaseDuration =
      eprosima::fastrtps::c_TimeInfinite;
  pqos.name(pqos_name);

  mp_participant =
      DomainParticipantFactory::get_instance()->create_participant(0, pqos);
}

bool DDSRouter::init() {
  // CREATE THE PARTICIPANT
  std::string pqos_name = std::string("robbin_RTPSParticipant_") + service_name;
  create_participant(pqos_name);
  if (mp_participant == nullptr) {
    return false;
  }

  // REGISTER TYPES
  mp_resultdatatype.register_type(mp_participant);
  mp_operationdatatype.register_type(mp_participant);

  // CREATE THE PUBLISHER
  mp_result_pub = mp_participant->create_publisher(PUBLISHER_QOS_DEFAULT);
  if (mp_result_pub == nullptr) {
    return false;
  }

  // CREATE THE RESULT TOPIC
  std::string mp_result_topic_name = service_name + std::string("_Result");
  mp_result_topic = mp_participant->create_topic(mp_result_topic_name, "Result",
                                                 TOPIC_QOS_DEFAULT);
  if (mp_result_topic == nullptr) {
    return false;
  }

  // CREATE THE DATAWRITER ON RESULT TOPIC
  DataWriterQos wqos = create_dataWriterQos();
  mp_result_writer = mp_result_pub->create_datawriter(mp_result_topic, wqos,
                                                      &m_resultsListener);
  if (mp_result_writer == nullptr) {
    return false;
  }

  // CREATE THE SUBSCRIBER
  mp_operation_sub = mp_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
  if (mp_operation_sub == nullptr) {
    return false;
  }

  // CREATE THE PARAM TOPIC
  std::string mp_operation_topic_name = service_name + std::string("_Param");
  mp_operation_topic = mp_participant->create_topic(
      mp_operation_topic_name, "Operation", TOPIC_QOS_DEFAULT);
  if (mp_operation_topic == nullptr) {
    return false;
  }

  // CREATE THE DATAREADER ON PARAM TOPIC
  DataReaderQos rqos = create_dataReaderQos();
  mp_operation_reader = mp_operation_sub->create_datareader(
      mp_operation_topic, rqos, &m_operationsListener);
  if (mp_operation_reader == nullptr) {
    return false;
  }

  return true;
}

bool DDSRouter::first_add_server(
    std::string server_guid, eprosima::fastdds::dds::DataReader *&result_reader,
    eprosima::fastdds::dds::DataWriter *&operation_writer,
    eprosima::fastdds::dds::Topic *&operation_topic,
    eprosima::fastdds::dds::Topic *&result_topic) {
  // CREATE THE TOPIC
  std::string mp_operation_topic_server_name =
      service_name + std::string("_Param_") + server_guid;
  operation_topic = mp_participant->create_topic(
      mp_operation_topic_server_name, "Operation", TOPIC_QOS_DEFAULT);
  if (operation_topic == nullptr) {
    return false;
  }

  // CREATE THE DATAWRITER
  DataWriterQos wqos = create_dataWriterQos();
  OperationServerListener *listener = new OperationServerListener(this);
  m_operationsServerListenerList.push_back(listener);
  operation_writer =
      mp_result_pub->create_datawriter(operation_topic, wqos, listener);
  if (operation_writer == nullptr) {
    return false;
  }

  // CREATE THE TOPIC
  std::string mp_result_topic_server_name =
      service_name + std::string("_Result_") + server_guid;
  result_topic = mp_participant->create_topic(mp_result_topic_server_name,
                                              "Result", TOPIC_QOS_DEFAULT);
  if (result_topic == nullptr) {
    return false;
  }

  // CREATE THE DATAREADER
  DataReaderQos rqos = create_dataReaderQos();
  ResultServerListener *temp_resultsServerListener =
      new ResultServerListener(this);
  m_resultsServerListenerList.push_back(temp_resultsServerListener);
  result_reader = mp_operation_sub->create_datareader(
      result_topic, rqos, temp_resultsServerListener);
  if (result_reader == nullptr) {
    return false;
  }
  return true;
}

bool DDSRouter::non_first_add_server(
    std::string server_guid, eprosima::fastdds::dds::DataReader *&result_reader,
    eprosima::fastdds::dds::DataWriter *&operation_writer,
    eprosima::fastdds::dds::Topic *&operation_topic,
    eprosima::fastdds::dds::Topic *&result_topic) {
  // CREATE THE PARTICIPANT
  std::string pqos_name = std::string("server_RTPSParticipant_") + server_guid;
  create_participant(pqos_name);
  if (mp_participant == nullptr) {
    return false;
  }

  // REGISTER TYPES
  mp_resultdatatype.register_type(mp_participant);
  mp_operationdatatype.register_type(mp_participant);

  // CREATE THE PUBLISHER
  eprosima::fastdds::dds::Publisher *mp_operation_pub =
      mp_participant->create_publisher(PUBLISHER_QOS_DEFAULT);
  if (mp_operation_pub == nullptr) {
    return false;
  }

  // CREATE THE TOPIC
  std::string mp_operation_topic_server_name =
      service_name + std::string("_Param_") + server_guid;
  operation_topic = mp_participant->create_topic(
      mp_operation_topic_server_name, "Operation", TOPIC_QOS_DEFAULT);
  if (operation_topic == nullptr) {
    return false;
  }

  // CREATE THE DATAWRITER
  DataWriterQos wqos = create_dataWriterQos();
  OperationServerListener *listener = new OperationServerListener(this);
  m_operationsServerListenerList.push_back(listener);
  operation_writer =
      mp_operation_pub->create_datawriter(operation_topic, wqos, listener);
  if (operation_writer == nullptr) {
    return false;
  }

  // CREATE THE SUBSCRIBER
  eprosima::fastdds::dds::Subscriber *mp_result_sub =
      mp_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
  if (mp_result_sub == nullptr) {
    return false;
  }

  // CREATE THE TOPIC
  std::string mp_result_topic_server_name =
      service_name + std::string("_Result_") + server_guid;
  result_topic = mp_participant->create_topic(mp_result_topic_server_name,
                                              "Result", TOPIC_QOS_DEFAULT);
  if (result_topic == nullptr) {
    return false;
  }

  // CREATE THE DATAREADER
  DataReaderQos rqos = create_dataReaderQos();
  ResultServerListener *temp_resultsServerListener =
      new ResultServerListener(this);
  m_resultsServerListenerList.push_back(temp_resultsServerListener);
  result_reader = mp_result_sub->create_datareader(result_topic, rqos,
                                                   temp_resultsServerListener);
  if (result_reader == nullptr) {
    return false;
  }
  return true;
}

bool DDSRouter::add_server(std::string server_guid) {
  // The following parameters are used to add server for this service
  eprosima::fastdds::dds::DataReader *result_reader;
  eprosima::fastdds::dds::DataWriter *operation_writer;
  eprosima::fastdds::dds::Topic *operation_topic;
  eprosima::fastdds::dds::Topic *result_topic;

  if (call_times == ONE) {
    if (!first_add_server(server_guid, result_reader, operation_writer,
                          operation_topic, result_topic)) {
      return false;
    }
  } else if (call_times >= TWO) {
    if (!non_first_add_server(server_guid, result_reader, operation_writer,
                              operation_topic, result_topic)) {
      return false;
    }
  } else {
    return false;
  }

  mp_operation_topic_server_list.push_back(operation_topic);
  mp_result_topic_server_list.push_back(result_topic);
  mp_result_reader_server_list.push_back(result_reader);
  mp_operation_writer_server_list.push_back(operation_writer);
  server_status_list.push_back(1);
  guid_server_list.push_back(server_guid);
  server_num++;

  return true;
}

void DDSRouter::adjust_index() {
  if (index >= server_num) {
    index = 0;
  }
  bool found_server = false;
  int count = 0;
  while (!found_server) {
    if (count == server_num) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(CALL_SERVER_TIMEOUT));
      count = 0;
    }
    if (server_status_list[index] != 1) {
      index = (index + 1) % server_num;
    } else {
      found_server = true;
    }

    count++;
  }
}

bool DDSRouter::call_server(std::vector<char> &param, std::vector<char> &result,
                            int &enclave_id) {
  // enclave id is similar to process id
  if (enclave_id > 0) {
    // use specific server
    index = enclave_id_to_server_index[enclave_id];
  } else {
    // find a server with round-robin algorithm
    adjust_index();
  }

  SampleInfo m_sampleInfo_server;
  clientserver::Operation m_operation_server;
  clientserver::Result m_result_server;
  m_operation_server.m_type = NORMAL_MESSAGE;
  m_operation_server.m_vector = param;
  m_operation_server.m_vector_size = param.size();
  if (enclave_id == 0) {
    m_operation_server.m_enclave_id = next_enclave_id;
    enclave_id_to_server_index[next_enclave_id] = index;
    ++next_enclave_id;
  } else {
    m_operation_server.m_enclave_id = enclave_id;
  }
  enclave_id = m_operation_server.m_enclave_id;
  std::cout << "ENCLAVE ID: " << enclave_id << std::endl;
  mp_operation_writer_server_list[index]->write((char *)&m_operation_server);
  do {
    m_result_server.m_guid = c_Guid_Unknown;
    mp_result_reader_server_list[index]->wait_for_unread_message(
        {RETRY_COUNT, 0});
    mp_result_reader_server_list[index]->take_next_sample(
        (char *)&m_result_server, &m_sampleInfo_server);
  } while (m_sampleInfo_server.instance_state !=
               eprosima::fastdds::dds::ALIVE_INSTANCE_STATE ||
           m_result_server.m_guid != m_operation_server.m_guid);

  result = m_result_server.m_vector;
  index++;
  /* if (m_operation_server.m_enclave_id > 0) { */
  /*   result[0] = enclave_id; */
  /*   result[ONE] = 0; */
  /*   result[TWO] = 0; */
  /*   result[THREE] = 0; */
  /* } */
  return true;
}
