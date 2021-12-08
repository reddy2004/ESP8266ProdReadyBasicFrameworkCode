# ESP8266ProdReadyBasicFrameworkCode
Framework working code with Wifi connectivity, firmware upgrades, OTA, configuration webpage etc to get started with ESP8266

This code has the following features:

(a) A configuration webpage on http://192.168.4.1/config to add wifi, edit Access point settings and update the firmware
(b) The esp8266 boots with a default Access point setting (tempWifi, tempPassword) for you to log in and configure the chip.
(c) OTA updates are supported by a config json file located remotly which points to a downloadable bin file. The chip can download the firmware and do OTA update.
(d) Update is also possible via http://192.168.4.1/firmware, once your update .bin is ready
(e) Access point can be configured.
(f) Support for multiple wifi networks.
(g) All configuration data is stored in a jsonFile using arduinoJson library
(h) Has basic http.GET() code which can be modifed to suit your need.
(i) Has basic MQTT code to connect to a MQTT server
(j) Feature addition is easy by defining a new feature in an enum and implimenting the setup() and loop() routines for the new feature.

The code already is a working example that pings a http endpoint every 2 mintues. It also has a mqtt client that connects to an mqtt server and is able to send and recive messages. One feature is impliment as an example "esp01_relay_pin", where a esp01 can control a relay. 

Ex Relay Device: https://robu.in/product/esp8266-esp-01-5v-wifi-relay-module/?gclid=EAIaIQobChMIxtLm_vTM9AIV_p1LBR0L8wtoEAQYASABEgLdrPD_BwE
