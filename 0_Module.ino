/*
 * Chip settings are computed each time. This is usually not required, but we just compute and update
 * the json config
 */
void ComputeChipParameters()
{
    JsonObject thischip = jsonConfig.createNestedObject("Chip");
    thischip["type"] = (thisModule == esp01)? "esp01" : "nodemcu";
    thischip["chipid"] =  ESP.getChipId();
    thischip["flashid"] = ESP.getFlashChipId();
    thischip["flashrealsize"] = ESP.getFlashChipRealSize();
    
    byte      mac[6];
    char     _macAddr[18];

    WiFi.macAddress(mac);
    sprintf(_macAddr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    thischip["mac"] = _macAddr;
    thischip["sdk"] = ESP.getSdkVersion();
    
    FlashMode_t ideMode = ESP.getFlashChipMode();
     
    thischip["flashsize"] = ESP.getFlashChipSize();
    thischip["flashmode"] = (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN");
    thischip["flashspeed"] = ESP.getFlashChipSpeed();
    thischip["firmware"] = current_build;

    SaveJsonToFile(&LittleFS);
}

/*
 * Some data is valid only during programming running and is reset every time the chip is started
 */
void ResetInCoreFlags()
{
    JsonObject thischip = jsonConfig.createNestedObject("Flags");
    thischip["pushdata"] = false;
    thischip["armedforany"] = false;
    

    JsonArray thischipFeatures = jsonConfig.createNestedArray("Features");
    //Now enter the features that are coded to run on this chip. This will be useful to debug as it is available in config.json file
      if (thisModule == esp01) {
          for (int i=0; i < esp01FeatureCount; i++) {
              thischipFeatures.add(feature_str[esp01Features[i]]);
          }
      } else if (thisModule == nodemcu) {
          for (int i=0; i < nodemcuFeatureCount; i++) {
              thischipFeatures.add(feature_str[nodumcuFeatures[i]]);
          }
      }
}
