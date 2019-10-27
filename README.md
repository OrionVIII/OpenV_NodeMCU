# OpenV_NodeMCU

work based on [Schnup89/OpenV_NodeMCU](https://github.com/Schnup89/OpenV_NodeMCU)

# files and changes for Viessmann Vitodens 200-W, with one heating circuit:

1. add text of **extract_config.yaml** to your config.yaml of homeassistant
2. add text of **extract_automations.yaml** to your automations.yaml of homeassistant
3. add text of **Lovelace.txt** to your LovelaceUI (rawconfig)
4. use the arduino sketch **VitoWiFi_NodeMCU.ino** (change your WiFi and mqtt settings)
5. the result in homeassistant should look like **Lovelace.png**

I used the Optolink adapter here: [saidlm/Viessmann-optical-interface](https://github.com/saidlm/Viessmann-optical-interface)

manual (in German): [OpenV-GitHub-Wiki](https://github.com/openv/openv/wiki/Bauanleitung-NodeMCU-WIFI---MQTT---HomeAssistant)
