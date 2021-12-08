
/*
 * Dont do much in the callbacks, this might cause an exception if callback dont complete quickly. Just set a flag
 * and handle it in the main loop.
 */
void timerCallback2min(void *pArg) {
      tickOccured2min = true;
} 

void timerCallback1min(void *pArg) {
      tickOccured1min = true;
}

void timerCallback10min(void *pArg) {
      tickOccured10min = true;
}

void setupWatchDogTimers()
{
  /* Set a timer clock, 2min, 1min and 10min*/
  os_timer_setfn(&myTimer2min, timerCallback2min, NULL);
  os_timer_arm(&myTimer2min, 120000, true);  

  os_timer_setfn(&myTimer1min, timerCallback1min, NULL);
  os_timer_arm(&myTimer1min, 60000, true); 

  os_timer_setfn(&myTimer10min, timerCallback10min, NULL);
  os_timer_arm(&myTimer10min, 600000, true); 
}

/*
 * Read the configu from file and check if there is a wifi available. If not create a wifi (hardCodedSSID, hardCodedPW) and
 * try to connect to it. If sucessfull, download the moduleconfig.json file from permalink (maybe github).
 * Check the config config and download the miniWeb files if version as changed/or not in this chip already.
 * 
 * If wifi entry is available, then check  for miniweb files. If miniweb files are absent then connect to wifi and download
 * them. (hardCodedSSID, hardCodedPW) is hardcoding and not present in config.json since it will be used only once.
 */
void setup() {
    Serial.begin(HW_UART_SPEED, SERIAL_8N1, SERIAL_TX_ONLY);
    Serial.println("Starting setup");

    /*Setup watchdog timers and read chip information & config */
    setupWatchDogTimers();
    
    ReadJsonFromFile(&LittleFS);
 
    ComputeChipParameters();
    ResetInCoreFlags();
    SetupMQTT();

    Serial.println("----------------------- Runtime config.json -----------------------------");
    serializeJsonPretty(jsonConfig, Serial);
    Serial.println("----------------------- ------------------- -----------------------------");

    yield();
    if (IsThereAnyConfiguredWifi() == false) {
        //Use hardcoded credentials, and download miniWeb files even if already present
        //Maybe user is unable to use miniWeb to add his own wifi.
        LOG_PRINTFLN(1, "No wifi info fount, try with hardcoded values");
        ConnectWifi(false, true);
    } else {
        //We have some valid wifi, lets try to get and update the loadbalancer details.
        ConnectWifi(false, false);
    }

    DoHttpGetLoadBalancer();
    downloadAllFilesFromPermaLink();
    DisconnectWifi("miniWeb download");
    
    SaveJsonToFile(&LittleFS);

    yield();
    startMiniWebServer();
    print_heap_usage("after setup");

    //Start wifi in AP mode, for user setup
    ConnectWifi(true, false);
}
