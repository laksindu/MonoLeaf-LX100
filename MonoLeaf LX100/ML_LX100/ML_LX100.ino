// 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 38, 39, 40, 41, 42, 47, 48
//1 is for serial

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Wire.h>
#include <Ticker.h>
#include <ModbusMaster.h>
#include <PubSubClient.h>

WebServer server(80);
Preferences pref;
ModbusMaster node;
Ticker ModbusTicker;

WiFiClient espclient;
PubSubClient client(espclient);

const char* ssid = "Dialog 4G";
const char* password = "N3GH5125L3M";

const char* mqtt_broker = "broker.emqx.io";
const int mqtt_port = 1883;

//relay pins
int relay_1 = 1;
int relay_2 = 2;
int relay_3 = 4;
int relay_4 = 5;

//Switch pins
int switch_1 = 6;
int switch_2 = 7;
int switch_3 = 8;
int switch_4 = 9;

// States
bool Relay_1 = false;
bool Relay_2 = false;
bool Relay_3 = false;
bool Relay_4 = false;

//channels
int Ch_1 = 10;
int Ch_2 = 11;
int Ch_3 = 12;
int Ch_4 = 13;
int Ch_5 = 14;
int Ch_6 = 15;

int Ch1_Buffer = 0;
int Ch2_Buffer = 0;
int Ch3_Buffer = 0;
int Ch4_Buffer = 0;
int Ch5_Buffer = 0;
int Ch6_Buffer = 0;

int SDA_1 = 16;
int SCL_1 = 17;
int SDA_2 = 18;
int SCL_2 = 21;

// RS485 MAX485
int DE_RE = 40;

int count = 0;
int slaveID = 0;

// I2C Bus device
String Bus_1;
String Bus_2;

int temp_bus_1;
int hum_bus_1;
int temp_bus_2;
int hum_bus_2;
int air_bus_1;
int air_bus_2;

String I2cSendingJSON; // for http
char I2csendingBuffer[256]; // for mqtt

String I2cConfigJSON; // for http
char I2cConfigBuffer[265];

String GETi2cJsonAuto; // for http (get config)

int Ch1_TCH;
int Ch2_TCH;
int Ch3_TCH;
int Ch4_TCH;
int Ch5_TCH;
int Ch6_TCH;

int Ch1_AC;
int Ch2_AC;
int Ch3_AC;
int Ch4_AC;
int Ch5_AC;
int Ch6_AC;

String Switch_Type = "Button"; 

bool sensor1_type = LOW; // sensor output ( Active LOW/HIGH);
bool sensor2_type = LOW;
bool sensor3_type = LOW;
bool sensor4_type = LOW;

bool LastState_1 = HIGH;
bool LastState_2 = HIGH;
bool LastState_3 = HIGH;
bool LastState_4 = HIGH;

bool RelayOUT_1 = LOW;
bool RelayOUT_2 = LOW;
bool RelayOUT_3 = LOW;
bool RelayOUT_4 = LOW;

String SwitchType1;
String SwitchType2;
String SwitchType3;
String SwitchType4;

bool CheckState;

//ExDIO var
int IO_PIN;
int IO_state;

String ExIoJsonRes = "{}";
char ExIoBuffer[256];

String switchStates[4] = {"", "", "", ""};
int sensorTypes[4] = {0,0,0,0};
int LastSensorType[4] ={0,0,0,0};
int LastSwitchType[4] ={0,0,0,0};

int SlaveIdArray[10];
int TempArray[10];
int HumArray[10];
String modbusJsonCache = "{}";
int array_0[4] = {0,0,0,0};

int LastChannelState[6];
int LastChannelTCH[6];
int LastChannelAC[6];


int channels[6];
String direction[6];
int state[6];
int targetChannel[6];
String action[6];

int channels_Array[6];
int directionBuffer_Array[6];
int state_Array[6];
int targetChannel_Array[6];
int actionBuffer_Array[6];

char modbusJson_data[256]; // modbus data mqtt 

void enbaleTransmissoin(){
  digitalWrite(DE_RE,HIGH);
}

void enableReading(){
  digitalWrite(DE_RE,LOW);
}

void setup(){

  pinMode(DE_RE,OUTPUT);
  enableReading();

  Serial.begin(115200);
  Serial1.begin(9600,SERIAL_8N1,39,38);

  Serial1.setTimeout(400);

  node.begin(1,Serial1);
  node.preTransmission(enbaleTransmissoin);
  node.postTransmission(enableReading);

  Wire.begin(SDA_1,SCL_1,100000);
  Wire1.begin(SDA_2,SCL_2,100000);

  pinMode(relay_1, OUTPUT);
  pinMode(relay_2, OUTPUT);
  pinMode(relay_3, OUTPUT);
  pinMode(relay_4, OUTPUT);

  pinMode(switch_1, INPUT_PULLUP);
  pinMode(switch_2, INPUT_PULLUP);
  pinMode(switch_3, INPUT_PULLUP);
  pinMode(switch_4, INPUT_PULLUP);

  WiFi.begin(ssid,password);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
  }

  if(WiFi.status() == WL_CONNECTED){
    Serial.print(WiFi.localIP());
  }

  client.setServer(mqtt_broker,mqtt_port);
  client.setCallback(callback);

  SoftAP();
  GetData();

  GetExIo();

  pref.begin("ModCount", true);
  count = pref.getInt("Count", 0);
  pref.end();

  Ch1_Buffer ? pinMode(Ch_1,INPUT_PULLUP) : pinMode(Ch_1, OUTPUT);
  Ch2_Buffer ? pinMode(Ch_2,INPUT_PULLUP) : pinMode(Ch_2, OUTPUT);
  Ch3_Buffer ? pinMode(Ch_3,INPUT_PULLUP) : pinMode(Ch_3, OUTPUT);
  Ch4_Buffer ? pinMode(Ch_4,INPUT_PULLUP) : pinMode(Ch_4, OUTPUT);
  Ch5_Buffer ? pinMode(Ch_5,INPUT_PULLUP) : pinMode(Ch_5, OUTPUT);
  Ch6_Buffer ? pinMode(Ch_6,INPUT_PULLUP) : pinMode(Ch_6, OUTPUT);

  //server.enableCORS(true);
  GetToggelEx();
  
  pref.begin("I2C_BUS",true);
  String BUS_1 = pref.getString("BUS_1",""); 
  String BUS_2 = pref.getString("BUS_2","");
  pref.end();

  if(BUS_1.length() > 0 || BUS_2.length() > 0){
    ScanBus();
  }



  //ModbusTicker.attach(2.0,Modbus);

  //ReadI2Cticker.attach(2.0,ReadI2C);
}
unsigned long lastModbusMillis = 0;
const long modbusInterval = 5000;

unsigned long lastIICMillis = 0;
const long IICInterval = 2000;

unsigned long lastReconnect = 0;
const long retry = 5000;

void loop(){

  server.handleClient();
  InbuiltSwitch();
  ExIoControl();
  RunModbusAutomation();
  RunIIcAutomation();

  if (millis() - lastModbusMillis >= modbusInterval) {
    lastModbusMillis = millis();
    Modbus();
    if(WiFi.status() == WL_CONNECTED){
      MQTT_modbus_env(); // Modbus data transmission
    }

  }

  if(millis() - lastIICMillis >= IICInterval){
    lastIICMillis = millis();
    ReadData(); // I2C_Sensor.ino

    if(WiFi.status() == WL_CONNECTED){
      MQTT_I2C_Sending(); // I2C mqtt transmission
    }

  }

  if(!client.connected()){
    if(millis() - lastReconnect >= retry){
      lastReconnect = millis();
      reconnect();
    }
  }
  else{
    client.loop();
  }


  
}

void callback(char* topic, byte* payload, unsigned long lenght){
  String messagePayload;

  for(int i = 0; i < lenght; i++){
    messagePayload +=(char)payload[i];
  }

  String JsonData = messagePayload;
  JsonDocument doc;
  if(deserializeJson(doc,JsonData) == DeserializationError :: Ok){
    String type = doc["type"];
    int target_relay = doc["target_relay"];
    int state = doc["state"];
    String condition = doc["condition"];
    String metric = doc["metric"];
    int thresh = doc["thresh"];
    int sensor_id = doc["sensor_id"];

    if(type == "relay"){
      bool* RelayOUT[4] = {&RelayOUT_1,&RelayOUT_2,&RelayOUT_3,&RelayOUT_4};
      int relays[4] = {1,2,4,5};
      *RelayOUT[target_relay -1] = state;
      array_0[target_relay -1] = state;
      digitalWrite(relays[target_relay -1],state);

      pref.begin("RelayStorage",false);
      RelayOUT_1 ? pref.putInt("R1",1) : pref.putInt("R1",0);
      RelayOUT_2 ? pref.putInt("R2",1) : pref.putInt("R2",0);
      RelayOUT_3 ? pref.putInt("R3",1) : pref.putInt("R3",0);
      RelayOUT_4 ? pref.putInt("R4",1) : pref.putInt("R4",0);
      pref.end();

      MQTT_Relays();
    }

    if(type == "DIO"){
      IO_PIN = doc["io_pin"];
      IO_state = doc["state"];
      ToggelExIO();
      MQTT_LAST_DIO();
    }

    if(type == "IIC"){
      GETi2cJsonAuto = JsonData;
      I2Cautomation();
      MQTT_I2C_Config();
    }
    

  }

}

void MQTT_Relays(){ // relay states

  JsonDocument doc;
  doc["r_1"] = RelayOUT_1;
  doc["r_2"] = RelayOUT_2;
  doc["r_3"] = RelayOUT_3;
  doc["r_4"] = RelayOUT_4;

  char buffer[256];
  serializeJson(doc,buffer);
  client.publish("lx100/test/relays",buffer);

}

void MQTT_LAST_DIO(){ // DIO states
  GetToggelEx();
  client.publish("lx100/test/DIO",ExIoBuffer);
}

void MQTT_I2C_Sending(){ // i2c env data (MQTT)
  ReadI2C();
  client.publish("lx100/test/iic_env",I2csendingBuffer);
}

void MQTT_I2C_Config(){ // i2c config data ( sending ) (MQTT)
  sendI2Cdata();
  client.publish("lx100/test/iic_config",I2cConfigBuffer);
}

void MQTT_modbus_env(){
  client.publish("lx100/test/modbus_env",modbusJson_data);
}

void reconnect(){
  while(!client.connected()){
    Serial.println("MQTT connecting");

    if(client.connect("lx100")){
      Serial.println("MQTT connected");
      client.subscribe("lx100/test/sub");
      client.publish("lx100/test/pub","online");
      MQTT_Relays();
      MQTT_LAST_DIO();
      MQTT_I2C_Config();
    }
    else{
      Serial.println("MQTT connection failed");
      delay(5000);
    }
  }
}

void InbuiltSwitch(){

  int read1 = digitalRead(switch_1);
  int read2 = digitalRead(switch_2);
  int read3 = digitalRead(switch_3);
  int read4 = digitalRead(switch_4);
  
  pref.begin("RelayStorage",false);

  //relay 1
  if (read1 != LastState_1) {
      if (read1 == sensor1_type) {
        digitalWrite(relay_1, HIGH);
        RelayOUT_1 = HIGH;
        pref.putInt("R1",1);
      } else {
        digitalWrite(relay_1, LOW);
        RelayOUT_1 = LOW;
        pref.putInt("R1",0);
      }
      LastState_1 = read1;
    }
  //relay 2
  if (read2 != LastState_2) {
      if (read2 == sensor2_type) {
        digitalWrite(relay_2, HIGH);
        RelayOUT_2 = HIGH;
        pref.putInt("R2",1);
      } else {
        digitalWrite(relay_2, LOW);
        RelayOUT_2 = LOW;
        pref.putInt("R2",0);
      }
      LastState_2 = read2;
    }
  //relay 3
  if (read3 != LastState_3) {
      if (read3 == sensor3_type) {
        digitalWrite(relay_3, HIGH);
        RelayOUT_3 = HIGH;
        pref.putInt("R3",1);
      } else {
        digitalWrite(relay_3, LOW);
        RelayOUT_3 = LOW;
        pref.putInt("R3",0);
      }
      LastState_3 = read3;
    }
//relay 4
  if (read4 != LastState_4) {
      if (read4 == sensor4_type) {
        digitalWrite(relay_4, HIGH);
        RelayOUT_4 = HIGH;
        pref.putInt("R4",1);
      } else {
        digitalWrite(relay_4, LOW);
        RelayOUT_4 = LOW;
        pref.putInt("R4",0);
      }
      LastState_4 = read4;
    } 

  pref.end();

    
}

void SoftAP(){
 // WiFi.softAP("HomePLC");
 // Serial.print("IP is : " +WiFi.softAPIP().toString());

  server.on("/relay",HTTP_GET,[](){
    server.sendHeader("Access-Control-Allow-Origin", "*");
    int relay = server.arg("id").toInt();
    int state = server.arg("state").toInt();

    Serial.print("relay ");
    Serial.println(relay);

    Serial.print("state ");
    Serial.println(state);

    int relayPins[4] = {1,2,4,5};
    int gpio = relayPins[relay -1]; // Maps 1 based relay ID to 0 based array index (e.g., ID 1 -> relayPins[1 - 1] -> relayPins[0] -> 1st relay || ID 2 -> relayPins[2-1] -> relayPins[1] -> 2nd relay 
    digitalWrite(gpio,state);
    Serial.print(gpio);

     // update Switch state with webdashboard input
    if(gpio == 1) RelayOUT_1 = state , array_0[0] = state; 
    if(gpio == 2) RelayOUT_2 = state , array_0[1] = state;
    if(gpio == 4) RelayOUT_3 = state , array_0[2] = state;
    if(gpio == 5) RelayOUT_4 = state , array_0[3] = state;

    pref.begin("RelayStorage",false);
    RelayOUT_1 ? pref.putInt("R1",1) : pref.putInt("R1",0);
    RelayOUT_2 ? pref.putInt("R2",1) : pref.putInt("R2",0);
    RelayOUT_3 ? pref.putInt("R3",1) : pref.putInt("R3",0);
    RelayOUT_4 ? pref.putInt("R4",1) : pref.putInt("R4",0);
    pref.end(); 

    server.send(200, "text/plain", "OK");
  });

  server.on("/R_OUT",HTTP_GET,[](){
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    
    String status = String(RelayOUT_1 ? "R1_1" : "R1_0") + "," +
                    String(RelayOUT_2 ? "R2_1" : "R2_0") + "," +
                    String(RelayOUT_3 ? "R3_1" : "R3_0") + "," +
                    String(RelayOUT_4 ? "R4_1" : "R4_0");


    server.send(200,"text/plain",status);

  });

      server.on("/Sconfig", HTTP_OPTIONS, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server.send(204); // 204 No Content is the standard response for preflight
    });

  server.on("/Sconfig",HTTP_POST,[](){
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

    String ConfigData = server.arg("plain");
    bool shouldRestart;

    StaticJsonDocument<1024>doc;
    if(deserializeJson(doc,ConfigData) == DeserializationError::Ok){
      JsonArray configs = doc["configuratoins"].as<JsonArray>();

    if (!configs.isNull() && configs.size() > 0) {
      int switchIntBuffer[4] = {0, 0, 0, 0}; // Temporary clean integer array for storing the switches safely

      int index = 0;
      for(JsonObject item : configs ){

        if(index >=4)break;

          sensorTypes[index] = item["sensortype"];
          switchStates[index] = item["switches"].as<String>();

          if(switchStates[index] == "Switch") switchIntBuffer[index] = 0;
          if(switchStates[index] == "Button") switchIntBuffer[index] = 1;
          if(switchStates[index] == "sensor") switchIntBuffer[index] = 2;

          if (!configs.isNull() && configs.size() > 0){
            CheckState = HIGH;
          }

        index++;
      }

        //Serial.println(sensor1_type);
        //Serial.println(switchStates[0]);
        /*
        sensor1_type = sensorTypes[0].toInt();
        sensor2_type = sensorTypes[1].toInt();
        sensor3_type = sensorTypes[2].toInt();
        sensor4_type = sensorTypes[3].toInt(); */

        sensor1_type = sensorTypes[0];
        sensor2_type = sensorTypes[1];
        sensor3_type = sensorTypes[2];
        sensor4_type = sensorTypes[3];

        /*
        SwitchType1 = switchStates[0];
        SwitchType2 = switchStates[1];
        SwitchType3 = switchStates[2];
        SwitchType4 = switchStates[3];*/

        size_t arraySize_1 = sizeof(switchIntBuffer); // Calculate total size in bytes: 5 elements * sizeof(int) = 20 bytes
        size_t arraySize_2 = sizeof(sensorTypes);

        pref.begin("Storage0",false);
        pref.putBytes("switchStates", switchIntBuffer , arraySize_1);
        pref.putBytes("SensorTypes", sensorTypes , arraySize_2);
        pref.end();
      }
      shouldRestart = true;
    }
    //server.send(200,"text/plain","OK");
     if (shouldRestart) {
        server.send(200, "text/plain", "OK. Restarting ESP...");
        delay(1000); // 1 second delay Wi-Fi chip finish sending the "OK" reply
        ESP.restart();
    } else {
        server.send(400, "text/plain", "Error: Invalid JSON or empty configuration");
    } 
  });

  server.on("/data", HTTP_GET, handleSendData);

  server.on("/ExIO",HTTP_OPTIONS,[](){
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(204); // 204 No Content is the standard response for preflight    
  });

  server.on("/exio_toggle",HTTP_OPTIONS,[](){
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(204); // 204 No Content is the standard response for preflight    
  });

  server.on("/ExIO",HTTP_POST,ExIO);

  server.on("/exio_toggle", HTTP_GET,HTTP_ToggelExIO);

  server.on("/GetToggelEx",HTTP_GET,HTTP_ExIoResSend);

  server.on("/ScanIIC",HTTP_OPTIONS,[](){
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(204); // 204 No Content is the standard response for preflight 
  });
  server.on("/IIC_POST",HTTP_POST,[](){
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(204); // 204 No Content is the standard response for preflight     
  });

  server.on("/ScanIIC",HTTP_GET,[](){
    server.sendHeader("Access-Control-Allow-Origin", "*");
    String data = server.arg("scan");
    if(data == "scan"){
      ScanBus();

      pref.begin("I2C_BUS",false);
      pref.putString("BUS_1",Bus_1);
      pref.putString("BUS_2",Bus_2);
      pref.end();

    }

    JsonDocument doc;

    doc["Bus_1"] = Bus_1;
    doc["Bus_2"] = Bus_2;

    String JsonString;
    serializeJson(doc,JsonString);

    server.send(200,"application/json",JsonString);
    delay(1500);
    ESP.restart();
  });

  server.on("/IIC_POST",HTTP_GET,HTTP_IIC_JSON);

  server.on("/i2c_automation",HTTP_OPTIONS,[](){
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(204); // 204 No Content is the standard response for preflight    
  });

  server.on("/i2c_automation",HTTP_POST,I2Cautomation_config);

  server.on("/I2C_POST",HTTP_OPTIONS,[](){
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(204); // 204 No Content is the standard response for preflight    
  });

  server.on("/I2C_POST",HTTP_GET,I2cConfig_send);

  server.on("/Modbus_new",HTTP_OPTIONS,[](){
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(204); // 204 No Content is the standard response for preflight    
  });

  server.on("/Modbus_new",HTTP_POST,GetModbusData);

  server.on("/Modbus_Data",HTTP_OPTIONS,[](){
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(204);
  });

  server.on("/Modbus_Data",HTTP_GET,handleModbusRequest);

  server.on("/modbus_automation",HTTP_OPTIONS,[](){
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(204);
  });
  server.on("/modbus_automation",HTTP_POST,GetModbusAutomation);

  server.begin();

}

void GetData(){
  pref.begin("Storage0",true);
  int bufferArray_1[4]; // Create a temporary buffer array to hold the incoming data
  int bufferArray_2[4];
  pref.getBytes("SensorTypes" , bufferArray_1 , sizeof(bufferArray_1));  // Pull the entire array out of preferences in one step
  pref.getBytes("switchStates", bufferArray_2, sizeof(bufferArray_2));

  sensor1_type = bufferArray_1[0];
  sensor2_type = bufferArray_1[1];
  sensor3_type = bufferArray_1[2];
  sensor4_type = bufferArray_1[3];

  // Last sensor type
  LastSensorType[0] = sensor1_type;
  LastSensorType[1] = sensor2_type;
  LastSensorType[2] = sensor3_type;
  LastSensorType[3] = sensor4_type;

  // Last switch type
  LastSwitchType[0] = bufferArray_2[0];
  LastSwitchType[1] = bufferArray_2[1];
  LastSwitchType[2] = bufferArray_2[2];
  LastSwitchType[3] = bufferArray_2[3];

  /*
  Serial.println(sensor1_type);
  Serial.println(sensor2_type);
  Serial.println(sensor3_type);
  Serial.println(sensor4_type);*/

  //Serial.println(bufferArray_2[0]);

  pref.end();

  
  int read1 = digitalRead(switch_1);
  digitalWrite(relay_1, read1 == sensor1_type ? HIGH : LOW);
  RelayOUT_1 = (read1 == sensor1_type);
  LastState_1 = read1;

  int read2 = digitalRead(switch_2);
  digitalWrite(relay_2, read2 == sensor2_type ? HIGH : LOW);
  RelayOUT_2 = (read2 == sensor2_type);
  LastState_2 = read2;

  int read3 = digitalRead(switch_3);
  digitalWrite(relay_3, read3 == sensor3_type ? HIGH : LOW);
  RelayOUT_3 = (read3 == sensor3_type);
  LastState_3 = read3;

  int read4 = digitalRead(switch_4);
  digitalWrite(relay_4, read4 == sensor4_type ? HIGH : LOW);
  RelayOUT_4 = (read4 == sensor4_type);
  LastState_4 = read4;

  pref.begin("RelayStorage",true);
  int R1 = pref.getInt("R1");
  int R2 = pref.getInt("R2");
  int R3 = pref.getInt("R3");
  int R4 = pref.getInt("R4");
  pref.end();

  digitalWrite(relay_1, R1);
  RelayOUT_1 = R1;
  digitalWrite(relay_2, R2);
  RelayOUT_2 = R2;
  digitalWrite(relay_3, R3);
  RelayOUT_3 = R3;
  digitalWrite(relay_4, R4);
  RelayOUT_4 = R4;
}

void handleSendData(){
  server.sendHeader("Access-Control-Allow-Origin", "*");

  JsonDocument doc;

  // Create the JSON array and populate it
  JsonArray LastSensor = doc.createNestedArray("LastSensor");
  for(int i = 0; i < 4 ; i++){
    LastSensor.add(LastSensorType[i]);
  }

  JsonArray LastSwitch = doc.createNestedArray("LastSwitch");
  for(int i = 0; i < 4; i++){
    LastSwitch.add(LastSwitchType[i]);
  }

  // Serialize the JSON document into a string
  String jsonResponse;
  serializeJson(doc,jsonResponse);

  server.send(200,"application/json", jsonResponse);
}

void ExIO(){ //Expansion digital I/O
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

  String configData = server.arg("plain");
  int directionBuffer[6] = {0,0,0,0,0,0};
  int actionBuffer[6] = {0,0,0,0,0,0};

  StaticJsonDocument<1024>doc;
  bool shouldRestart;

  if(deserializeJson(doc,configData) == DeserializationError::Ok){
    JsonArray configs = doc["configData"].as<JsonArray>(); // this is for json object   
    //Parse the root as a raw JsonArray directly (no ["configData"] key) , JsonArray configs = doc.as<JsonArray>();

    if(!configs.isNull() && configs.size() > 0){
      int index = 0;
      for (JsonObject item : configs){

        if(index >= 6)break;

        channels[index] = item["channel"];
        direction[index] = item["direction"].as<String>();
        state[index] = item["state"];
        targetChannel[index] = item["targetChannel"].as<int>();
        action[index] = item["action"].as<String>();

        if(direction[index] == "Output") directionBuffer[index] = 0;
        if(direction[index] == "Input") directionBuffer[index] = 1;

        if(action[index] == "HIGH") actionBuffer[index] = 1;
        if(action[index] == "LOW") actionBuffer[index] = 0;

        index++;

      }

        size_t arraySize_1 = sizeof(channels);
        size_t arraySize_2 = sizeof(directionBuffer);
        size_t arraySize_3 = sizeof(state);
        size_t arraySize_4 = sizeof(targetChannel);
        size_t arraySize_5 = sizeof(actionBuffer);

        pref.begin("ExIO",false);
        pref.putBytes("channels", channels, arraySize_1);
        pref.putBytes("directionBuffer", directionBuffer, arraySize_2);
        pref.putBytes("state", state, arraySize_3);
        pref.putBytes("targetChannel", targetChannel, arraySize_4);
        pref.putBytes("actionBuffer", actionBuffer, arraySize_5);
        pref.end();

        GetExIo();
        ExIoControl();

        shouldRestart = true;
    }

  }

  server.send(200,"text/plain","ok");
  
  if(shouldRestart){
    delay(2000);
    ESP.restart();
  }
}

void GetExIo(){
  pref.begin("ExIO",true);
  pref.getBytes("channels", channels_Array, sizeof(channels_Array));
  pref.getBytes("directionBuffer", directionBuffer_Array, sizeof(directionBuffer_Array));
  pref.getBytes("state", state_Array, sizeof(state_Array));
  pref.getBytes("targetChannel", targetChannel_Array, sizeof(targetChannel_Array));
  pref.getBytes("actionBuffer", actionBuffer_Array, sizeof(actionBuffer_Array));
  pref.end();

  // Channels Buffer 
  directionBuffer_Array[0] ? Ch1_Buffer = 1 : Ch1_Buffer = 0;
  directionBuffer_Array[1] ? Ch2_Buffer = 1 : Ch2_Buffer = 0;
  directionBuffer_Array[2] ? Ch3_Buffer = 1 : Ch3_Buffer = 0;
  directionBuffer_Array[3] ? Ch4_Buffer = 1 : Ch4_Buffer = 0;
  directionBuffer_Array[4] ? Ch5_Buffer = 1 : Ch5_Buffer = 0;
  directionBuffer_Array[5] ? Ch6_Buffer = 1 : Ch6_Buffer = 0;


  //Target Channels
  Ch1_TCH = targetChannel_Array[0];
  Ch2_TCH = targetChannel_Array[1];
  Ch3_TCH = targetChannel_Array[2];
  Ch4_TCH = targetChannel_Array[3];
  Ch5_TCH = targetChannel_Array[4];
  Ch6_TCH = targetChannel_Array[5];

  //Channel Actions
  Ch1_AC = actionBuffer_Array[0];
  Ch2_AC = actionBuffer_Array[1];
  Ch3_AC = actionBuffer_Array[2];
  Ch4_AC = actionBuffer_Array[3];
  Ch5_AC = actionBuffer_Array[4];
  Ch6_AC = actionBuffer_Array[5];


}

void ExIoControl(){

  if(Ch1_Buffer == 1){
    digitalWrite(Ch1_TCH, (digitalRead(Ch_1) == Ch1_AC) ? HIGH : LOW);
  }
  
  if(Ch2_Buffer == 1){
    digitalWrite(Ch2_TCH, (digitalRead(Ch_2) == Ch2_AC) ? HIGH : LOW);
  }

  if(Ch3_Buffer == 1){
    digitalWrite(Ch3_TCH , (digitalRead(Ch_3) == Ch3_AC) ? HIGH : LOW);
  }

  if(Ch4_Buffer == 1){
    digitalWrite(Ch4_TCH , (digitalRead(Ch_4) == Ch4_AC) ? HIGH : LOW);
  }
  
  if(Ch5_Buffer == 1){
    digitalWrite(Ch5_TCH , (digitalRead(Ch_5) == Ch5_AC) ? HIGH : LOW);
  }

  if(Ch6_Buffer == 1){
    digitalWrite(Ch6_TCH, (digitalRead(Ch_6) == Ch6_AC) ? HIGH : LOW);
  }

}



void HTTP_ToggelExIO(){

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

  IO_PIN = server.arg("gpio").toInt();
  IO_state = server.arg("state").toInt();

  ToggelExIO();

  server.send(200,"text/plain","Ok");
}

void ToggelExIO(){

  digitalWrite(IO_PIN,IO_state);

  pref.begin("Storage_1",false);
  if(IO_PIN == 10) pref.putInt("Ch_1",IO_state);
  if(IO_PIN == 11) pref.putInt("Ch_2",IO_state);
  if(IO_PIN == 12) pref.putInt("Ch_3",IO_state);
  if(IO_PIN == 13) pref.putInt("Ch_4",IO_state);
  if(IO_PIN == 14) pref.putInt("Ch_5",IO_state);
  if(IO_PIN == 15) pref.putInt("Ch_6",IO_state);
  pref.end();
}

void GetToggelEx(){

  pref.begin("Storage_1",true);
  int Ch1_Get = pref.getInt("Ch_1");
  int Ch2_Get = pref.getInt("Ch_2");
  int Ch3_Get = pref.getInt("Ch_3");
  int Ch4_Get = pref.getInt("Ch_4");
  int Ch5_Get = pref.getInt("Ch_5");
  int Ch6_Get = pref.getInt("Ch_6");
  pref.end();

  digitalWrite(Ch_1,Ch1_Get);
  digitalWrite(Ch_2,Ch2_Get);
  digitalWrite(Ch_3,Ch3_Get);
  digitalWrite(Ch_4,Ch4_Get);
  digitalWrite(Ch_5,Ch5_Get);
  digitalWrite(Ch_6,Ch6_Get);

  LastChannelState[0] = Ch1_Get;
  LastChannelState[1] = Ch2_Get;
  LastChannelState[2] = Ch3_Get;
  LastChannelState[3] = Ch4_Get;
  LastChannelState[4] = Ch5_Get;
  LastChannelState[5] = Ch6_Get;

  LastChannelTCH[0] = Ch1_TCH;
  LastChannelTCH[1] = Ch2_TCH;
  LastChannelTCH[2] = Ch3_TCH;
  LastChannelTCH[3] = Ch4_TCH;
  LastChannelTCH[4] = Ch5_TCH;
  LastChannelTCH[5] = Ch6_TCH;

  LastChannelAC[0] = Ch1_AC;
  LastChannelAC[1] = Ch2_AC;
  LastChannelAC[2] = Ch3_AC;
  LastChannelAC[3] = Ch4_AC;
  LastChannelAC[4] = Ch5_AC;
  LastChannelAC[5] = Ch6_AC;

  JsonDocument doc;

  JsonArray arrState = doc.createNestedArray("LastChannelState");
  for(int i = 0; i < 6; i++){
    arrState.add(LastChannelState[i]);
  }

  JsonArray arrTCH = doc.createNestedArray("LastChannelTCH");
  for(int i =0; i < 6; i++){
    arrTCH.add(LastChannelTCH[i]);
  }

  JsonArray arrAC = doc.createNestedArray("LastChannelAC");
  for(int i =0; i < 6; i++){
    arrAC.add(LastChannelAC[i]);
  }

  serializeJson(doc,ExIoJsonRes);
  serializeJson(doc,ExIoBuffer);
}

void HTTP_ExIoResSend(){
  GetToggelEx();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200,"application/json",ExIoJsonRes);
}

void ReadI2C(){

  pref.begin("I2C_BUS",true);
  String BUS_1 = pref.getString("BUS_1"); 
  String BUS_2 = pref.getString("BUS_2");
  pref.end();



  JsonDocument doc;

  String THSensors[] = {"AHT10 / AHT20","HS4001","SHT31 / SHT30"};
  String AirSensors[] = {"BME680","BME688","ZMOD4410AI1V ","SGP30"};

  bool thBus1 = false;
  bool thBus2 = false;

  bool AirBus1 = false;
  bool AirBus2 = false;
  //const int THsensorCount = sizeof(THSensors) / sizeof(THSensors[0]);
 // const int AirSensorCount = sizeof(AirSensors) / sizeof(AirSensors[0]);

  for(int i = 0; i < 3; i++){
    if(BUS_1 == THSensors[i]) thBus1 = true;
    if(BUS_2 == THSensors[i]) thBus2 = true;
  }

  for(int i = 0; i < 4; i++){
    if(BUS_1 == AirSensors[i]) AirBus1 = true;
    if(BUS_2 == AirSensors[i]) AirBus2 = true;
  }

  // BUS 1
  if(thBus1){
    JsonObject bus1obj = doc["BUS_1"].to<JsonObject>();
    bus1obj["type"] = BUS_1; // Fixed: Add properties directly into bus1obj
    bus1obj["temp"] = temp_bus_1;
    bus1obj["hum"] = hum_bus_1;
  }

  else if(AirBus1){
    JsonObject bus1obj = doc["BUS_1"].to<JsonObject>();
    bus1obj["type"] = BUS_1; // Fixed: Add properties directly into bus1obj
    bus1obj["tvoc"] = air_bus_1;
    bus1obj["eco2"] = air_bus_1;
  }

  // BUS 2
  if(thBus2){
    JsonObject bus2obj = doc["BUS_2"].to<JsonObject>();
    bus2obj["type"] = BUS_2;
    bus2obj["temp"] = temp_bus_2;
    bus2obj["hum"] = hum_bus_2;
  }

  else if(AirBus2){
    JsonObject bus2obj = doc["BUS_2"].to<JsonObject>();
    bus2obj["type"] = BUS_2;
    bus2obj["tvoc"] = air_bus_2;
    bus2obj["eco2"] = air_bus_2;
  }

  serializeJson(doc,I2cSendingJSON);
  serializeJson(doc,I2csendingBuffer);

  //Serial.println(JsonString);
}
void HTTP_IIC_JSON(){ // i2c sensors env data (http)
  ReadI2C();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200,"application/json",I2cSendingJSON);
}

void I2Cautomation_config(){ // for http
  GETi2cJsonAuto = server.arg("plain");
  I2Cautomation();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200,"text/plain","ok");
}

void I2Cautomation(){
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc,GETi2cJsonAuto);

  int BUS_1_relay = doc["b1_relay"];
  int BUS_1_thresh = doc["b1_thresh"];
  String BUS_1_Condition = doc["b1_cond"];
  String B_1_type = doc["b1_type"];

  int BUS_2_relay = doc["b2_relay"];
  int BUS_2_thresh = doc["b2_thresh"];
  String BUS_2_Condition = doc["b2_cond"];
  String B_2_type = doc["b2_type"];

  pref.begin("I2C_auto",false);

  pref.putInt("BUS_1_relay",BUS_1_relay);
  pref.putInt("BUS_1_thresh",BUS_1_thresh);
  pref.putString("BUS_1_Condition",BUS_1_Condition);
  pref.putString("B_1_type",B_1_type);

  pref.putInt("BUS_2_relay",BUS_2_relay);
  pref.putInt("BUS_2_thresh",BUS_2_thresh);
  pref.putString("BUS_2_Condition",BUS_2_Condition);
  pref.putString("B_2_type",B_2_type);

  pref.end();

  RunIIcAutomation();

}

void RunIIcAutomation(){
  
  pref.begin("I2C_auto",true);
  // BUS 1
  int BUS_1_relay = pref.getInt("BUS_1_relay");
  int BUS_1_thresh = pref.getInt("BUS_1_thresh");
  String BUS_1_Condition = pref.getString("BUS_1_Condition");
  String B_1_type = pref.getString("B_1_type");
  // BUS 2
  int BUS_2_relay = pref.getInt("BUS_2_relay");
  int BUS_2_thresh = pref.getInt("BUS_2_thresh");
  String BUS_2_Condition = pref.getString("BUS_2_Condition");
  String B_2_type = pref.getString("B_2_type");
  pref.end();

  int thresh_1;
  int thresh_2;

  if(B_1_type == "temp") thresh_1 = temp_bus_1;
  if(B_1_type == "air") thresh_1 = air_bus_1;
  if(B_2_type == "temp") thresh_2 = temp_bus_2;
  if(B_2_type == "air") thresh_2 = air_bus_2;

  int array_1[4] = {1,2,4,5};
  bool* RelayOUT[4] = {&RelayOUT_1,&RelayOUT_2,&RelayOUT_3,&RelayOUT_4}; 

  // BUS 1
  for(int i = 0; i < 4; i++){
    if(BUS_1_Condition == "above"){
      if(BUS_1_relay == array_1[i] && BUS_1_thresh < thresh_1 && !array_0[i]){
        digitalWrite(array_1[i],HIGH);
        *RelayOUT[i] = HIGH;
      }
      else if(BUS_1_relay == array_1[i] && BUS_1_thresh == thresh_1 && !array_0[i]){
        digitalWrite(array_1[i],LOW);
        *RelayOUT[i] = LOW;
      }      
    }

    if(BUS_1_Condition == "below"){
      if(BUS_1_relay == array_1[i] && BUS_1_thresh > thresh_1 and !array_0[i]){
        digitalWrite(array_1[i],HIGH);
        *RelayOUT[i] = HIGH;
      }
      else if(BUS_1_relay == array_1[i] && BUS_1_thresh == thresh_1 and !array_0[i]){
        digitalWrite(array_1[i],LOW);
        *RelayOUT[i] = LOW;
      }      
    }
  }

  // BUS 2
  for(int i = 0; i < 4; i++){
    if(BUS_2_Condition == "above"){
      if(BUS_2_relay == array_1[i] && BUS_2_thresh < thresh_2 && !array_0[i]){
        digitalWrite(array_1[i],HIGH);
        *RelayOUT[i] = HIGH;
      }
      else if(BUS_2_relay == array_1[i] && BUS_2_thresh == thresh_2 && !array_0[i]){
        digitalWrite(array_1[i],LOW);
        *RelayOUT[i] = LOW;
      }
    }

    if(BUS_2_Condition == "below"){
      if(BUS_2_relay == array_1[i] && BUS_2_thresh > thresh_2 && !array_0[i]){
        digitalWrite(array_1[i],HIGH);
        *RelayOUT[i] = HIGH;
      }
      else if(BUS_2_relay == array_1[i] && BUS_2_thresh == thresh_2 && !array_0[i]){
        digitalWrite(array_1[i],LOW);
        *RelayOUT[i] = LOW;
      }
    }
  }


}


void sendI2Cdata(){
  pref.begin("I2C_auto",true);
  // BUS 1
  int BUS_1_relay = pref.getInt("BUS_1_relay");
  int BUS_1_thresh = pref.getInt("BUS_1_thresh");
  String BUS_1_Condition = pref.getString("BUS_1_Condition");
  String B_1_type = pref.getString("B_1_type");
  // BUS 2
  int BUS_2_relay = pref.getInt("BUS_2_relay");
  int BUS_2_thresh = pref.getInt("BUS_2_thresh");
  String BUS_2_Condition = pref.getString("BUS_2_Condition");
  String B_2_type = pref.getString("B_2_type");
  pref.end();

  JsonDocument doc;
  JsonObject BUS1obj = doc["BUS_1"].to<JsonObject>();
  BUS1obj["BUS_1_relay"] = BUS_1_relay;
  BUS1obj["BUS_1_thresh"] = BUS_1_thresh;
  BUS1obj["BUS_1_Condition"] = BUS_1_Condition;

  JsonObject BUS2obj = doc["BUS_2"].to<JsonObject>();
  BUS2obj["BUS_2_relay"] = BUS_2_relay;
  BUS2obj["BUS_2_thresh"] = BUS_2_thresh;
  BUS2obj["BUS_2_Condition"] = BUS_2_Condition;

  serializeJson(doc,I2cConfigJSON);
  serializeJson(doc,I2cConfigBuffer);
}

void I2cConfig_send(){ // i2c config data send (http)
  sendI2Cdata();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200,"application/json",I2cConfigJSON);
}

void GetModbusData(){
  String data = server.arg("new");
  
  if(data == "new"){
    count += 1;
    slaveID = count + 1;

    node.begin(1,Serial1);

    uint8_t result = node.writeSingleRegister(0x0101,slaveID);
    Serial.println(slaveID);
    Serial.println("Unpluged the sensor");

    pref.begin("ModCount",false);
    pref.putInt("Count",count);
    pref.end();

  }
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200,"text/plain","ok");
}

void Modbus(){

  uint8_t results;

  JsonDocument doc;

  for(int id = 2; id <= (count+1); id++){
    node.begin(id,Serial1);
    results = node.readInputRegisters(0x0001,2);

    if(results == node.ku8MBSuccess){
      uint16_t data_0 = node.getResponseBuffer(0) / 10.0;
      uint16_t data_1 = node.getResponseBuffer(1) / 10.0;
      
      String idString = String(id);
      JsonObject idData = doc[idString].to<JsonObject>();
      idData["id"] = id;
      idData["temp"] = data_0;
      idData["hum"] = data_1;

      //update arrays
      SlaveIdArray[id] = id;
      TempArray[id] = data_0;
      HumArray[id] = data_1;

      //Serial.println(TempArray[id]);
    }
    yield();
  }
    serializeJson(doc,modbusJsonCache);
    serializeJson(doc,modbusJson_data);
}

void handleModbusRequest() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", modbusJsonCache);
}

void GetModbusAutomation(){

  String data = server.arg("plain");

  pref.begin("Modbus_auto",false);
  pref.putString("auto_data",data);
  pref.end();

  //Serial.println(data);

  RunModbusAutomation();

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200,"text/plain","ok");



}

void RunModbusAutomation(){

  pref.begin("Modbus_auto",true);
  String savedJson = pref.getString("auto_data");
  pref.end();

  //Serial.println(savedJson);

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc,savedJson);

  int array_1[4] = {1,2,4,5};
  bool* RelayOUT[4] = {&RelayOUT_1,&RelayOUT_2,&RelayOUT_3,&RelayOUT_4};

  JsonArray automation = doc["modbus_automations"];
  for(JsonObject item : automation){
    int sensorId = item["id"];
    int relay = item["relay"];
    int thresh = item["thresh"];
    String metric = item["metric"];
    String condition = item["cond"];

    int currentTemp = TempArray[sensorId];
    int currentHum = HumArray[sensorId];

    for(int i = 0; i < 4; i++){
      if(relay == array_1[i]){ // check relay and switch state of inbuilt 4 relays
        if(condition == "above"){ // check condition
          if(metric == "temp"){ // check metric
            if(thresh < currentTemp  && !array_0[i]){ digitalWrite(array_1[i],HIGH); *RelayOUT[i] = HIGH; }
            else if(thresh == currentTemp  && !array_0[i]){ digitalWrite(array_1[i],LOW); *RelayOUT[i] = LOW; }
          }
          else if(metric == "hum"){
            if(thresh < currentHum  && !array_0[i]) { digitalWrite(array_1[i],HIGH); *RelayOUT[i] = HIGH; }
            else if(thresh == currentHum  && !array_0[i]){digitalWrite(array_1[i],LOW); *RelayOUT[i] = LOW; }
          }
        }
        else if(condition == "below"){
          if(metric == "temp"){ // check metric
            if(thresh > currentTemp && !array_0[i]){ digitalWrite(array_1[i],HIGH); *RelayOUT[i] = HIGH;}
            else if(thresh == currentTemp  && !array_0[i]){ digitalWrite(array_1[i],LOW); *RelayOUT[i] = LOW;}
          }
          else if(metric == "hum"){
            if(thresh < currentHum  && !array_0[i]) { digitalWrite(array_1[i],HIGH); *RelayOUT[i] = HIGH;}
            else if(thresh == currentHum  && !array_0[i]){ digitalWrite(array_1[i],LOW); *RelayOUT[i] = LOW;}
          }          
        }
        break;
      }
    }

  }
}