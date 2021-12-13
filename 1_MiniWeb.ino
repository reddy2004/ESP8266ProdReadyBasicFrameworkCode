/*
 * Consists of a simple webpage that can be accessed in AP Mode
 * This webpage does not have any access credentials. Anyone with the
 * APMode access point SSID and password can connect to this module and edit the configs
 */
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater(true);
String webPage = "";

/* 
 *  XXXX. Keep outside, dont use as a var in function and this wont alloc/free on stack correctly.
 *  Defining HttpClient in a function causes exception before function terminates.
 *  Somehow keeping this as global solves the problem.
*/
HTTPClient http3;

/*
 * Global values for both browser based update and ota update
 */ 
const char* update_path       = "/firmware";
const char* update_username   = "admin";
const char* update_password   = "admin";

 /*
 * Look at configuration data and start one of the following servers
 * (a) HTTP update. If the user requested it in the last reboot. Erase this info from FS after starting server
 * (b) CONFIG PAGE. This is usual page thats a form for user to modify values in the fox.
 */ 

bool checkForMiniWebFiles()
{
    return (LittleFS.exists("miniWeb/body.txt") && LittleFS.exists("miniWeb/logic.js"));
}

/*
 * https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html#http-server
 */
void DoWebUpdate()
{
    JsonObject firmwareUpgrade = (jsonConfig["updates"].isNull() == false)? jsonConfig["updates"] : jsonConfig.createNestedObject("updates");
    String uri = firmwareUpgrade["newBuildLink"];
    LOG_PRINTFLN(1, "**** Updating firmware : %s *****", uri.c_str());

    WiFiClient client;
    int idx = uri.lastIndexOf("/");
    String binFileURL = uri.substring(0, idx);
    String file = uri.substring(idx+1, uri.length());
    
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, binFileURL, 80, file);
      switch(ret) {
      case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          LOG_PRINTFLN(1, "[update] Update failed.");
          break;
      case HTTP_UPDATE_NO_UPDATES:
          LOG_PRINTFLN(1, "[update] Update no Update.");
          break;
      case HTTP_UPDATE_OK:
          LOG_PRINTFLN(1, "[update] Update ok.");
          break;
      }       

  /*Would have already restarted in case of sucessfull update*/
}


bool downloadAndSaveFile(String fileName, String  url){

  bool returnValue = true;
  HTTPClient http;
  WiFiClient wifiClient;

  if (WiFi.status() != WL_CONNECTED) {
      LOG_PRINTFLN(1, "Wifi not connected, cannot download miniWeb files");
      return false;  
  }
  Serial.println("[HTTP] begin...\n");
  Serial.println(fileName);
  Serial.println(url);
  http.begin(wifiClient,url);
  
  Serial.printf("[HTTP] GET...%s\n", url.c_str());
  // start connection and send HTTP header
  int httpCode = http.GET();
  if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      Serial.printf("[FILE] open file for writing %s\n", fileName.c_str());
      
      File file = LittleFS.open(fileName, "w");

      // file found at server
      if(httpCode == HTTP_CODE_OK) {

          int len = http.getSize() - 1; //XXX Why is it coming less?
           LOG_PRINTFLN(1,"Content-Length : %d", len);
            int currentPosition = 0;
            // get tcp stream
            WiFiClient * stream = http.getStreamPtr();
      
            // read all data from server
            while(http.connected() && (len > 0 || len == -1)) {
              // get available data size
              size_t size = stream->available();
              uint8_t  jsonBuffer[256]; // verify this
              uint8_t  actual[257]; // verify this
             // char current[265];
              if(size) {
                // read up to 128 byte
                int c = stream->readBytes(jsonBuffer, ((size > sizeof(jsonBuffer)) ? sizeof(jsonBuffer): size));
                //String myString = String(jsonBuffer);
                
               
                strncpy((char*)actual,(char*)jsonBuffer,c);
                actual[c] = '\0';
                LOG_PRINTFLN(1, "%d ->%d  %s", size, c, actual);
                if(len > 0)
                {
                  len -= c;
                }
                file.print((char *)actual);
              }
          }

          file.flush();
          file.close();
          
      }
      Serial.println("[FILE] closing file\n");
  } 
  else 
  {
      LOG_PRINTFLN(1,"Failed to download miniWeb files: %s, error: %s", fileName, http.errorToString(httpCode).c_str()); 
      returnValue = false;
  }
  http.end();
  return returnValue;
}

/*
 * This will download a config file from a permanent link. This file has links to miniWeb files,
 * and upgrade paths for firmware
 * Defined in String hardCodedPermalink;
 */ 
void DownloadModuleConfigFile()
{
        WiFiClient client;   
        http3.begin(client, hardCodedPermalink); //HTTP
        http3.addHeader("Content-Type", "application/json");

        int httpCode = http3.GET();

        if (httpCode != 200) {
            LOG_PRINTFLN(1, "HTTP Get failed: %d", httpCode);
            return;  
        }

        if (httpCode > 0) {
            String payload = http3.getString();
            Serial.println(payload);
            yield();
            DynamicJsonDocument jsonModuleConfig(1024); //From a hardcoded link
            //nightfox
            DeserializationError error = deserializeJson(jsonModuleConfig, payload.c_str());
            LOG_PRINTFLN(1, "Finished deserializeJson");
            yield();
            if (error) {
                LOG_PRINTFLN(1, "Failed to de-serialize jsonModuleConfig:");
            } else {
                JsonObject miniWebSrc = (jsonModuleConfig["miniWebFiles"].isNull() == false)? jsonModuleConfig["miniWebFiles"] : jsonModuleConfig.createNestedObject("miniWebFiles");
                JsonObject miniWebDest = (jsonConfig["miniWeb"].isNull() == false)? jsonConfig["miniWeb"] : jsonConfig.createNestedObject("miniWeb");

                LOG_PRINTFLN(1, "Versoin compare remove vs local %d &  %d",   miniWebSrc["version"].as<int>(), miniWebDest["version"].as<int>());
                if (miniWebSrc["version"].as<int>() != miniWebDest["version"].as<int>()) {
                    miniWebDest["version"] = miniWebSrc["version"].as<int>();
                    miniWebDest["body"] = miniWebSrc["body"];
                    miniWebDest["logic"] = miniWebSrc["logic"];
                    miniWebDest["update"] = 1;
                }

                //Now lets look at firmware upgrade options
                JsonArray firmwareUpgradeSrc = jsonModuleConfig["firmWareupgrade"];
                JsonObject firmwareUpgradeDest = (jsonConfig["updates"].isNull() == false)? jsonConfig["updates"] : jsonConfig.createNestedObject("updates");

                boolean firmwareUpgradeFound = false;
                for (int i=0;i<firmwareUpgradeSrc.size();i++) {
                    if (firmwareUpgradeSrc[i]["current"] == current_build) {
                        firmwareUpgradeDest["currentBuild"] = current_build;
                        firmwareUpgradeDest["newBuild"] = firmwareUpgradeSrc[i]["update"];
                        firmwareUpgradeDest["newBuildLink"] = firmwareUpgradeSrc[i]["link"];
                        firmwareUpgradeDest["newBuildDesc"] = firmwareUpgradeSrc[i]["desc"];
                        firmwareUpgradeDest["isUpdateAvailable"] = true;
                        firmwareUpgradeFound = true;
                    }
                }

                //Maybe developer removed it, there is no point keeping it since if user clicks on upgrade,
                //it may fail if the bin file is not present. Silently remove the entry
                if (!firmwareUpgradeFound) {
                    firmwareUpgradeDest["currentBuild"] = current_build;
                    firmwareUpgradeDest["newBuild"] = "";
                    firmwareUpgradeDest["newBuildLink"] = "";
                    firmwareUpgradeDest["newBuildDesc"] = "";
                    firmwareUpgradeDest["isUpdateAvailable"] = false;
                }
            }
            LOG_PRINTFLN(1, "Finished deserializeJson update");
            return;
        }
}

void downloadAllFilesFromPermaLink()
{
    LittleFS.begin();

    if (!LittleFS.exists("miniWeb")) {
      LittleFS.mkdir("miniWeb");
      LOG_PRINTFLN(1,"Mkdir miniWeb sucessfull");
    }

    bool updateRequired = false;

    //Download moduleconfig and set this up.
    //This has links for miniWeb files and an array of upgrade paths
    //This should be small or else we will get exceptions!
    DownloadModuleConfigFile();

    JsonObject miniWeb = (jsonConfig["miniWeb"].isNull() == false)? jsonConfig["miniWeb"] : jsonConfig.createNestedObject("miniWeb");
    LOG_PRINTFLN(1, "Lets see if we need an update, version %d & update %d",   miniWeb["version"].as<int>(), miniWeb["update"].as<int>());
    if (!miniWeb["update"].isNull() && miniWeb["update"].as<int>() == 1) {
        LOG_PRINTFLN(1,"MiniWeb files we be updated!");
        updateRequired = true;
    }
    yield();

    if (updateRequired) {
        //Check if we need to update or not  
        //if version has changed, then we update our config and download the new files.
        //XXX does the miniWeb pointer updated automatically
        
        if (downloadAndSaveFile("miniWeb/body.txt", miniWeb["body"].as<char*>()) &&
                downloadAndSaveFile("miniWeb/logic.js", miniWeb["logic"].as<char*>())) {
             miniWeb["update"] = 0; //we are done     
        }
    }
    SaveJsonToFile(&LittleFS);
}

void startMiniWebServer( )
{
    httpUpdater.setup(&httpServer, update_path, update_username, update_password);
    
    int tselect = 0;

    print_heap_usage("before http server");
    webPage += "<!DOCTYPE html>"
              "<html><head>";

    webPage += "</head><body><br>";

    String output = "";
    serializeJsonPretty(jsonConfig, output);

    LOG_PRINTFLN(1, "Custom Webpage created");
    httpServer.on("/config", [](){
        
        httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        httpServer.sendHeader("Pragma", "no-cache");
        httpServer.sendHeader("Expires", "-1");
        httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
        // here begin chunked transfer
        httpServer.send(200, "text/html", "");
        
        httpServer.sendContent("<!DOCTYPE html><html><head> </head><body style='width: 500px;  background: darkgrey; margin: auto;'>");

        File file22 = LittleFS.open("miniWeb/body.txt", "r");
        while (file22.available()) {
            String line = file22.readString();
             httpServer.sendContent(line);
        }
        httpServer.sendContent("<script>");
        File file1= LittleFS.open("miniWeb/logic.js", "r");

        //Just json as is as a string, and the browser will convert it to an object
        //very hacky, but works nonetheless!
        char json_string[2048];
        serializeJson(jsonConfig, json_string);
        httpServer.sendContent("var obj1str = '");
        httpServer.sendContent(json_string);
        httpServer.sendContent("'; var obj1 = JSON.parse(obj1str);");
        
        while (file1.available()) {
             String line = file1.readString();
             httpServer.sendContent(line);
        }
        httpServer.sendContent("</script></body></html>");

        //httpServer.sendHeader(String(F("Connection")), String(F("close")));
        httpServer.sendContent("");
        httpServer.client().stop();
    });

    httpServer.on("/",[]() {
        httpServer.send(200,"text/plain","Try /config or /firmware");  
    });

    httpServer.on("/doupdate", []() {
        if (httpServer.method() == HTTP_GET) {
            Serial.println("HTTP GET, /doupdate");
            for (uint8_t i=0; i<httpServer.args(); i++){
                String key = httpServer.argName(i);
                String value = httpServer.arg(i);
                String message = "REQUEST: " + key + ": " + value;
            }
            httpServer.send(200, "text/html", "<!DOCTYPE html><html><body> Will update and reboot automatically. No progress is shown on this site. Wait for sometime and try to reconnect.</body></html>");
            delay(100);
            DoWebUpdate();
            RestartModule();
        }
    });
    
    httpServer.on("/saveap", []() {
        if (httpServer.method() == HTTP_GET) {
             String ap_ssid_t, ap_password_t;
            Serial.println("HTTP GET, /saveap");
            for (uint8_t i=0; i<httpServer.args(); i++){
                String key = httpServer.argName(i);
                String value = httpServer.arg(i);
                String message = "REQUEST: " + key + ": " + value;

                if (key == "SSID")
                  ap_ssid_t = value;
                else if(key == "Password")
                  ap_password_t = value;
            }
 
            if (ap_ssid_t.length() >= 3 && ap_password_t.length() >= 8) {
                httpServer.send(200, "text/html", "<!DOCTYPE html><html><body> Config sucess, rebooting....</body></html>");
                CreateNewAccessPointSetting(ap_ssid_t, ap_password_t);
                SaveJsonToFile(&LittleFS);
                RestartModule();
            } else {
                httpServer.send(200, "text/html", "<!DOCTYPE html><html><body> Error. SSID must be atleast 3 chars & password 8 chars</body></html>");
            }
            delay(100);
        }
    });

    httpServer.on("/delete", []() {
        if (httpServer.method() == HTTP_GET) {
            Serial.println("HTTP GET, /delete");
            String ssid_t;
            for (uint8_t i=0; i<httpServer.args(); i++){
                String key = httpServer.argName(i);
                String value = httpServer.arg(i);
                String message = "REQUEST: " + key + ": " + value;

                if (key == "SSID")
                  ssid_t = value;
            }
            if (ssid_t.length() > 0 && DoesSSIDExist(ssid_t)) {
                RemoveSSID(ssid_t);
                httpServer.send(200, "text/html", "<!DOCTYPE html><html><body> Config sucess, rebooting....</body></html>");
                SaveJsonToFile(&LittleFS);
                RestartModule();
            } else {
                  httpServer.send(200, "text/html", "<!DOCTYPE html><html><body>Given SSID is invalid</body></html>");
            }
            delay(100);
        }
    });

    httpServer.on("/savewifi", []() {
      if (httpServer.method() == HTTP_GET) {
            String ssid_t, password_t;// ip_t, gateway_t, subnet_t;
            bool dhcp_t = true; //by default

            for (uint8_t i=0; i<httpServer.args(); i++){
                String key = httpServer.argName(i);
                String value = httpServer.arg(i);
                String message = "REQUEST: " + key + ": " + value;
                
                if (key == "SSID")
                  ssid_t = value;
                else if(key == "Password")
                  password_t = value;
                /*
                else if(key == "manual") 
                  dhcp_t = false; 
                else if(key == "IP")
                  ip_t = value;
                else if(key == "Subnet")
                  subnet_t = value;
                else if(key == "Gateway")
                  gateway_t = value;
                */  
            }
            if (ssid_t.length() >= 3 && password_t.length() >= 8) {
                httpServer.send(200, "text/html", "<!DOCTYPE html><html><body> Config sucess, rebooting....</body></html>");
                AddNewWifiCredentials(ssid_t, password_t);
                SaveJsonToFile(&LittleFS);
                RestartModule();
            } else {
                httpServer.send(200, "text/html", "<!DOCTYPE html><html><body> Error. SSID must be atleast 3 chars & password 8 chars</body></html>");
            }
            delay(100);
        }
    }); 

    httpServer.on("/savemqtt", []() {
      if (httpServer.method() == HTTP_GET) {
            String server, un, pw, clientid, sub, pub, port;

            for (uint8_t i=0; i<httpServer.args(); i++){
                String key = httpServer.argName(i);
                String value = httpServer.arg(i);
                String message = "REQUEST: " + key + ": " + value;
                Serial.println(message);
                if (key == "Server")
                  server = value;
                else if(key == "Port")
                  port = value;
                else if(key == "MQTTUser") 
                  un = value; 
                else if(key == "MQTTPw")
                  pw = value;
                else if(key == "Clientid")
                  clientid = value;
                else if(key == "Sub")
                  sub = value;
                else if(key == "Pub")
                  pub = value; 
            }
            if (server.length() < 8 || port.length() == 0 || un.length() == 0 || pw.length() ==0 || clientid.length() ==0 || pub.length() == 0 || sub.length() == 0) {
                httpServer.send(200, "text/html", "<!DOCTYPE html><html><body> Error. Invalid inputs, please try again</body></html>");
            } else {
                httpServer.send(200, "text/html", "<!DOCTYPE html><html><body> Config sucess, rebooting....</body></html>");

                UpdateMQTTConfig(sub, pub, clientid, un, pw, server, port.toInt());
                SaveJsonToFile(&LittleFS);
                RestartModule();
                
            }
            delay(100);
        }
    }); 
out:   
    httpServer.begin();
    print_heap_usage("After miniweb");
    LOG_PRINTFLN(1, "http Server.begin called");
}
