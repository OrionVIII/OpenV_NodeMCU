#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <VitoWiFi.h>
#include <AsyncMqttClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

//HTTP-Server fuer Info-Webseite
ESP8266WebServer httpServer(80);
//HTTP-Server fuer Sketch/Firmware Update
ESP8266HTTPUpdateServer httpUpdater;

//###### Variablen
volatile bool updateVitoWiFi = false;
bool bStopVito = false;
Ticker timer;

//###### Konfiguration
static const char SSID[] = "MySSID";
static const char PASS[] = "MySSIDPass";
static const IPAddress BROKER(192, 168, 10, 60);
static const uint16_t PORT =  1883;
static const char CLIENTID[] = "VitoWifi";
static const char MQTTUSER[] = "myMQTTUser";
static const char MQTTPASS[] = "myMQTTPass";
static const int READINTERVAL = 60; //Abfrageintervall OptoLink, in Sekunden
VitoWiFi_setProtocol(P300);

//###### Allgemein
DPTemp getTempA("gettempa", "allgemein", 0x0800);                               //Aussentemperatur
DPStat getAlarmStatus("getalarmstatus", "allgemein", 0x0A82);                   //Sammelstoerung Ja/Nein

//###### Warmwasser
DPTemp getTempWWist("gettempwwist", "wasser", 0x0804);                          //Warmwasser-Ist
DPTempS getTempWWsoll("gettempwwsoll", "wasser", 0x6300);                       //Warmwasser-Soll
DPTempS setTempWWsoll("settempwwsoll", "wasser", 0x6300);                       //Warmwasser-Soll schreiben

//###### Kessel
DPTemp getTempAbgas("gettempabgas", "kessel", 0x0816);                          //Abgastemperatur

//###### Brenner
DPStat getBrennerStatus("getbrennerstatus", "brenner", 0x55D3);                 //Brennerstatus
DPHours getBrennerStunden("getbrennerstunden", "brenner", 0x08A7);              //Brennerstunden
DPCount getBrennerStarts("getbrennerstarts", "brenner", 0x088A);                //Brennerstarts

//###### Heizkreise
DPTemp getTempVListM1("gettempvlistm1", "heizkreise", 0x2900);                  //HK1 Vorlauftemp
DPStat getHKPumpeM1("gethkpumpem1","kessel", 0x7663);                           //HK1 Pumpe Betrieb
DPTempS getTempRaumNorSollM1("gettempraumnorsollm1", "heizkreise", 0x2306);     //HK1 Raumtemp-Soll
DPTempS setTempRaumNorSollM1("settempraumnorsollm1", "heizkreise", 0x2306);     //HK1 Raumtemp-Soll schreiben
DPTempS getTempRaumRedSollM1("gettempraumredsollm1", "heizkreise", 0x2307);     //HK1 Reduzierte Raumtemp-Soll
DPTempS setTempRaumRedSollM1("settempraumredsollm1", "heizkreise", 0x2307);     //HK1 Reduzierte Raumtemp-Soll schreiben
DPCoP getNeigungKennlinieA1("getneigungkennliniea1", "heizkreise", 0x27D3);     //Neigung Heizkennlinie A1
DPTempS setNeigungKennlinieA1("setneigungkennliniea1", "heizkreise", 0x27D3);   //Neigung Heizkennlinie A1 schreiben; DPCoP funktioniert nicht beim Schreiben!
DPTempS getNiveauKennlinieA1("getniveaukennliniea1", "keizkreise", 0x27D4);     //Niveau Heizkennlinie A1
DPTempS setNiveauKennlinieA1("setniveaukennliniea1", "keizkreise", 0x27D4);     //Niveau Heizkennlinie A1 schreiben

//###### Betriebsarten
DPMode getBetriebArtM1("getbetriebartm1","betriebsarten", 0x2323);          //HK1 0=Abschalt,1=nur WW,2=Heiz+WW, 3=DauerRed,3=DauerNormal
DPMode setBetriebArtM1("setbetriebartm1","betriebsarten", 0x2323);          //HK1 0=Abschalt,1=nur WW,2=Heiz+WW, 3=DauerRed,3=DauerNormal
DPStat getBetriebSparM1("getbetriebsparm1","betriebsarten", 0x2302);        //HK1 Sparbetrieb
DPStat getBetriebPartyM1("getbetriebpartym1","betriebsarten", 0x2303);      //HK1 Party
DPStat setBetriebPartyM1("setbetriebpartym1","betriebsarten", 0x2330);      //HK1 Party schreiben
DPStat setBetriebSparM1("setbetriebsparm1","betriebsarten", 0x2331);        //HK1 Spar schreiben

//###### Objekte und Event-Handler
AsyncMqttClient mqttClient;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;


void connectToWiFi() {
  //Mit WLAN verbinden
  WiFi.begin(SSID, PASS);
}

void connectToMqtt() {
  //Mit MQTT-Server verbinden
  mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  //Wenn WLAN-Verbindung steht und IP gesetzt, zu MQTT verbinden
  timer.once(2, connectToMqtt);
}

void onMqttConnect(bool sessionPresent) {
  //Wenn MQTT verbunden, Topics abonnieren
  mqttClient.subscribe("VITOWIFI/setBetriebArtM1", 0);
  mqttClient.subscribe("VITOWIFI/setBetriebPartyM1", 0);
  mqttClient.subscribe("VITOWIFI/setBetriebSparM1", 0);
  mqttClient.subscribe("VITOWIFI/setTempWWsoll", 0);
  mqttClient.subscribe("VITOWIFI/setTempRaumNorSollM1", 0);
  mqttClient.subscribe("VITOWIFI/setTempRaumRedSollM1", 0);
  mqttClient.subscribe("VITOWIFI/setNeigungKennlinieA1", 0);
  mqttClient.subscribe("VITOWIFI/setNiveauKennlinieA1", 0);

  mqttClient.publish("VITOWIFI/$status/online", 1, true, "1");  //Topic setzen, dieser haelt den Verfuegbarkeitsstatus
  //Timer aktivieren, alle X Sekunden die Optolink-Schnittstelle abfragen
  timer.attach(READINTERVAL, [](){
    updateVitoWiFi = true;
  });
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  //Wenn Mqtt Verbindung verloren und wifi noch verbunden
  if (WiFi.isConnected()) {
    //Mqtt erneut verbinden
    timer.once(2, connectToMqtt);
  }
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  //Wenn WLAN Verbindung verloren, neu verbinden
  timer.once(2, connectToWiFi);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  //char debugChannel[30] = "DEBUG/";
  //mqttClient.publish(debugChannel, 1, true, String(topic).c_str()); 
  
  //Wenn abonnierte MQTT Nachricht erhalten
  if(strcmp(topic,"VITOWIFI/setBetriebArtM1") == 0) {
     
     //mqttClient.publish(debugChannel, 1, true, String("im if BetriebArt").c_str()); 
     //mqttClient.publish(debugChannel, 1, true, String(len).c_str()); 
     //mqttClient.publish(debugChannel, 1, true, String(*payload).c_str()); 

     uint8_t setTemp = atoi(payload);
     if(setTemp>=0 && setTemp<=4){
       DPValue value(setTemp);
       VitoWiFi.writeDatapoint(setBetriebArtM1, value);
       VitoWiFi.readDatapoint(getBetriebArtM1);
     }
  }
  if(strcmp(topic,"VITOWIFI/setBetriebPartyM1") == 0) {
     bool setParty = 0;
     //Wert(Payload) auswerten und Variable setzen
     if (*payload == '1') setParty = 1;
     //In DPValue Konvertieren (siehe github vitowifi fuer Datentypen)
     DPValue value(setParty);
     //Wert an Optolink schicken
     VitoWiFi.writeDatapoint(setBetriebPartyM1, value);
     //Wert auslesen um aktuellen Status an MQTT-Broker zu senden
     VitoWiFi.readDatapoint(getBetriebPartyM1);
  }
  if(strcmp(topic,"VITOWIFI/setBetriebSparM1") == 0) {
     bool setSpar = 0;
     //Wert(Payload) auswerten und Variable setzen
     if (*payload == '1') setSpar = 1;
     //In DPValue Konvertieren (siehe github vitowifi fuer Datentypen)
     DPValue value(setSpar);
     //Wert an Optolink schicken
     VitoWiFi.writeDatapoint(setBetriebSparM1, value);
     //Wert auslesen um aktuellen Status an MQTT-Broker zu senden
     VitoWiFi.readDatapoint(getBetriebSparM1);
  }
  if(strcmp(topic,"VITOWIFI/setTempWWsoll") == 0) {
     uint8_t setTemp = atoi(payload);
     //gemäß Doku: 10 bis 95°C; hier Bereich eingeschraenkt
     if(setTemp>=45 && setTemp<=60){
       DPValue value(setTemp);
       //Wert an Optolink schicken
       VitoWiFi.writeDatapoint(setTempWWsoll, value);
       //Wert auslesen um aktuellen Status an MQTT-Broker zu senden
       VitoWiFi.readDatapoint(getTempWWsoll);
     }
  }
  if(strcmp(topic,"VITOWIFI/setTempRaumNorSollM1") == 0) {
     uint8_t setTemp = atoi(payload);
     if(setTemp>=3 && setTemp<=37){
       DPValue value(setTemp);
       //Wert an Optolink schicken
       VitoWiFi.writeDatapoint(setTempRaumNorSollM1, value);
       //Wert auslesen um aktuellen Status an MQTT-Broker zu senden
       VitoWiFi.readDatapoint(getTempRaumNorSollM1);
     }
  }
  if(strcmp(topic,"VITOWIFI/setTempRaumRedSollM1") == 0) {
     uint8_t setTemp = atoi(payload);
     if(setTemp>=3 && setTemp<=37){
       DPValue value(setTemp);
       //Wert an Optolink schicken
       VitoWiFi.writeDatapoint(setTempRaumRedSollM1, value);
       //Wert auslesen um aktuellen Status an MQTT-Broker zu senden
       VitoWiFi.readDatapoint(getTempRaumRedSollM1);
     }
  }
  if(strcmp(topic,"VITOWIFI/setNiveauKennlinieA1") == 0) {
     //Steuerung erwartet unsigned integer, deshalb umkopieren
     int8_t setNiveauSigned = atoi(payload);
     uint8_t setNiveau = (uint8_t)setNiveauSigned;
     //erlaubter Bereich -13 bis -1  entspricht als unsigned integer: 243 bis 255
     if((setNiveau>=243 && setNiveau<=255) or (setNiveau>=0 && setNiveau<=40)){
       DPValue value(setNiveau);
       //Wert an Optolink schicken
       VitoWiFi.writeDatapoint(setNiveauKennlinieA1, value);
       //Wert auslesen um aktuellen Status an MQTT-Broker zu senden
       VitoWiFi.readDatapoint(getNiveauKennlinieA1);
     }
  }
  if(strcmp(topic,"VITOWIFI/setNeigungKennlinieA1") == 0) {
     //payload kommt als Kommazahl. Steuerung erwartet uint_8 mit Faktor 10
     //Beispiel: aus Neigung 1.4 muss 14 werden 
     float setNeigungfloat = atof(String(payload).c_str());
     uint8_t setNeigung = (uint8_t)(10 * setNeigungfloat);
     //Abfrage Wert zwischen 0.2 bis 3.5
     if(setNeigung>=2 && setNeigung<=35){
       DPValue value(setNeigung);
       //Wert an Optolink schicken
       VitoWiFi.writeDatapoint(setNeigungKennlinieA1, value);
       //Wert auslesen um aktuellen Status an MQTT-Broker zu senden
       VitoWiFi.readDatapoint(getNeigungKennlinieA1);
     }
  }
}

//DPTemp - float |  MQTT-Topic bsp: VITOWIFI/gettempa 
void tempCallbackHandler(const IDatapoint& dp, DPValue value) {
  //Umwandeln, und zum schluss per mqtt publish an mqtt-broker senden
  char outVal[9];
  dtostrf(value.getFloat(), 6, 1, outVal);
  char outName[40] = "VITOWIFI/";
  strcat(outName,dp.getName());
  mqttClient.publish(outName, 1, true, outVal);  
}

//DPHours - float |  MQTT-Topic bsp: VITOWIFI/getbrennerstunden 
void hoursCallbackHandler(const IDatapoint& dp, DPValue value) {
  //Umwandeln, und zum schluss per mqtt publish an mqtt-broker senden
  char outVal[9];
  dtostrf(value.getFloat(), 6, 2, outVal);
  char outName[40] = "VITOWIFI/";
  strcat(outName,dp.getName());
  mqttClient.publish(outName, 1, true, outVal);  
}

//DPTemps & DPMode - uint8_t |  MQTT-Topic bsp: VITOWIFI/getBetriebArtM1
void tempSCallbackHandler(const IDatapoint& dp, DPValue value) {
  //Umwandeln, und zum schluss per mqtt publish an mqtt-broker senden
  int nValue = value.getU8();
  char outName[40] = "VITOWIFI/";
  strcat(outName,dp.getName());
  mqttClient.publish(outName, 1, true, String(nValue).c_str()); 
}

//niveau - int8_t  |  neu für Niveau Heizkennlinie: signed integer
void niveauCallbackHandler(const IDatapoint& dp, DPValue value) {
  //Umwandeln, und zum schluss per mqtt publish an mqtt-broker senden
  int8_t nValue = (int8_t)value.getU8();
  char outName[40] = "VITOWIFI/";
  strcat(outName,dp.getName());
  mqttClient.publish(outName, 1, true, String(nValue).c_str()); 
}

//DPStat - bool |  MQTT-Topic bsp: VITOWIFI/getbrennerstatus 
void statCallbackHandler(const IDatapoint& dp, DPValue value) {
  //Umwandeln, und zum schluss per mqtt publish an mqtt-broker senden
  char outName[40] = "VITOWIFI/";
  strcat(outName,dp.getName());
  mqttClient.publish(outName, 1, true, (value.getBool()) ? "1" : "0");  
}

//DPCount - uint32_t |  MQTT-Topic bsp: VITOWIFI/getBrennerStarts
void countCallbackHandler(const IDatapoint& dp, DPValue value) {
  //Umwandeln, und zum schluss per mqtt publish an mqtt-broker senden
  int nValue = value.getU32();
  char outName[40] = "VITOWIFI/";
  strcat(outName,dp.getName());
  mqttClient.publish(outName, 1, true, String(nValue).c_str()); 
}

//DPCountS - uint16_t |  MQTT-Topic bsp:    VITOWIFI/weissnochnicht
//uint16_t nur fuer Testzwecke benutzt, keinen Parameter mit diesem Format gefunden 
void countSCallbackHandler(const IDatapoint& dp, DPValue value) {
  //Umwandeln, und zum schluss per mqtt publish an mqtt-broker senden
  int nValue = value.getU16();
  char outName[40] = "VITOWIFI/";
  strcat(outName,dp.getName());
  mqttClient.publish(outName, 1, true, String(nValue).c_str()); 
}

//DPCoP - ??uint16_t |  MQTT-Topic bsp: VITOWIFI/getNeigungKennlinieA1
void CoPCallbackHandler(const IDatapoint& dp, DPValue value) {
  //Umwandeln, und zum schluss per mqtt publish an mqtt-broker senden
  char outVal[9];
  dtostrf(value.getFloat(), 6, 1, outVal);
  char outName[40] = "VITOWIFI/";
  strcat(outName,dp.getName());
  mqttClient.publish(outName, 1, true, outVal);
}

void setup() {
  //DEBUG WiFi.mode(WIFI_AP_STA);
  //Setze WLAN-Optionen
  WiFi.mode(WIFI_STA);
  WiFi.hostname(CLIENTID);

  //Setze Datenpunkte als beschreibbar
  setBetriebArtM1.setWriteable(true);
  setBetriebPartyM1.setWriteable(true);
  setBetriebSparM1.setWriteable(true);
  setTempWWsoll.setWriteable(true);
  setTempRaumNorSollM1.setWriteable(true);
  setTempRaumRedSollM1.setWriteable(true);
  setNeigungKennlinieA1.setWriteable(true);
  setNiveauKennlinieA1.setWriteable(true);

  //Zuweisung der Datenpunkte anhand des Rueckgabewerts an entsprechende Handler
  //(siehe github vitowifi fuer Datentypen)
  getTempA.setCallback(tempCallbackHandler);
  getTempWWist.setCallback(tempCallbackHandler);
  getTempVListM1.setCallback(tempCallbackHandler);
  getTempAbgas.setCallback(tempCallbackHandler);
  
  getBrennerStunden.setCallback(hoursCallbackHandler);

  getTempWWsoll.setCallback(tempSCallbackHandler);
  getTempRaumNorSollM1.setCallback(tempSCallbackHandler);
  getTempRaumRedSollM1.setCallback(tempSCallbackHandler);
  getBetriebArtM1.setCallback(tempSCallbackHandler);
  getNiveauKennlinieA1.setCallback(niveauCallbackHandler);

  getAlarmStatus.setCallback(statCallbackHandler);
  getBrennerStatus.setCallback(statCallbackHandler);
  getBetriebPartyM1.setCallback(statCallbackHandler);
  getBetriebSparM1.setCallback(statCallbackHandler);
  getHKPumpeM1.setCallback(statCallbackHandler);

  getBrennerStarts.setCallback(countCallbackHandler);

  getNeigungKennlinieA1.setCallback(CoPCallbackHandler);
  
  //Wichtig, da sonst ueber die Serielle-Konsole (Optolink) Text geschrieben wird
  VitoWiFi.disableLogger();
  //Setze Serielle PINS an VitoWifi
  VitoWiFi.setup(&Serial);

  //Verbindungsaufbau und setzen der Optionen
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  mqttClient.setServer(BROKER, PORT);
  mqttClient.setClientId(CLIENTID);
  mqttClient.setCredentials(MQTTUSER, MQTTPASS);
  mqttClient.setKeepAlive(5);
  mqttClient.setCleanSession(true);
  mqttClient.setWill("VITOWIFI/$status/online", 1, true, "0");
  connectToWiFi();

  //Info-Webseite anzeigen auf HTTP-Port 80
  httpServer.on("/", [](){
    httpServer.send(200, "text/html", "<h1>Vito-Status: " + String((bStopVito ? "Stopped" : "Running")) + "</h1><a href='http://vitowifi.fritz.box/start'>Start</a> <a href='http://vitowifi.fritz.box/stop'>Stop</a> <br><br> <b>Compiled: " __DATE__ " " __TIME__ "</b><br><br><a href='http://vitowifi.fritz.box/reboot'>reboot</a><br><a href='http://vitowifi.fritz.box/update'>update</a><br>");
  });

  //Stop-Funktion sollte etwas schieflaufen :)
  httpServer.on("/stop", [](){
    bStopVito = true;
    httpServer.send(200, "text/plain", "OK - Stopped");
  });
  //Startfunktion
  httpServer.on("/start", [](){
    bStopVito = false;
    httpServer.send(200, "text/plain", "OK - Started");
  });
  //Reboot ueber Webinterface
  httpServer.on("/reboot", [](){
    httpServer.send(200, "text/plain", "OK - rebooting...");
    ESP.restart();
  });

  //Starte Webserver fuer Sketch/Firmware-Update
  httpUpdater.setup(&httpServer);
  httpServer.begin();
}

void loop() {
  if (!bStopVito){
    VitoWiFi.loop();
    if (updateVitoWiFi && mqttClient.connected()) {
      updateVitoWiFi = false;
      VitoWiFi.readAll();
    }
  }
  httpServer.handleClient();
}
