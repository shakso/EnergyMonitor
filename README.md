# Energy Monitors
All files/scripts for implementing the energy monitor and Octopus Agile rate picture frames featured on [alexshakespeare.com](https://alexshakespeare.com) and [YouTube](https://youtube.com).

The clock/ directory contains the files to implement the following:
![N|Solid](https://alexshakespeare.com/wp-content/uploads/2023/10/20231031_170918.jpg)
Directory structure is as follows
- arduino/ directory contains the sketch that needs to be uploaded to the ESP32 device - any ESP32 device should work, it's to too difficult to port to ESP8266 either as only WiFi is required.
- python/ directory contains the script to send data to the clock through MQTT - an MQTT server needs to be specified, the Octopus Agile rate endpoints also need to be updated to your own plan.
- fritzing/ directory contains pinouts/schematic for implementation of the hardware.
 
The meter/ directory contains the files to implement the following:
![N|Solid](https://alexshakespeare.com/wp-content/uploads/2023/10/20231031_170913.jpg)
Directory structure is as follows
- arduino/ directory contains the sketch that needs to be uploaded to the ESP32 device - any ESP32 device should work, it's to too difficult to port to ESP8266 either as only WiFi is required.
- python/ directory contains the script to send data to the clock through MQTT - an MQTT server needs to be specified, and you need to be setup on the Hildenbrad Ecosystem (see https://github.com/cybermaggedon/pyglowmarkt for more detail). This script also contains a hook to update Home Assistant custom variables, but this can be commented out.
- fritzing/ directory contains pinouts/schematic for implementation of the hardware.