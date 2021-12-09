# ESP8266ProdReadyBasicFrameworkCode
Framework code with implimented features such as Wifi connectivity, firmware upgrades, OTA, configuration webpage etc to get started on your project with ESP8266


This code has the following features:

(a) A configuration webpage on http://192.168.4.1/config to add wifi, edit Access point settings and update the firmware

(b) The esp8266 boots with a default Access point setting (tempWifi, tempPassword) for you to log in and configure the chip on initial boot up after flashing.

(c) OTA updates are supported by a config json file located remotly which points to a downloadable bin file. The chip can download the firmware and do an OTA update.

(d) Update is also possible via http://192.168.4.1/firmware as well. You can upload your new .bin directly.

(e) Access points can be configured.

(f) Support for multiple wifi networks., the module will try to connect to any of the configured WiFis'.

(g) All configuration data is stored in a jsonFile using arduinoJson library

(h) Has basic http.GET() code which can be modifed to suit your need.

(i) Has basic MQTT code to connect to a MQTT server

(j) Feature addition is easy by defining a new feature in an enum and implimenting the setup() and loop() routines for the new feature.


![alt text](https://github.com/reddy2004/ESP8266ProdReadyBasicFrameworkCode/blob/main/images/miniWeb.png)

Use code in https://github.com/reddy2004/Node_MQTT_HTTP_Server_For_ESP8266 as your server.

The code already is a working example that pings a http endpoint every 2 mintues. It also has a mqtt client that connects to an mqtt server and is able to send and recive messages. One feature is implimented as an example "esp01_relay_pin", where a esp01 can control a relay. 

Ex Relay Device: https://robu.in/product/esp8266-esp-01-5v-wifi-relay-module/?gclid=EAIaIQobChMIxtLm_vTM9AIV_p1LBR0L8wtoEAQYASABEgLdrPD_BwE

Please go through the pdf and the text documentation to learn how to use this code as a starter for your project.

If you find this piece of code useful, please consider 'star'ing the project.:)
