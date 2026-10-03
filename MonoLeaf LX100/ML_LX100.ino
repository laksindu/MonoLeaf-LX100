// 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 38, 39, 40, 41, 42, 47, 48
//1 is for serial

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>

WebServer server(80);
Preferences pref;

const char* ssid = "Dialog 4G";
const char* password = "N3GH5125L3M";

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

String switchStates[4] = {"", "", "", ""};
int sensorTypes[4] = {0,0,0,0};
int LastSensorType[4] ={0,0,0,0};
int LastSwitchType[4] ={0,0,0,0};

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


void setup(){

  Serial.begin(115200);

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

  SoftAP();
  GetData();

  GetExIo();

  Ch1_Buffer ? pinMode(Ch_1,INPUT_PULLUP) : pinMode(Ch_1, OUTPUT);
  Ch2_Buffer ? pinMode(Ch_2,INPUT_PULLUP) : pinMode(Ch_2, OUTPUT);
  Ch3_Buffer ? pinMode(Ch_3,INPUT_PULLUP) : pinMode(Ch_3, OUTPUT);
  Ch4_Buffer ? pinMode(Ch_4,INPUT_PULLUP) : pinMode(Ch_4, OUTPUT);
  Ch5_Buffer ? pinMode(Ch_5,INPUT_PULLUP) : pinMode(Ch_5, OUTPUT);
  Ch6_Buffer ? pinMode(Ch_6,INPUT_PULLUP) : pinMode(Ch_6, OUTPUT);

  //server.enableCORS(true);
  GetToggelEx();
}

void loop(){

  server.handleClient();
  InbuiltSwitch();
  ExIoControl();
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
    if(gpio == 1) RelayOUT_1 = state; 
    if(gpio == 2) RelayOUT_2 = state; 
    if(gpio == 4) RelayOUT_3 = state;
    if(gpio == 5) RelayOUT_4 = state;

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
        delay(1000); // 1-second delay lets the Wi-Fi chip finish sending the "OK" reply
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

  server.on("/exio_toggle", HTTP_GET,ToggelExIO);

  server.on("/GetToggelEx",HTTP_GET,GetToggelEx);


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

void ToggelExIO(){

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

  String GPIO = server.arg("gpio");
  String State = server.arg("state");

  int gpio_int = GPIO.toInt();
  int state_int = State.toInt();

  digitalWrite(gpio_int,state_int);

  pref.begin("Storage_1",false);
  if(gpio_int == 10) pref.putInt("Ch_1",state_int);
  if(gpio_int == 11) pref.putInt("Ch_2",state_int);
  if(gpio_int == 12) pref.putInt("Ch_3",state_int);
  if(gpio_int == 13) pref.putInt("Ch_4",state_int);
  if(gpio_int == 14) pref.putInt("Ch_5",state_int);
  if(gpio_int == 15) pref.putInt("Ch_6",state_int);
  pref.end();

  //Serial.println(gpio_int);
  //Serial.println(state_int);
  
  server.send(200,"text/plain","Ok");
}

void GetToggelEx(){

  server.sendHeader("Access-Control-Allow-Origin", "*");

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

  String jsonResponse;
  serializeJson(doc,jsonResponse);

  server.send(200,"application/json",jsonResponse);

}