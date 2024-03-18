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

#include "ClientServerTypes.h"

using namespace clientserver;
using namespace eprosima::fastrtps::rtps;

bool OperationDataType::serialize(void *data, SerializedPayload_t *payload)
{
    Operation *op = (Operation *)data;
    payload->length = this->m_typeSize;
    uint32_t pos = 0;
    memcpy_s(payload->data, GUID_PREFIX_SIZE, op->m_guid.guidPrefix.value,
             GUID_PREFIX_SIZE);
    pos += GUID_PREFIX_SIZE;
    memcpy_s(payload->data + pos, GUID_ENTITYID_SIZE, op->m_guid.entityId.value,
             GUID_ENTITYID_SIZE);
    pos += GUID_ENTITYID_SIZE;
    *(int *)(payload->data + pos) = op->m_type;
    pos += sizeof(int);
    *(int *)(payload->data + pos) = op->m_vector_size;
    pos += sizeof(int);
    *(int *)(payload->data + pos) = op->m_enclave_id;
    pos += sizeof(int);
    // logic for fragmentation
    *(int *)(payload->data + pos) = op->fragment_idx;
    pos += sizeof(int);
    *(int *)(payload->data + pos) = op->total_fragment;
    pos += sizeof(int);
    for (int i = 0; i < op->m_vector_size; i++) {
        *(char *)(payload->data + pos) = op->m_vector[i];
        pos += sizeof(char);
    }
    return true;
}

bool OperationDataType::deserialize(SerializedPayload_t *payload, void *data)
{
    Operation *op = (Operation *)data;
    uint32_t pos = 0;
    memcpy_s(op->m_guid.guidPrefix.value, GUID_PREFIX_SIZE, payload->data,
             GUID_PREFIX_SIZE);
    pos += GUID_PREFIX_SIZE;
    memcpy_s(op->m_guid.entityId.value, GUID_ENTITYID_SIZE, payload->data + pos,
             GUID_ENTITYID_SIZE);
    pos += GUID_ENTITYID_SIZE;
    op->m_type = *(int *)(payload->data + pos);
    pos += sizeof(int);
    op->m_vector_size = *(int *)(payload->data + pos);
    pos += sizeof(int);
    op->m_enclave_id = *(int *)(payload->data + pos);
    pos += sizeof(int);
    // logic for fragmentation
    op->fragment_idx = *(int *)(payload->data + pos);
    pos += sizeof(int);
    op->total_fragment = *(int *)(payload->data + pos);
    pos += sizeof(int);
    for (int i = 0; i < op->m_vector_size; i++) {
        op->m_vector.push_back(*(char *)(payload->data + pos));
        pos += sizeof(char);
    }
    return true;
}

std::function<uint32_t()> OperationDataType::getSerializedSizeProvider(void *)
{
    return []() -> uint32_t {
        return GUID_SIZE + THREE * sizeof(int) +
               MAX_TRANSFER_VECTOR_SIZE * sizeof(char);
    };
}

void *OperationDataType::createData()
{
    return (void *)new Operation();
}

void OperationDataType::deleteData(void *data)
{
    delete ((Operation *)data);
}

bool ResultDataType::serialize(void *data, SerializedPayload_t *payload)
{
    Result *res = (Result *)data;
    payload->length = this->m_typeSize;
    uint32_t pos = 0;
    memcpy_s(payload->data, GUID_PREFIX_SIZE, res->m_guid.guidPrefix.value,
             GUID_PREFIX_SIZE);
    pos += GUID_PREFIX_SIZE;
    memcpy_s(payload->data + pos, GUID_ENTITYID_SIZE,
             res->m_guid.entityId.value, GUID_ENTITYID_SIZE);
    pos += GUID_ENTITYID_SIZE;
    *(int *)(payload->data + pos) = res->m_type;
    pos += sizeof(int);
    *(int *)(payload->data + pos) = res->m_vector_size;
    pos += sizeof(int);
    *(int *)(payload->data + pos) = res->m_enclave_id;
    pos += sizeof(int);
    // logic for fragmentation
    *(int *)(payload->data + pos) = res->ack_idx;
    pos += sizeof(int);
    for (int i = 0; i < res->m_vector_size; i++) {
        *(char *)(payload->data + pos) = res->m_vector[i];
        pos += sizeof(char);
    }
    return true;
}

bool ResultDataType::deserialize(SerializedPayload_t *payload, void *data)
{
    Result *res = (Result *)data;
    uint32_t pos = 0;
    memcpy_s(res->m_guid.guidPrefix.value, GUID_PREFIX_SIZE, payload->data,
             GUID_PREFIX_SIZE);
    pos += GUID_PREFIX_SIZE;
    memcpy_s(res->m_guid.entityId.value, GUID_ENTITYID_SIZE,
             payload->data + pos, GUID_ENTITYID_SIZE);
    pos += GUID_ENTITYID_SIZE;
    res->m_type = *(int *)(payload->data + pos);
    pos += sizeof(int);
    res->m_vector_size = *(int *)(payload->data + pos);
    pos += sizeof(int);
    res->m_enclave_id = *(int *)(payload->data + pos);
    pos += sizeof(int);
    // logic for fragmentation
    res->ack_idx = *(int *)(payload->data + pos);
    pos += sizeof(int);
    for (int i = 0; i < res->m_vector_size; i++) {
        res->m_vector.push_back(*(char *)(payload->data + pos));
        pos += sizeof(char);
    }
    return true;
}

std::function<uint32_t()> ResultDataType::getSerializedSizeProvider(void *)
{
    return []() -> uint32_t {
        return GUID_SIZE + THREE * sizeof(int) +
               MAX_TRANSFER_VECTOR_SIZE * sizeof(char);
    };
}

void *ResultDataType::createData()
{
    return (void *)new Result();
}

void ResultDataType::deleteData(void *data)
{
    delete ((Result *)data);
}
