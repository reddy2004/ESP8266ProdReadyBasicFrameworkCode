#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266httpUpdate.h>
#include <MqttClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

/* Serial port baud rate */
#define HW_UART_SPEED                9600L


volatile long ctrx = 0;
volatile int  debug_level = 10;
volatile  unsigned long   LoopCounterCurrentId = 0;

enum ESP8266ModuleType {
      esp01,
      nodemcu
};


/* 
* Enum defining all potential features in the code. This code is a stripped down
* version of all the below features I had implimented elsewhere.
*/
enum  ModuleFeature{
      esp01_alarm_pin,
      esp01_relay_pin,
      wifi_always_on_mqtt,
      wifi_always_on_http,
      wifi_on_activity_http,
      i2c,
      serial,
      pwm,
      radio_transmitter_433mhz, //use wifi_always_on_mqtt for better working
      radio_reciever_433mhz,    //use along with wifi_always_on_mqtt for best results
      analog_read,
      mqttClient,
      ota,
      ultrasonic,
      gyro,
      gpsglonas
} ;

/* Human readable text descriptions, of course for printing on screen! */
const char * const feature_str[] =
{
    [esp01_alarm_pin] = "esp01_alarm_pin",
    [esp01_relay_pin] = "esp01_relay_pin",
    [wifi_always_on_mqtt] = "wifi_always_on_mqtt",
    [wifi_always_on_http]  = "wifi_always_on_http",
    [wifi_on_activity_http]  = "wifi_on_activity_http",
    [i2c] = "i2c",
    [serial] = "serial",
    [pwm]  = "pwm",
    [radio_transmitter_433mhz]  = "radio_transmitter_433mhz",
    [radio_reciever_433mhz] = "radio_reciever_433mhz",
    [analog_read] = "analog_read",
    [mqttClient]  = "mqttClient",
    [ota]  = "ota",
    [ultrasonic] = "ultrasonic",
    [gyro] = "gyro",
    [gpsglonas]  = "gpsglonas"
};

/* 
 *  List of features that you want to enable on the module,
 *  plus the module type and firmware version
 */ 
static ModuleFeature nodumcuFeatures[] = {esp01_relay_pin, wifi_always_on_mqtt, ota, mqttClient};
static ModuleFeature esp01Features[] = {esp01_relay_pin, wifi_on_activity_http, ota};
int nodemcuFeatureCount = 4;
int esp01FeatureCount = 3;


ESP8266ModuleType thisModule = nodemcu;
const char* current_build = "nfox_linea" ;

/*
 * NTP Time
 */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
 
/*
 * All the dynamic runtime configuration of the chip is located here.
 */ 
DynamicJsonDocument jsonConfig(2048);

/*
 * This is the json that has outgoing (this chip to server) and
 * incoming (server->this chip) data.
 */
DynamicJsonDocument jsonOutgoing(512);
DynamicJsonDocument jsonIncomingHttp(512);
DynamicJsonDocument jsonIncomingMqtt(512); 


/*
 * Hardcoded values for some of chip functionality. Add bits of information that the chip
 * needs after boot to figure out where and how to connect and configure itself. 
 * moduleconfig.json file is located on the web with its link hardcoded here. It should
 * look something like this.
 * {
  "miniWebFiles": {
    "version" :  3,
    "body": "http://128.199.20.242:8080/downloadfilesforesp8266/body.txt",
    "logic": "http://128.199.20.242:8080/downloadfilesforesp8266/logic.js"
  },
  "firmWareupgrade": [
    {
      "current": "nfox_linea_11",
      "update": "nfox_linea_v1.00",
      "link": "http://github.com/reddy2004/files/firmware.bin",
      "desc": "http://github.com/reddy2004/files/firmware.bin"
    }
  ]
}
 */ 
String hardCodedSSID = "tempWifi";
String hardCodedPW = "tempPassword";
String hardCodedPermalink = "http://128.199.20.242:8080/downloadfilesforesp8266/moduleconfig.json";

/*
 * minute timers. Use like a watchdog. Just a placeholder for now.
 */
os_timer_t    myTimer2min, myTimer1min, myTimer10min;
volatile bool tickOccured2min = false, tickOccured1min = false, tickOccured10min = false;
volatile int  minutesElapsedSinceBoot = 0;
volatile bool WaitForUserWifiSetup = true;

/*
 * Wrapper around printf's to basically have a debug level. So that we can filter out less important
 * debug levels during development., i.e reduce clutter on serial port
 */
#define LOG_PRINTFLN(level, fmt, ...)  logfln(level, fmt, ##__VA_ARGS__)
#define LOG_SIZE_MAX 2048
void logfln(int level, const char *fmt, ...) {
  if (level < debug_level) {
      char buf[LOG_SIZE_MAX];
      va_list ap;
      va_start(ap, fmt);
      vsnprintf(buf, LOG_SIZE_MAX, fmt, ap);
      va_end(ap);
      Serial.println(buf);
      Serial.flush();
  }
}

void print_heap_usage(char *reason) {
    long  fh = ESP.getFreeHeap();
    LOG_PRINTFLN(1, "%ld (heap: %ld) %s", ctrx++ ,fh, reason);
}


void RestartModule()
{
    LOG_PRINTFLN(1, "Restarting ESP module");
    delay(1000);
    ESP.eraseConfig();
    ESP.restart();
}

/*
 * We know the module type, lets check if the feature is enabled here. We have two different module types and
 * different viable set of features. For ex. Its hard to interface, say GPS chip, on an ESP01, while it can be
 * done with relatively less effort on a nodemcu.
 * 
 * Remember, this code base is monolithic. Irrespecive of weather you want a feature or no, all the code is compiled
 * and the bin file size remains the same. Having limited set of functions will keep the code flow clutter free for
 * debugging. With limited feature sets, you can also control how certain things such as wifi connections behave.
 * Ex. If this chip is battery operated, you might want to enable wifi only periodically to conserve power.
 */
bool isFeatureSupported(ModuleFeature feature) {
  if (thisModule == esp01) {
      for (int i=0; i < esp01FeatureCount; i++) { 
          if (feature == esp01Features[i]) {
            return true;
          }
      }
      return false;
  } else if (thisModule == nodemcu) {
      for (int i=0; i < nodemcuFeatureCount; i++) {
          if (feature == nodumcuFeatures[i]) {
              return true;
          }
      }
      return false;    
  }
  return false;
}
