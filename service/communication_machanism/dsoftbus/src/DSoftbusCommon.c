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
#include "DSoftbusCommon.h"

int SessionOpened(int sessionId, int result)
{
    printf("<SessionOpened> sessionId: %d, result: %d\n", sessionId, result);
    return result;
}

void SessionClosed(int sessionId)
{
    printf("<SessionClosed> sessionId: %d\n", sessionId);
}

void ByteReceived(int sessionId, const char *data, unsigned int dataLen)
{
    printf("<ByteReceived> sessionId: %d, dataLen: %u\n", sessionId, dataLen);
}

void MessageReceived(int sessionId, const char *data, unsigned int dataLen)
{
    printf("<MessageReceived> sessionId: %d, dataLen: %u\n", sessionId,
           dataLen);
}

int CreateSessionServerInterface(void)
{
    const ISessionListener sessionCB = {
        .OnSessionOpened = SessionOpened,
        .OnSessionClosed = SessionClosed,
        .OnBytesReceived = ByteReceived,
        .OnMessageReceived = MessageReceived,
    };

    return CreateSessionServer(PACKAGE_NAME, LOCAL_SESSION_NAME, &sessionCB);
}

void PublishSuccess(int publishId)
{
    printf("<PublishSuccess> publishId: %d\n", publishId);
}

void PublishFailed(int publishId, PublishFailReason reason)
{
    printf("<PublishFailed> publishId: %d, reason: %d\n", publishId,
           (int)reason);
}

int PublishServiceInterface(void)
{
    PublishInfo info = {.publishId = DEFAULT_PUBLISH_ID,
                        .mode = DISCOVER_MODE_PASSIVE,
                        .medium = COAP,
                        .freq = MID,
                        .capability = DEFAULT_CAPABILITY,
                        .capabilityData = (unsigned char *)"capdata4",
                        .dataLen = strlen("capdata4")};
    IPublishCallback cb = {.OnPublishSuccess = PublishSuccess,
                           .OnPublishFail = PublishFailed};
    return PublishService(PACKAGE_NAME, &info, &cb);
}

void UnPublishServiceInterface(void)
{
    int ret = UnPublishService(PACKAGE_NAME, DEFAULT_PUBLISH_ID);
    if (ret != 0) {
        printf("UnPublishService fail: %d\n", ret);
    }
}

void DeviceFound(const DeviceInfo *device)
{
    unsigned int i;
    printf("<DeviceFound>: Device has found\n");
    printf("\tdevId=%s\n", device->devId);
    printf("\tdevName=%s\n", device->devName);
    printf("\tdevType=%d\n", device->devType);
    printf("\taddrNum=%d\n", device->addrNum);
    for (i = 0; i < device->addrNum; i++) {
        printf("\t\taddr%u:type=%d,", i + 1, device->addr[i].type);
        switch (device->addr[i].type) {
            case CONNECTION_ADDR_WLAN:
            case CONNECTION_ADDR_ETH:
                printf("ip=%s,port=%d,", device->addr[i].info.ip.ip,
                       device->addr[i].info.ip.port);
                break;
            default:
                break;
        }
        printf("peerUid=%s\n", device->addr[i].peerUid);
    }
    printf("\tcapabilityBitmapNum=%d\n", device->capabilityBitmapNum);
    for (i = 0; i < device->addrNum; i++) {
        printf("\t\tcapabilityBitmap[%u]=0x%x\n", i + 1,
               device->capabilityBitmap[i]);
    }
    printf("\tcustData=%s\n", device->custData);
}

void DiscoverySuccess(int subscribeId)
{
    printf("<DiscoverySuccess>: discover subscribeId=%d\n", subscribeId);
}

void DiscoveryFailed(int subscribeId, DiscoveryFailReason reason)
{
    printf("<DiscoveryFailed>: discover subscribeId=%d failed, reason=%d\n",
           subscribeId, (int)reason);
}

int DiscoveryInterface(void)
{
    SubscribeInfo info = {.subscribeId = DEFAULT_PUBLISH_ID,
                          .mode = DISCOVER_MODE_ACTIVE,
                          .medium = COAP,
                          .freq = MID,
                          .isSameAccount = false,
                          .isWakeRemote = false,
                          .capability = DEFAULT_CAPABILITY,
                          .capabilityData = nullptr,
                          .dataLen = 0};
    IDiscoveryCallback cb = {.OnDeviceFound = DeviceFound,
                             .OnDiscoverFailed = DiscoveryFailed,
                             .OnDiscoverySuccess = DiscoverySuccess};
    return StartDiscovery(PACKAGE_NAME, &info, &cb);
}

void StopDiscoveryInterface(void)
{
    int ret = StopDiscovery(PACKAGE_NAME, DEFAULT_PUBLISH_ID);
    if (ret) {
        printf("StopDiscovery fail:%d\n", ret);
    }
}

void RemoveSessionServerInterface(void)
{
    int ret = RemoveSessionServer(PACKAGE_NAME, LOCAL_SESSION_NAME);
    if (ret) {
        printf("RemoveSessionServer fail:%d\n", ret);
    }
}

int OpenSessionInterface(const char *peerNetworkId)
{
    SessionAttribute attr = {
        .dataType = TYPE_BYTES,
        .linkTypeNum = 1,
        .linkType[0] = LINK_TYPE_WIFI_WLAN_2G,
        .attr = {RAW_STREAM},
    };

    return OpenSession(LOCAL_SESSION_NAME, TARGET_SESSION_NAME, peerNetworkId,
                       DEFAULT_SESSION_GROUP, &attr);
}

void CloseSessionInterface(int sessionId)
{
    CloseSession(sessionId);
}

int GetAllNodeDeviceInfoInterface(NodeBasicInfo **dev)
{
    int ret, num;

    ret = GetAllNodeDeviceInfo(PACKAGE_NAME, dev, &num);
    if (ret) {
        printf("GetAllNodeDeviceInfo fail:%d\n", ret);
        return -1;
    }

    printf("<GetAllNodeDeviceInfo>return %d Node\n", num);
    for (int i = 0; i < num; i++) {
        printf("<num %d>deviceName=%s\n", i + 1, dev[i]->deviceName);
        printf("\tnetworkId=%s\n", dev[i]->networkId);
        printf("\tType=%d\n", dev[i]->deviceTypeId);
    }

    return num;
}

void FreeNodeInfoInterface(NodeBasicInfo *dev)
{
    FreeNodeInfo(dev);
}

void SendBytesInterface(const char *data, int dataLen)
{
    NodeBasicInfo *dev = nullptr;
    int dev_num, sessionId, ret;

    dev_num = GetAllNodeDeviceInfoInterface(&dev);
    if (dev_num <= 0) {
        return;
    }

    sessionId = OpenSessionInterface(dev[0].networkId);
    if (sessionId < 0) {
        printf("OpenSessionInterface fail, ret=%d\n", sessionId);
    }

    ret = SendBytes(sessionId, data, dataLen);
    if (ret) {
        printf("SendBytes fail:%d\n", ret);
    }

    CloseSessionInterface(sessionId);
    FreeNodeInfoInterface(dev);
}