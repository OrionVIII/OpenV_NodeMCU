# OpenV_NodeMCU

work based on [Schnup89/OpenV_NodeMCU](https://github.com/Schnup89/OpenV_NodeMCU)

# files and changes for Viessmann Vitodens 200-W, with one heating circuit:

1. add text of **extract_configuration.yaml** to your configuration.yaml of homeassistant
2. add text of **extract_automations.yaml** to your automations.yaml of homeassistant
3. add text of **Lovelace.txt** to your LovelaceUI (rawconfig)
4. use the arduino sketch **VitoWiFi_NodeMCU.ino** (change your WiFi and mqtt settings)
5. the result in homeassistant should look like **Lovelace.png**

I used the Optolink adapter here: [saidlm/Viessmann-optical-interface](https://github.com/saidlm/Viessmann-optical-interface)

I used most addresses out of this [document](https://connectivity.viessmann.com/content/dam/vi-micro/CONNECTIVITY/Vitogate/Vitogate-200/7542150-KNX/Datenpunktlisten/DE/20CB_Vitodens_2xx_W_F_B2HA_B2KA_B2LA_B2TA_B2SA_Vitotronic_200_HO1B.pdf/_jcr_content/renditions/original.media_file.download_attachment.file/20CB_Vitodens_2xx_W_F_B2HA_B2KA_B2LA_B2TA_B2SA_Vitotronic_200_HO1B.pdf)

manual (in German): [OpenV-GitHub-Wiki](https://github.com/openv/openv/wiki/Bauanleitung-NodeMCU-WIFI---MQTT---HomeAssistant)
