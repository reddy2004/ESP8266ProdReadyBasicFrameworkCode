/*
 * Works to be done come here, where the commands.json from the server is processed
 */
HTTPClient http2;

/*
 * Ex for this module
 * https://robu.in/product/esp8266-esp-01-5v-wifi-relay-module/?gclid=EAIaIQobChMIxtLm_vTM9AIV_p1LBR0L8wtoEAQYASABEgLdrPD_BwE
 */
void SetPinStateForESP01Relay(bool state)
{
    pinMode(0, OUTPUT);
    if (state)
      digitalWrite(0, HIGH);
    else
      digitalWrite(0, LOW);
    LOG_PRINTFLN(1,"In SetPinStateForESP01Relay, setting pin0 to %d", state);
}

void ProcessPayloadInJson()
{
    serializeJsonPretty(jsonIncomingHttp, Serial);
    int esp01Pin0 = jsonIncomingHttp["esp01Pin0"].as<int>();
    if (isFeatureSupported(esp01_relay_pin)) {
        SetPinStateForESP01Relay(esp01Pin0);
    }  
}

void ProcessPayload(String payload)
{
    DeserializationError error = deserializeJson(jsonIncomingHttp, payload.c_str());  
    if (error || jsonIncomingHttp.isNull()) {
      LOG_PRINTFLN(1, "Failed to deserialize json in ProcessPayload");
      return;
    }
    ProcessPayloadInJson();
}

void NodePingServer()
{
        LOG_PRINTFLN(1,"NodePingServer");
        WiFiClient client;   
        http2.begin(client, "http://nightfoxsecurity.com:80/fox_ping"); //HTTP
        http2.addHeader("Content-Type", "application/json");

        //Belongs to old nfxo system
        jsonOutgoing["apssid"] = "Nightfox_1000002";
        jsonOutgoing["token"]  = "mytoken";
        jsonOutgoing["build"] = "nfox_linea";
        jsonOutgoing["alarm"] = false;
        jsonOutgoing["message"] = "LINEA"; //_customMessage;
        jsonOutgoing["timeago"] = 0;
        jsonOutgoing["justboot"] = (LoopCounterCurrentId <= 10)? 1: 0;
        
        JsonObject nested = (jsonOutgoing["foxGpio"].isNull() == false)? jsonOutgoing["foxGpio"] : jsonOutgoing.createNestedObject("foxGpio");
        nested["currwifi"] = "";
        nested["currip"] = "";
        // End = above belnongs to nfox system

        char json_string[256];
        serializeJson(jsonOutgoing, json_string);
        int httpCode = http2.POST(json_string);

        if (httpCode != 200) {
            LOG_PRINTFLN(1, "HTTP Get failed: %d", httpCode);
            return;  
        }

        if (httpCode > 0) {
          String payload = http2.getString();
          Serial.println(payload);
          yield();

          DeserializationError error = deserializeJson(jsonIncomingHttp, payload.c_str());

          JsonObject result = (jsonIncomingHttp["result"].isNull() == false)? jsonIncomingHttp["result"] : jsonIncomingHttp.createNestedObject("result");
          if (result["armedforany"].isNull()) { 
              //Dont do anything, maybe old api?
          } else {
              jsonIncomingHttp["esp01Pin0"] = (result["armedforany"].as<bool>())? 1: 0;  
          }

          ProcessPayloadInJson();
          http2.end();
        }
        yield();
}
