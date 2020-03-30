#ifndef UDP_CONVERSATION_H
#define UDP_CONVERSATION_H
#include <Arduino.h>
#include "constCollection.h"
#include "espIoTWiFi.h"


#define DEFAULT_UDP_PORT 43152

void udpSetupNotifyer(Print* _ptrLog = NULL);
void udpSetupServer(bool (*_pacreUDPPackege)(const String & pakage) = NULL, Print* _ptrLog = NULL);
void udploop();
void udpScanDevicesInLAN();
void pushToRemoteDeviceList(const IPAddress& address, bool _needSend = false);
bool udpPublishToAllDevices(const String & stringData);


#endif