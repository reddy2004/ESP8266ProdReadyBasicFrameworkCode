
/*
 * Each node has only one AP setting
 */
void CreateNewAccessPointSetting(String apssid, String appassword)
{
    JsonObject apcred = ((jsonConfig["APMode"]).isNull() == false)? jsonConfig["APMode"]: jsonConfig.createNestedObject("APMode");
    apcred["apssid"] = apssid;
    apcred["appw"] = appassword;
    SaveJsonToFile(&LittleFS);
}

/*
 * Adds the credentials if ssid does not already exist.
 *If ssid exists, then password is updated if it has changed
*/
bool AddNewWifiCredentials(String ssid, String password)
{
    int numWifiNetworks = 0;
    boolean ssidExists = false;
    boolean pwUpdated = false;
    JsonArray wifiList = ((jsonConfig["wifiList"]).isNull() == false)? jsonConfig["wifiList"] : jsonConfig.createNestedArray("wifiList");
    numWifiNetworks = wifiList.size();

    for (int i=0;i<numWifiNetworks;i++) {
        String cssid = wifiList[i]["ssid"];
        String pw = wifiList[i]["pw"];
        if (cssid == ssid) {
              if (password != pw) {
                  wifiList.remove(i);
                  pwUpdated = true;
              }
              ssidExists = true;
              break;
        }
    }

    if (pwUpdated == true || ssidExists == false) {
          JsonObject data2 = wifiList.createNestedObject();
          data2["ssid"] = ssid;
          data2["pw"] = password;
          data2["ip"] = "";
          data2["subnet"] = "";
          data2["gw"] = "";
          numWifiNetworks++;
          return true;
    } else {
        LOG_PRINTFLN(1, "wifi already exists : %s, pw updated : %d", ssid, pwUpdated);
        return false;
    }
}

/*
 * Wifi handling depending on config settings
 */
void DisconnectWifi(char* caller)
{
    LOG_PRINTFLN(1, "Disconnect wifi : %s", caller);
    WiFi.disconnect();
    WiFi.softAPdisconnect(true);  
    WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);
    delay(500);
}

bool DoesSSIDExist(String ssid)
{
      int numWifiNetworks = 0;
      JsonArray wifiList = ((jsonConfig["wifiList"]).isNull() == false)? jsonConfig["wifiList"] : jsonConfig.createNestedArray("wifiList");
      numWifiNetworks = wifiList.size();  

      for (int i=0;i<numWifiNetworks;i++) {
          if (wifiList[i]["ssid"] == ssid) {
              return true;  
          }
      }
      return false;
}

void RemoveSSID(String ssid)
{
    JsonArray wifiList = ((jsonConfig["wifiList"]).isNull() == false)? jsonConfig["wifiList"] : jsonConfig.createNestedArray("wifiList");
    for (JsonArray::iterator it=wifiList.begin(); it!=wifiList.end(); ++it) {
      if ((*it)["ssid"] == ssid) {
         wifiList.remove(it);
      }
    }  
}

bool IsThereAnyConfiguredWifi()
{
      if ((jsonConfig["wifiList"]).isNull()) {
          return false;  
      }
      JsonArray wifiList = jsonConfig["wifiList"];
      return (wifiList.size() > 0);  
}

void ConnectWifi(bool isAPMode, bool useHardCodedCreds)
{
      WiFi.persistent(false);
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_OFF);

      int retries = 0;
      int retrycount = 1;
      
      if (isAPMode) {
          boolean useHardcodedAPValues = false;
          JsonObject wifiAPCreds = jsonConfig["APMode"];
          if (wifiAPCreds.isNull()) {
            LOG_PRINTFLN(1, "Access point creds not found. Aborting AP creation");
            useHardcodedAPValues = true;
          }
          LOG_PRINTFLN(1, "AP Creds: %s %s", wifiAPCreds["apssid"].as<char*>(), wifiAPCreds["appw"].as<char*>());
          WiFi.mode(WIFI_AP);

          bool apret = true;
          if (useHardcodedAPValues) {
              apret = WiFi.softAP(hardCodedSSID, hardCodedPW); 
          } else {
              apret = WiFi.softAP(wifiAPCreds["apssid"].as<char*>(), wifiAPCreds["appw"].as<char*>()); 
          }
          if (apret == false) {
              LOG_PRINTFLN(1, "Soft ap failed to set up correctly!");  
              WiFi.printDiag(Serial);
          } else {
              LOG_PRINTFLN(1, "Current AP mode ip : %s", WiFi.localIP().toString());
              Serial.println(WiFi.softAPIP());
          }
      } 
      else 
      {
          WiFi.mode(WIFI_STA);
  
          int numWifiNetworks = 0;
          JsonArray wifiList = ((jsonConfig["wifiList"]).isNull() == false)? jsonConfig["wifiList"] : jsonConfig.createNestedArray("wifiList");
          numWifiNetworks = wifiList.size();

          if (useHardCodedCreds) {
              retries = 0;
              WiFi.begin(hardCodedSSID, hardCodedPW);
              LOG_PRINTFLN(1, "Trying hardcoded wifi : %s", hardCodedSSID);
              while(WiFi.waitForConnectResult() != WL_CONNECTED && (retries < retrycount)){
                  retries++;
                  WiFi.begin(hardCodedSSID.c_str(), hardCodedPW.c_str());
                  delay(10);
                  LOG_PRINTFLN(1, "WiFi failed (station mode) hardcoded ssid, retrying. %s", hardCodedSSID);
              }
              //Dont try any other network
              return;              
          }
          
          //Try each of the wifi points
          for (int i=0;i<numWifiNetworks;i++) {
              const char* cssid = wifiList[i]["ssid"].as<char*>();
              const char* cpw = wifiList[i]["pw"].as<char*>();

              retries = 0;
              WiFi.begin(cssid, cpw);
              LOG_PRINTFLN(1, "Trying wifi : %s", cssid);
              while(WiFi.waitForConnectResult() != WL_CONNECTED && (retries < retrycount)){
                  retries++;
                  WiFi.begin(cssid, cpw);
                  delay(10);
                  LOG_PRINTFLN(1, "WiFi failed (station mode), retrying. %s", cssid);
              }

              if (WiFi.status() == WL_CONNECTED) {
                  LOG_PRINTFLN(1, "WiFi connected to (station mode) : %s", cssid);
                  break;
              }
          }
      }
      if (WiFi.status() != WL_CONNECTED && !isAPMode) {
          LOG_PRINTFLN(1, "WiFi FAILED (station mode) :");
      }
}
