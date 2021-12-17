//if we cannot connect 3 times, i.e 6 minutes, just off the relay
volatile int FailedToConnectTries = 0;

/*
 * Connect to one of the configured wifi points if settings support, if not wait if we
 * are in WaitForUserWifiSetup mode. The function is called at the end of the loop
 */
void wifiManagerLoop()
{
    if (WaitForUserWifiSetup) {
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        /*
         * This is  the end of the loop, any activity is already done. Otherwise keep
         * connected if its not specified or if always_on or mqtt is also being used.
         */
         if (isFeatureSupported(wifi_on_activity_http)) {
              DisconnectWifi("wifi manager");
         }
    } else {
        /*
         * Always keep wifi connection. This will use more power. If its turned off, then
         * lets try to switch it on
         */
        if ((isFeatureSupported(wifi_always_on_mqtt) && isFeatureSupported(mqttClient)) ||
            isFeatureSupported(wifi_always_on_http)) {
          ConnectWifi(false, false);  
        }
    }
}

void loop() {
    delay(500);
    LoopCounterCurrentId++;

    yield();
    
    /*
     * do some work, ideally not during the first 5 minutes dedicated for setup.
     * Only items that require a loop() should come here, ex. when reading sensors
     * Remember that the delay is 500ms and this loop executes approximately every 500ms
     */
    if (WaitForUserWifiSetup == false) {
      
        //Just so we know when this module started
        jsonOutgoing["loopid"] = LoopCounterCurrentId;
        LoopForMQTT();
        timeClient.update();
    }

out:
    if (tickOccured2min) {
          minutesElapsedSinceBoot += 2;
          bool mqttConnected = false;
          if (isFeatureSupported(mqttClient) && mqtt->isConnected()) {
              mqttConnected = true;
          }
          LOG_PRINTFLN(1, "Tick Occurred (loop id): %ld, (mins since boot): %d, (heap) : %ld, wifiConnected : %d, mqttConnected: %d", 
                    LoopCounterCurrentId, minutesElapsedSinceBoot, ESP.getFreeHeap(),(WiFi.status() == WL_CONNECTED), mqttConnected );
          tickOccured2min = false;

          //Stay in AP Mode if we dont have any wifi to connect to
          if (WaitForUserWifiSetup && minutesElapsedSinceBoot > 1 && IsThereAnyConfiguredWifi()) {
              WaitForUserWifiSetup = false;
              LOG_PRINTFLN(1, "Turning off AP Mode, %d min elapsed since boot", minutesElapsedSinceBoot);
              DisconnectWifi("stop AP Mode");
          }

          if (WiFi.status() != WL_CONNECTED) {
              //We have work to do in 2 min loop, so lets connect!, wifiManagerLoop will later disconnect
              //after our work is done.
              //Add workflows that require a once in 2min action, say such as sending a heartbeat to the server
              //or say saving 2 min of sensor data to flash etc.
              ConnectWifi(false, false);

              if (WiFi.status() != WL_CONNECTED) {
                FailedToConnectTries++;
                if (FailedToConnectTries >= 3)
                    LOG_PRINTFLN(1, "Turning off relay, as Wifi is not connected : counter : %d", FailedToConnectTries);
                    digitalWrite(15, LOW); 
              } else {
                  FailedToConnectTries = 0;
                  NodePingServer();
              }
          }
    }

    if (tickOccured1min) {
          String formattedTime = timeClient.getFormattedTime();
          Serial.print("Formatted Time: ");
          Serial.println(formattedTime); 
          char tomqtt[128];
          sprintf(tomqtt, "loopid = %ld", LoopCounterCurrentId);
          publishMessage(tomqtt, strlen(tomqtt));
         // LOG_PRINTFLN(1, "Tick Occurred 1Min (loop id): %ld, (mins since boot): %d, (heap) : %ld, isConnected : %d", 
         //           LoopCounterCurrentId, minutesElapsedSinceBoot, ESP.getFreeHeap(),(WiFi.status() == WL_CONNECTED));
         tickOccured1min = false;
    }

    if (tickOccured10min) {
          //LOG_PRINTFLN(1, "Tick Occurred 10Min (loop id): %ld, (mins since boot): %d, (heap) : %ld, isConnected : %d", 
          //          LoopCounterCurrentId, minutesElapsedSinceBoot, ESP.getFreeHeap(),(WiFi.status() == WL_CONNECTED));
          tickOccured10min = false;    
    }

    /* Start the regular loops for features */
    wifiManagerLoop();

    /*
     * Only during the first 5 min when in AP mode. If user wants to flash, then restart the chip and
     * do it during the first 5 min. Remote OTA does not require AP Mode, we can download the bin
     * file and update and restart automatically.
     */
    if (WaitForUserWifiSetup) {
        httpServer.handleClient();
    }
}
