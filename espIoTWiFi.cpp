#include "espIoTWiFi.h"
#include <ESPmDNS.h>


/**
 * Конструктор класса espIoTWifi
 */
espIoTWiFi::espIoTWiFi(){
    _apMode = true; // Режим точки доступа (true) или инфраструктуры (false)
    _isIPStatic = false; // Режим получени адреса динамич (false) или статическая (true)
}

espIoTWiFi::~espIoTWiFi(){
}

/**
 * Предустановка для старта в режиме точки доступа
 * Метод должен быть вызван из функции setup()
 */
void espIoTWiFi::begin(Print* __log){ 
    _log = __log; // Логи скетча
    _apMode = true; // Режим точки доступа (true) или инфраструктуры (false)
    _isIPStatic = false; // Режим получени адреса динамич (false) или статическая (true)
    _wifiConfig.ssid = FPSTR(defSSID);
    _wifiConfig.ssid += getBoardId();
    _wifiConfig.password = FPSTR(defPassword);
    _wifiConfig.wifiMode = WIFI_MODE_AP;
    startWiFiAsAP();
}

/**
 * Предустановка для старта с выбором режима WiFi
 * Метод должен быть вызван из функции setup()
 * @param _wifiConf конфигурационные данные
 */
void espIoTWiFi::begin(const configWiFi & _wifiConf, Print* __log){
    _log = __log; // Логи скетча
    _wifiConfig = _wifiConf;
    _isIPStatic = false;
    _apMode = _wifiConfig.wifiMode == WIFI_MODE_AP;
    startWiFi();
}

/**
 * Метод должен быть вызван из функции loop() скетча
 * Предназначен для циклической обработки работы класса
 */
void espIoTWiFi::loop(){
    const uint32_t timeout = 300000; // 5 min.
    static uint32_t nextTime = timeout;

    if ((!_apMode) && (WiFi.status() != WL_CONNECTED) && ((WiFi.getMode() == WIFI_STA) || ((int32_t)(millis() - nextTime) >= 0))) {
        startWiFi();
        nextTime = millis() + timeout;
    }

    yield(); // For WiFi maintenance
}

/**
 * Формирование и получение уникального идентификатора процессора.
 * @returns уникальный идентификатор класса
 */
String espIoTWiFi::getBoardId() {
    uint64_t chipID = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).;
    String result = String((uint16_t)(chipID>>32), HEX);//High 2 bytes
    result += String((uint32_t)(chipID), HEX);//Low 4bytes.
    result.toUpperCase();
    return result;
}

/**
 * Настройка модуля в режиме инфраструктуры
 * @return true если подключение прошло успешно и false если с ошибкой
 */
bool espIoTWiFi::startWiFiAsStation(){
    uint32_t maxTime = millis() + wifi_timeout;//создаем метку для расчета времени неподключения 
    if (! _wifiConfig.ssid.length()) {//Проверка условия наличия идентификатора wifi сети если идентификатор отсутствует функция завершается с ошибкой
        if(_log != NULL)
            _log->println(F("Empty SSID!"));
        return false;
    }
    if(_log != NULL){
        _log->print(F("Connecting to \""));
        _log->print(_wifiConfig.ssid);
        _log->print("\"");
    }
    
    WiFi.mode(WIFI_STA); //Установка режима
    
    if(_isIPStatic){//если есть требование на установку статического IP адреса
        IPAddress _ip;
        IPAddress _nm;
        IPAddress _gw;
        IPAddress _dn1;
        IPAddress _dn2;
        if(_ip.fromString(_wifiConfig.ipaddr) && _nm.fromString(_wifiConfig.netmask) && _gw.fromString(_wifiConfig.gateway)){
            if(_dn1.fromString(_wifiConfig.dns1)){
                if(_dn2.fromString(_wifiConfig.dns2))
                    WiFi.config(_ip, _gw, _nm, _dn1, _dn2);
                else
                    WiFi.config(_ip, _gw, _nm, _dn1);
            } else
                WiFi.config(_ip, _gw, _nm);
        }
    }
    
    WiFi.begin(_wifiConfig.ssid.c_str(), _wifiConfig.password.c_str());

    if(_log != NULL)
        _log->println(" Current status: "+String(WiFi.status()));
    
    while (WiFi.status() != WL_CONNECTED){
        delay(500);
        if(_log != NULL)
            _log->print(".");
        if ((int32_t)(millis() - maxTime) >= 0) {
            if(_log != NULL)
                _log->println(F(" FAIL!"));
            return false;
        }
    }

    if(_log != NULL){
        _log->print(" ");
        _log->println(WiFi.localIP());
    }

    return true;
}

/**
 * Настройка модуля в режиме точки доступа
 */
void espIoTWiFi::startWiFiAsAP(){
    String ssid, password;

    if (_apMode) {
        ssid = _wifiConfig.ssid;
        password = _wifiConfig.password;
    } else {
        ssid = FPSTR(defSSID);
        ssid += getBoardId();
        password = FPSTR(defPassword);
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str(), password.c_str());
    if(_log != NULL){
        _log->print(F("Configuring access point \""));
        _log->print(ssid);
        _log->print(F("\" with password \""));
        _log->print(password);
        _log->print(F("\" on IP address "));
        _log->println(WiFi.softAPIP());
    }
}

/**
 *  Попытка настройки модуля в заданный параметрами режим, 
 * при неудаче принудительный переход в режим точки доступа
 */
void espIoTWiFi::startWiFi(){
    if (_apMode || (! startWiFiAsStation()))
        startWiFiAsAP();

    curWiFiStatus = WiFi.status();
    curWifiMode = WiFi.getMode();

    if (_wifiConfig.mDNS.length()) {
        if (MDNS.begin(_wifiConfig.mDNS.c_str())) {
            MDNS.addService("http", "tcp", 80);
            _log->println(F("mDNS responder started"));
        } else {
            _log->println(F("Error setting up mDNS responder!"));
        }
    }
}