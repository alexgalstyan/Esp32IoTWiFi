#include "udpConversation.h"
#include <AsyncUDP.h>
#include <pgmspace.h>


const char* const udpGASHello PROGMEM = "G.A.S_Hello"; // 
const char* const udpGASListener PROGMEM = "G.A.S_Listener"; // 

    /** RemoteDevice link store unit
     * @param address remote device IP address
     * @param needSend factor to approve 
     */
    struct UDPRemoteDevice{
        IPAddress address;
        AsyncUDP udpClient;
        bool needSend;
        bool udpPublish(const String & stringData);
    };

    /**
     * publish to this device
     * @param stringData string content for send to device
     * @return true if succesful sending
     */ 
    bool UDPRemoteDevice::udpPublish(const String & stringData){
        if(udpClient.connect(address, UDP_DEFAULT_PORT)){
            // udpClient.write((uint8_t *)stringData.c_str(), stringData.length());
            udpClient.print(stringData);
            udpClient.close();
            return true;
        }
        return false;
    };

    const uint32_t udpBroadcasScanPeriod = 40000;
    const uint32_t udpScanDelay = 1000;
    uint32_t udpScanTimer;
    uint32_t udpBroadcastScanTimer;
    bool udpNeedBroadcasScan = false;

    uint8_t deviceListCount = 0;
    UDPRemoteDevice * deviceList;

AsyncUDP udp;
Print * udpPtrLog;



    /**
     * @brief pointer to function for parcing pakage and assigne if it expected data
     * need write parce function in this body
     * @param pakage is received data by UDP and cheked with even byte
     * @return true if data expected and assigned
     */
    bool (*parceUDPPackege)(const String & pakage) = NULL;

/**
 * Receive response from recipients for conversation
 * @param packet received UDP packet container
 * @return true if package is good
 */
bool receiveUDPPacket(AsyncUDPPacket packet){
    if(udpPtrLog != NULL){
        String tempString = (char *)packet.data();
        #ifdef UDP_SERVER_ROLE
            if(tempString == udpGASHello){
                if(udpPtrLog != NULL){
                    udpPtrLog->print(F("Have got G.A.S_Hello From: "));
                    udpPtrLog->print(packet.remoteIP());
                    udpPtrLog->print(":");
                    udpPtrLog->println(packet.remotePort());
                }
                //reply to the client
                packet.print("G.A.S_Listener");
            } else {
                if(udpPtrLog != NULL)
                    udpPtrLog->println("receive:" + tempString);
                if(parceUDPPackege != NULL){
                    parceUDPPackege(tempString);
                }
            }
        #else
            if(tempString == udpGASListener){
                pushToRemoteDeviceList(packet.remoteIP(), true);
            }
        #endif
    }

    return true;
}


/**
 * Satrs a UDP server 
 * @return start status true if success
 */
bool startUdpServer(){
    if(udp.listen(UDP_DEFAULT_PORT)) {
        if(udpPtrLog != NULL){
            udpPtrLog->print("UDP Listening on IP: ");
            udpPtrLog->print(WiFi.localIP());
            udpPtrLog->print(F(":"));
            udpPtrLog->println(UDP_DEFAULT_PORT);
        }
        udp.onPacket(receiveUDPPacket);
    } else {
        if(udpPtrLog != NULL)
            udpPtrLog->println(F("Error starting UDP server!"));
        return false;
    }
    return true;
}

/**
 * sending pakage to the specified address
 * @param address recipient IP address
 * @param stringData pakage to send by UDP
 * @return a sending succes state
 */  
bool udpPublish(IPAddress address, const String & stringData){
    AsyncUDPMessage message;
    message.write((uint8_t *)stringData.c_str(), (size_t)stringData.length());
    if (udp.sendTo(message, address, UDP_DEFAULT_PORT)){
        if(udpPtrLog != NULL)
            udpPtrLog->println("Sent UDP T=" + stringData+" to IP=" + address.toString());
        return true;
    } else {
        if(udpPtrLog != NULL)
            udpPtrLog->println(F("Error sending command UDP packet!"));
        return false;
    }
}

    /**
     * sending pakage to all stored recipients (addresses)
     * @param stringData pakage to send by UDP
     * @return a sending succes state
     */  
    bool udpPublishToAllDevices(const String & stringData){
        if(deviceList != NULL){
            for(uint8_t i = 0; i < deviceListCount; i++){
                if(deviceList[i].needSend){
                    if (deviceList[i].udpPublish(stringData)){
                        if(udpPtrLog != NULL)
                            udpPtrLog->println("Sent UDP " + stringData+" to IP=" + deviceList[i].address.toString());
                        return true;
                    } else {
                        if(udpPtrLog != NULL)
                            udpPtrLog->println(F("Error sending command UDP packet!"));
                        return false;
                    }
                }
            }
            return true;
        }
        return false;
    }

/**
 * add specified adress to recipient device list
 * @param address recipient IP address,
 * @param _cheked mutex to use recipient in All publish function
 */
void pushToRemoteDeviceList(const IPAddress& address, bool _needSend){ //default value for _checked = false
    String logTextDeviceAdded = F(" added to remote device list");
    if(deviceList != NULL){
        bool b = false;
        for(uint8_t i = 0; i < deviceListCount; i++){
            b = deviceList[i].address == address;
        }
        if(!b){
            deviceListCount++;
            UDPRemoteDevice * tmpPtr = deviceList;
            deviceList = new UDPRemoteDevice [deviceListCount];
            for(uint8_t i = 0; i < deviceListCount-1; i++){
                deviceList[i] = tmpPtr[i];
            }
            deviceList[deviceListCount-1].address = address;
            deviceList[deviceListCount-1].needSend = _needSend;
            delete[] tmpPtr;
            if(udpPtrLog != NULL)
                udpPtrLog->println(address.toString() + logTextDeviceAdded);
        }
    } else {
        deviceListCount = 1;
        deviceList = new UDPRemoteDevice [deviceListCount];
        deviceList[0].address = address;
        deviceList[0].needSend = _needSend;
        if(udpPtrLog != NULL)
            udpPtrLog->println(address.toString() + logTextDeviceAdded);
    }
}

/**
 * start stcaning by broadcas for other device
 * function is asinchrone and will by start with loop() function
 */
void udpScanDevicesInLAN(){
    udpNeedBroadcasScan = true;
    udpScanTimer = millis() + udpScanDelay;
    udpBroadcastScanTimer = udpScanTimer + udpBroadcasScanPeriod - udpScanDelay;
    startUdpServer();
}

/**
 * Stopig scan with LAN broadcat
 */
void udpStopBroadcast(){
    udpNeedBroadcasScan = false;
    udp.close();
}
    
/**
 * Setting up UDP connection
 * @param _ptrConfing configuration object gets with const state
 * @param _udpPtrLog pointer to stream for loggig out, by default is NULL
 */
void udpSetupNotifyer(Print* _PtrLog){
    udpPtrLog = _PtrLog;
        startUdpServer();        
}

/**
 * Setting up UDP connection
 * @param _ptrConfing configuration object gets with const state
 * @param _parceUDPPackege pointer to the function that will be called upon receipt of the package
 * @param _udpPtrLog pointer to stream for loggig out, by default is NULL
 */
void udpSetupServer(bool (*_parceUDPPackege)(const String & pakage), Print* _PtrLog){
    udpPtrLog = _PtrLog;
    parceUDPPackege = _parceUDPPackege;
    startUdpServer();        
}


/**
 * Loop function, need to set in main loop()
 * Organize scanig by broadcast line to fine listener devices
 */
void udploop(){
        if(udpNeedBroadcasScan && udpScanTimer < millis()){
            if (udp.broadcast(udpGASHello)) //udp.sendTo(message,addr, UDP_DEFAULT_PORT)
                udpPtrLog->println(F("Sending UDP broadcast command "));
            else
                udpPtrLog->println(F("Error sending command UDP packet!"));

            if(udpBroadcastScanTimer < millis())
                udpStopBroadcast();
            else
                udpScanTimer += udpScanDelay;      
        }
} 

