/*
 * MQTT routines go here.
 */
typedef struct MQTTConnData {
    char  clientid[32];
    char  username[32];
    char  password[32];
    char  host[128];
    int   port;
    char  subscribeChannel[64];
    char  publishChannel[64];
    bool isValid;
}MQTTConnectionData;

//put mqtt connection data into this struct, used globally and populated from jsonArray in a file.
//Keep memory footprint low.
MQTTConnectionData mcd;

static MqttClient *mqtt = NULL;
static WiFiClient network;
#define MQTT_LOG_ENABLED 1

typedef struct MQTTRecievedData {
  bool isValid;
  char MQTTPayload[256]; //512 byte payload only, as json
}MQTTRecievedData;

MQTTRecievedData mqttRCDData;

// ==== Object to supply system functions ===
class System: public MqttClient::System {
  public:
    unsigned long millis() const {
      return ::millis();
    }
    void yield(void) {
      ::yield();
    }
};


void UpdateMQTTConfig(String sub, String pub, String clientid, String username, String password, String host, int port) 
{   
    JsonObject mqtt = ((jsonConfig["MQTT"]).isNull() == false)? jsonConfig["MQTT"] : jsonConfig.createNestedObject("MQTT");
    mqtt["publish"] = pub;
    mqtt["subscribe"] = sub;
    mqtt["clientid"] = clientid;
    mqtt["username"] = username;
    mqtt["password"] = password;
    mqtt["server"] = host;
    mqtt["port"] = port;

    SaveJsonToFile(&LittleFS);
}

struct MQTTConnData* GetMQTTConnectionData() {
    if (mcd.isValid) {
      return &mcd;
    }

    if ((jsonConfig["MQTT"]).isNull()) {
        LOG_PRINTFLN(1, "MQTT data is empty. Aborting MQTT Client creation");
        mcd.isValid = false;
        return &mcd;
    }

    JsonObject mqtt = ((jsonConfig["MQTT"]).isNull() == false)? jsonConfig["MQTT"] : jsonConfig.createNestedObject("MQTT");

    sprintf(mcd.subscribeChannel,"%s",mqtt["subscribe"].as<char*>());
    sprintf(mcd.publishChannel,"%s",mqtt["publish"].as<char*>());
    sprintf(mcd.clientid,"%s",mqtt["clientid"].as<char*>());    
    sprintf(mcd.username,"%s",mqtt["username"].as<char*>());
    sprintf(mcd.password,"%s",mqtt["password"].as<char*>());
    sprintf(mcd.host,"%s",mqtt["server"].as<char*>());
    
    mcd.port = mqtt["port"].as<int>();
    mcd.isValid = true;
    return &mcd;
}

void SetupMQTTClientConfig()
{
    /*
     * This is specific to your device so you can hardcode it here. The config.json is created/updated during runtime
     * I dont feel a need to create a remove config/ miniWeb interface to update this!
     * (Maybe if you are flashing 1000s of devices then maybe you could do it via other methods.
     */
    if (isFeatureSupported(mqttClient)) {
        JsonObject wifiAPCreds = jsonConfig["APMode"];
        if (wifiAPCreds.isNull()) {
            LOG_PRINTFLN(1, "Access point creds not found. Aborting MQTT Client creation");
            return;
        }
        String apssid = wifiAPCreds["apssid"];
        String appassword = wifiAPCreds["appw"];
        char tofox[64];
        char fromfox[64];
        sprintf(tofox,"NFOXMQTT/%s/tofox",apssid.c_str());
        sprintf(fromfox,"NFOXMQTT/%s/fromfox",apssid.c_str());
        UpdateMQTTConfig((String)tofox, (String)fromfox, apssid, apssid, appassword, hardCodedServerIP.c_str(), 1883);
        struct MQTTConnData* mcd = GetMQTTConnectionData();
        LOG_PRINTFLN(1, "Created MQTT config data!!!");

        RestartModule();
    }  
}

void SetupMQTT(void)
{
      LOG_PRINTFLN(1, "Entering MQTT SetupMQTT()");
      if (!isFeatureSupported(mqttClient)) {
        return;
      }
    
      struct MQTTConnData* mcd = GetMQTTConnectionData();
      if (!mcd->isValid) {
          //Let set up the configs needed on this module : Hardcoded for now.
          SetupMQTTClientConfig();  
      }
      
      // Setup MqttClient
      MqttClient::System *mqttSystem = new System;
      MqttClient::Logger *mqttLogger = new MqttClient::LoggerImpl<HardwareSerial>(Serial);
      MqttClient::Network * mqttNetwork = new MqttClient::NetworkClientImpl<WiFiClient>(network, *mqttSystem);
      //// Make 128 bytes send buffer
      MqttClient::Buffer *mqttSendBuffer = new MqttClient::ArrayBuffer<2048>();
      //// Make 128 bytes receive buffer
      MqttClient::Buffer *mqttRecvBuffer = new MqttClient::ArrayBuffer<2048>();
      //// Allow up to 2 subscriptions simultaneously
      MqttClient::MessageHandlers *mqttMessageHandlers = new MqttClient::MessageHandlersImpl<2>();
      //// Configure client options
      MqttClient::Options mqttOptions;
      ////// Set command timeout to 100 seconds
      mqttOptions.commandTimeoutMs = 200000;
      //// Make client object
      mqtt = new MqttClient(
        mqttOptions, *mqttLogger, *mqttSystem, *mqttNetwork, *mqttSendBuffer,
        *mqttRecvBuffer, *mqttMessageHandlers
      );
      
      LOG_PRINTFLN(1, "MQTT client created!");
}

/*
 * Keep this callback as light as possible. Do not do too much work here
 * We shall just copy the payload to our array and return.
 */ 
void processMessage(MqttClient::MessageData& md) {

    const MqttClient::Message& msg = md.message;
    if (msg.payloadLen > 256 || mqttRCDData.isValid ) {
        return;  
    }
    memcpy(mqttRCDData.MQTTPayload, msg.payload, msg.payloadLen);
    mqttRCDData.isValid = true;

    mqttRCDData.MQTTPayload[msg.payloadLen] = '\0';
    //https://github.com/marvinroger/async-mqtt-client/issues/29
    //dont do any heavy lifting here or calling delay yeild etc.
    //LOG_PRINTFLN(1,
   //   "Message arrived: qos %d, retained %d, dup %d, packetid %d, payload len:[%d]",
    //  msg.qos, msg.retained, msg.dup, msg.id, msg.payloadLen
   //   );
}

/*
 * Caller should have already verfied that MQTT is enabled and configured correctly.
 * Or else, expect an exception!
 */
void sendMessageToTopic(MQTTConnData* mcd, char *topic, char *payload, int length)
{
    if (length == 0)
    {
        //No special cases
    }
    else 
    {
      MqttClient::Message message;
      message.qos = MqttClient::QOS0;
      message.retained = false;
      message.dup = false;
      message.payload = (void*) payload;
      message.payloadLen = length;
      mqtt->publish(topic, message);
    }
    // Idle for 1 seconds
    mqtt->yield(1000L);
}

bool publishMessage(char *payload, int length)
{
    if (!isFeatureSupported(mqttClient)) {
        return false;  
    }

    struct MQTTConnData* mcd = GetMQTTConnectionData();
    if (mcd->isValid) {
        sendMessageToTopic(mcd, mcd->publishChannel, payload, length);
        return true;
    }
    return false;
}


void LoopForMQTT(void)
{
    if (!isFeatureSupported(mqttClient)) {
        return;
    }

    //Do we have any data
    if (mqttRCDData.isValid) {
        ProcessPayload(mqttRCDData.MQTTPayload);
        mqttRCDData.isValid = false;
    };
    
    mqtt->yield(50L); //50ms

    if (!mqtt->isConnected()) {
        LOG_PRINTFLN(1, "Entering LoopForMQTT");
        struct MQTTConnData* mcd = GetMQTTConnectionData();
        if (!mcd->isValid) {
            LOG_PRINTFLN(1, "FATAL: LoopForMQTT() encounted mcd->isValid as false!");
            return;
        }

        // Close connection if exists
        network.stop();
  
        // Re-establish TCP connection with MQTT broker
        network.connect(mcd->host, mcd->port);
        if (!network.connected()) {
            LOG_PRINTFLN(1, "Can't establish mqtt the TCP connection, %s port: %d , usersetup %d, wifi %d",mcd->host, mcd->port, WaitForUserWifiSetup, WiFi.status() == WL_CONNECTED);
            return;
        }
        
        // Start new MQTT connection
        MqttClient::ConnectResult connectResult;
    
        // Connect
        {
            MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
            options.MQTTVersion = 4;
            options.clientID.cstring = mcd->clientid;
            options.username.cstring = mcd->username;
            options.password.cstring = mcd->password;
            
            options.cleansession = true;
            options.keepAliveInterval = 400; 
            MqttClient::Error::type rc = mqtt->connect(options, connectResult);
            if (rc != MqttClient::Error::SUCCESS) {
                LOG_PRINTFLN(1, "Connection error: mqtt: Missing server or authtication failed.");
                return;
            }
        }
    
        // Subscribe, since this is lost during conn lost case
        {  
            MqttClient::Error::type rc = mqtt->subscribe(
                mcd->subscribeChannel, MqttClient::QOS0, processMessage
            );
            if (rc != MqttClient::Error::SUCCESS) {
                mqtt->disconnect();
                return;
            }
        }
        LOG_PRINTFLN(1,"MQTT subscribed with handler : processMessage");
    } //end of if (!connected)

    if (tickOccured1min) {
        struct MQTTConnData* mcd = GetMQTTConnectionData();
        char json_string[1024];
        serializeJson(jsonConfig, json_string);
        sendMessageToTopic(mcd, "NFOXMQTT/configJson", json_string, strlen(json_string));
    }
    mqtt->yield(100L);
}
