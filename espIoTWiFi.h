#ifndef ESPIOTWIFI_H
#define ESPIOTWIFI_H

// Подключение библиотек 
    #include <pgmspace.h>
    #include <Arduino.h>
    #ifdef ESP32
        #include "WiFi.h"
        #include <ESPmDNS.h>
        // #include "WiFiGeneric.h"
    #elif defined(ESP8266)
        #include <ESP8266WiFi.h>
        #include <ESP8266mDNS.h>
        // #include <ESP8266WiFiGeneric.h>
    #endif

// Строковые константы хнанимые на флеше
    const char* const defSSID PROGMEM = "G.A.S_"; // Префикс имени точки доступа по умолчанию
    const char* const defPassword PROGMEM = "P@$$w0rd"; // Пароль точки доступа по умолчанию

// // Глобальные констатны
    const uint32_t wifi_timeout = 60000; // 1 min. for normal ... (setupMode == SLEEP_MODE)? 40000: 20 sec. for sleepmode and 
    const uint32_t wifi_timeout_long = 300000; // 5 min.


/**
 * Структура для передачи и хранения установчных данных для подключения по WiFi
 */
struct configWiFi{
    WiFiMode_t wifiMode;
    wl_status_t WiFiStatus;
    String ssid; // Имя сети или точки доступа
    String password; // Пароль сети
    String mDNS; // mDNS домен
    String ipaddr; //адрес
    String netmask; //Подседка
    String gateway; //Основной шлюз
    String dns1; //Основной DNS
    String dns2; //Алтернативный DNS
    configWiFi(){
        wifiMode = WIFI_MODE_AP;
        ssid = "";
        password = "";
        mDNS = "";
        ipaddr = "";
        netmask = "";
        gateway = "";
        dns2 = "";
        dns1 = "";
    }
}; 

/**
 * class of IoT connection to WiFi like station or AP mode
 */
class espIoTWiFi{
public:
    espIoTWiFi();
    ~espIoTWiFi();
    void begin(Print* __log = NULL); // Метод должен быть вызван из функции setup() скетча предназначен для старта 
    void begin(const configWiFi & _wifiConf, Print* __log = NULL); // Метод должен быть вызван из функции setup() скетча предназначен старта поключение к инфраструктуре
    void loop(); // Метод должен быть вызван из функции loop() скетча
    String getBoardId(); // Строковый идентификатор модуля (MAC адрес процессора)    

private:
    
    bool startWiFiAsStation(); // Настройка модуля в режиме инфраструктуры
    void startWiFiAsAP(); // Настройка модуля в режиме точки доступа
    void startWiFi(); // Попытка настройки модуля в заданный параметрами режим, при неудаче принудительный переход в режим точки доступа

    Print* _log; // Логи скетча
    bool _apMode; // Режим точки доступа (true) или инфраструктуры (false)
    configWiFi _wifiConfig; //конфигурация подключения
    bool _isIPStatic; // Режим получени адреса динамич (false) или статическая (true)

};

extern espIoTWiFi wifiConnection;


#endif