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
#include "ClientServerTypes.h"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <thread>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastrtps::rtps;
using namespace clientserver;

DDSClient::DDSClient()
    : mp_operation_pub(nullptr), mp_result_sub(nullptr),
      mp_participant(nullptr), mp_resultdatatype(new ResultDataType()),
      mp_operationdatatype(new OperationDataType()),
      m_operationsListener(nullptr), m_resultsListener(nullptr),
      m_isReady(false), m_operationMatched(0), m_resultMatched(0) {
  m_operationsListener.mp_up = this;
  m_resultsListener.mp_up = this;
}

DDSClient::~DDSClient() {
  if (mp_operation_pub != nullptr && mp_operation_writer != nullptr) {
    mp_operation_pub->delete_datawriter(mp_operation_writer);
  }
  if (mp_participant != nullptr && mp_operation_pub != nullptr) {
    mp_participant->delete_publisher(mp_operation_pub);
  }
  if (mp_participant != nullptr && mp_operation_topic != nullptr) {
    mp_participant->delete_topic(mp_operation_topic);
  }
  if (mp_result_sub != nullptr && mp_result_reader != nullptr) {
    mp_result_sub->delete_datareader(mp_result_reader);
  }
  if (mp_participant != nullptr && mp_result_sub != nullptr) {
    mp_participant->delete_subscriber(mp_result_sub);
  }
  if (mp_participant != nullptr && mp_result_topic != nullptr) {
    mp_participant->delete_topic(mp_result_topic);
  }

  auto instance = DomainParticipantFactory::get_instance();
  if (instance) {
    instance->delete_participant(mp_participant);
  }
}

void DDSClient::create_participant(std::string pqos_name) {
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

static DataWriterQos create_dataWriterQos() {
  DataWriterQos wqos;
  wqos.history().kind = KEEP_LAST_HISTORY_QOS;
  wqos.history().depth = HISTORY_DEPTH;
  wqos.resource_limits().max_samples = MAX_SAMPLES;
  wqos.resource_limits().allocated_samples = ALLOC_SAMPLES;
  wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
  // wqos.publish_mode().kind =
  // eprosima::fastdds::dds::ASYNCHRONOUS_PUBLISH_MODE;
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

bool DDSClient::init(std::string service_name) {
  m_guid = get_random_guid();
  // CREATE THE PARTICIPANT
  std::string pqos_name = std::string("client_RTPSParticipant_") + m_guid;
  create_participant(pqos_name);
  if (mp_participant == nullptr) {
    return false;
  }

  // REGISTER TYPES
  mp_resultdatatype.register_type(mp_participant);
  mp_operationdatatype.register_type(mp_participant);

  // CREATE THE PUBLISHER
  mp_operation_pub = mp_participant->create_publisher(PUBLISHER_QOS_DEFAULT);
  if (mp_operation_pub == nullptr) {
    return false;
  }

  // CREATE THE PARAM TOPIC
  std::string mp_operation_topic_name = service_name + std::string("_Param");
  mp_operation_topic = mp_participant->create_topic(
      mp_operation_topic_name, "Operation", TOPIC_QOS_DEFAULT);
  if (mp_operation_topic == nullptr) {
    return false;
  }

  // CREATE THE DATAWRITER ON PARAM TOPIC
  DataWriterQos wqos = create_dataWriterQos();
  mp_operation_writer = mp_operation_pub->create_datawriter(
      mp_operation_topic, wqos, &this->m_operationsListener);
  if (mp_operation_writer == nullptr) {
    return false;
  }

  // CREATE THE SUBSCRIBER
  mp_result_sub = mp_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
  if (mp_result_sub == nullptr) {
    return false;
  }

  // CREATE THE RESULT TOPIC
  std::string mp_result_topic_name = service_name + std::string("_Result");
  mp_result_topic = mp_participant->create_topic(mp_result_topic_name, "Result",
                                                 TOPIC_QOS_DEFAULT);
  if (mp_result_topic == nullptr) {
    return false;
  }

  // CREATE THE DATAREADER ON RESULT TOPIC
  DataReaderQos rqos = create_dataReaderQos();
  mp_result_reader = mp_result_sub->create_datareader(mp_result_topic, rqos,
                                                      &this->m_resultsListener);
  if (mp_result_reader == nullptr) {
    return false;
  }

  return true;
}

Serialization *DDSClient::call_service(Serialization *param, int enclave_id) {
  SampleInfo m_sampleInfo;
  std::vector<char> param_to_send;

  for (int i = 0; i < param->size(); i++) {
    param_to_send.push_back(param->data()[i]);
  }

  auto randomize_guid = [](GUID_t &guid) {
    for (int i = 0; i < 12; i++) {
      guid.guidPrefix.value[i] = rand() % 256;
    }
    guid.entityId.value[3] = rand() % 256;
  };

  auto print_guid = [](GUID_t &guid) {
    printf("GUID: ");
    for (int i = 0; i < 12; i++) {
      printf("%02x", guid.guidPrefix.value[i]);
    }
    printf(":%d\n", guid.entityId.value[3]);
  };

  m_operation.m_type = DUMMY_MESSAGE;
  m_operation.m_enclave_id = enclave_id;
  printf("randomize_guid\n");
  randomize_guid(m_operation.m_guid);

  printf("BEGIN DUMMY WRITE\n");
  do {
    // TODO: this dummy write is necessary, otherwise subscriber can't receive
    // following messages. Don't know why for now.
    mp_operation_writer->write((char *)&m_operation);
    resetResult();
    mp_result_reader->wait_for_unread_message({1, 0});
    mp_result_reader->take_next_sample((char *)&m_result, &m_sampleInfo);

    printf("RESULT GUID: ");
    print_guid(m_result.m_guid);
    printf("EXPECTED GUID: ");
    print_guid(m_operation.m_guid);
  } while (m_sampleInfo.instance_state !=
               eprosima::fastdds::dds::ALIVE_INSTANCE_STATE &&
           m_result.m_guid != m_operation.m_guid);
  printf("END DUMMY WRITE\n");

  m_operation.m_type = NORMAL_MESSAGE;
  m_operation.m_vector = param_to_send;
  m_operation.m_vector_size = param_to_send.size();

  // use guid to identify the message
  randomize_guid(m_operation.m_guid);
  printf("WRITE (%luB) ENCLAVE ID: %d\n", m_operation.m_vector.size(),
         enclave_id);

  m_result.m_type = NORMAL_MESSAGE;
  do {
    mp_operation_writer->write((char *)&m_operation);
    resetResult();
    mp_result_reader->wait_for_unread_message({1, 0});
    mp_result_reader->take_next_sample((char *)&m_result, &m_sampleInfo);
  } while (m_result.m_type != DUMMY_MESSAGE);

  m_result.m_vector.clear();
  do {
    resetResult();
    std::cout << "wait_for_unread_message" << std::endl;
    mp_result_reader->wait_for_unread_message({RETRY_COUNT, 0});
    std::cout << "take_next_sample" << std::endl;
    mp_result_reader->take_next_sample((char *)&m_result, &m_sampleInfo);

    printf("RESULT GUID: ");
    print_guid(m_result.m_guid);
    printf("EXPECTED GUID: ");
    print_guid(m_operation.m_guid);
  } while (m_sampleInfo.instance_state !=
               eprosima::fastdds::dds::ALIVE_INSTANCE_STATE ||
           m_result.m_guid != m_operation.m_guid);

  std::cout << "READ DONE: ENCLAVE_ID = " << m_result.m_enclave_id << std::endl;
  Serialization *result = new Serialization(
      StreamBuffer(&m_result.m_vector[0], m_result.m_vector.size()));
  return result;
}

void DDSClient::resetResult() { m_result.m_guid = c_Guid_Unknown; }

void DDSClient::OperationListener::on_publication_matched(
    eprosima::fastdds::dds::DataWriter *,
    const eprosima::fastdds::dds::PublicationMatchedStatus &info) {
  if (info.current_count_change == 1) {
    std::cout << "MATCHED 1" << std::endl;
    mp_up->m_operationMatched++;
  } else if (info.current_count_change == -1) {
    std::cout << "UNMATCHED 1" << std::endl;
    mp_up->m_operationMatched--;
  } else {
    std::cout << info.current_count_change
              << " is not a valid value for PublicationMatchedStatus "
                 "current count change"
              << std::endl;
  }
  mp_up->isReady();
}

void DDSClient::ResultListener::on_subscription_matched(
    DataReader *, const SubscriptionMatchedStatus &info) {
  if (info.current_count_change == 1) {
    std::cout << "MATCHED 2" << std::endl;
    mp_up->m_resultMatched++;
  } else if (info.current_count_change == -1) {
    std::cout << "UNMATCHED 2" << std::endl;
    mp_up->m_resultMatched--;
  } else {
    std::cout << info.current_count_change
              << " is not a valid value for SubscriptionMatchedStatus "
                 "current count change"
              << std::endl;
  }
  mp_up->isReady();
}

void DDSClient::ResultListener::on_data_available(DataReader *) {}

bool DDSClient::isReady() {
  if (m_operationMatched >= 1 && m_resultMatched >= 1) {
    m_isReady = true;
  } else {
    m_isReady = false;
  }
  return m_isReady;
}
