

void SaveJsonToFile(FS *fs)
{
    if (!fs->begin()) {
      Serial.printf("Unable to begin(), aborting\n");
      return;
    }
    File file = fs->open("/config.json", "w");
    serializeJson(jsonConfig, file);
    file.flush();
    file.close();
}

void ReadJsonFromFile(FS *fs)
{
    if (!fs->begin()) {
      Serial.printf("Unable to begin(), aborting\n");
      return;
    }
  
    File file = fs->open("/config.json", "r");
    DeserializationError error = deserializeJson(jsonConfig, file);  
  
    if (error) {
      Serial.println(F("Failed to read file, using default configuration"));
      return;
    }
}

/*
 * Your module can have 1 or more features defined in "enum  ModuleFeature"
 * Each of these features must be implimented in this code base and both
 * setup routines and loop rountines must be there. You can add your own features as well.
 * You can trace how one of the feature routine works. ex. ultrasonic distance sensor.
 * 
 * The only purpose of using a list like this is that I can code all the features that i would
 * use nodemcu/esp01, but only enable/disable them for my particular project at hand.
 * For ex. I have code for reading GPS data, but interfacing in esp01 is hard. I will use GPS
 * for some projects and wont need it for others. This config will help me filter out functionality
 * that i dont need without maintaining multiple different code bases for each project.
 * 
 * This technique will not reduce the size of the flash image. Since all code is compiled and
 * build as a single image.
 */
ModuleFeature EnableNewFeatureForModule(char *feature_name)
{
    ModuleFeature  f;
    bool found = false;
    Serial.println(sizeof(feature_str)/sizeof(feature_str[0]));
    for (int i=0; i < (sizeof(feature_str)/sizeof(feature_str[0])); i++ ){
        Serial.println(feature_str[i]);
        if (0 == strcmp (feature_name, feature_str[i])) {
              f = (ModuleFeature)i;
              found == true;
        }
    }
    Serial.println(f);
    return f;
}
