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
#ifndef DSOFTBUSCOMMON_H
#define DSOFTBUSCOMMON_H

#include "Serialization.h"
#include "defaults.h"
#include "discovery_service.h"
#include "session.h"
#include "softbus_bus_center.h"
#include "softbus_error_code.h"
#include "string.h"

#define PACKAGE_NAME "softbus_sample"
#define LOCAL_SESSION_NAME "session_test"
#define TARGET_SESSION_NAME "session_test"
#define DEFAULT_SESSION_GROUP "group_test"
#define DEFAULT_PUBLISH_ID 123
#define DEFAULT_CAPABILITY "dvKit"

int SessionOpened(int sessionId, int result);

void SessionClosed(int sessionId);

void ByteReceived(int sessionId, const char *data, unsigned int dataLen);

void MessageReceived(int sessionId, const char *data, unsigned int dataLen);

int CreateSessionServerInterface(void);

void PublishSuccess(int publishId);

void PublishFailed(int publishId, PublishFailReason reason);

int PublishServiceInterface(void);

void UnPublishServiceInterface(void);

void DeviceFound(const DeviceInfo *device);

void DiscoverySuccess(int subscribeId);

void DiscoveryFailed(int subscribeId, DiscoveryFailReason reason);

int DiscoveryInterface(void);

void StopDiscoveryInterface(void);

void RemoveSessionServerInterface(void);

int OpenSessionInterface(const char *peerNetworkId);

void CloseSessionInterface(int sessionId);

int GetAllNodeDeviceInfoInterface(NodeBasicInfo **dev);

void FreeNodeInfoInterface(NodeBasicInfo *dev);

void SendBytesInterface(const char *data, int dataLen);

#endif