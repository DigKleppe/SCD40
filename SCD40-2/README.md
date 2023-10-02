SCD40 sensor app 
====================
ESP32 connected to Sensirion SCD40 CO2 - t - RH sensor
non-arduiono code, IDF 5.1

The SDC40 (small Aliexpress board) is put in a small box with ESP32 miniboard and powersupply.

At powerup the app tries to connects to the station. If it fails the ESP32 Smartconfig is started. The user can set the network name and password.
The mdns name is default "SensorModule" . (address: SensorModule.local)

ifdef USE_LOWPOWER: (in main.cpp)
During a minute a webserver is enabled. There the sensor name can be changed and offsets can be set. (website can be polished a bit!)

To avoid as munch heat as possible the app measures 1x per minute and sends its data to an UDP port. Then the ESP is put into deep-sleep mode.

If the sensor is mounted away from the heatsource the low-power mode can be disabled. The data is logged and the webserver shows graphs and values.

Todo: restarting from deep-sleep sometimes causes reboots. Awoke reason 4 -5, should be 8.

Note: The normal partitions.csv can be used instead of partitionsOTA-4M.csv. OTA update not used now.
  





  

  







