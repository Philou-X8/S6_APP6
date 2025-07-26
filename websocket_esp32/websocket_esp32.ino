#include <WiFi.h>
//#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// BLE beacon
//#include <Arduino.h>

#include <BLEDevice.h>
//#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
//#include <BLEEddystoneURL.h>
//#include <BLEEddystoneTLM.h>
#include <BLEBeacon.h>

#include <vector>


const char* ssid = "PLACEHOLDER"; // wifi name 
const char* password = "PLACEHOLDER"; // wifi password

bool ledState = 1;
#define ledPin 2

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


int scanTime = 5;  //In seconds
BLEScan *pBLEScan;

std::vector<String> beacon_UUID;
std::vector<int> beacon_state; // join, check, exit



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------ Web socked recieve
//------------------------------------------------------------------------------
void notifyClients(String data_str) {
  ws.textAll(data_str);
  Serial.print("sending: ");
  Serial.println(data_str);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;

    if (strcmp((char*)data, "LED0") == 0) {
      ledState = 0;
      digitalWrite(ledPin, ledState);
      notifyClients("esp32 toggling led");
    }
    else if (strcmp((char*)data, "LED1") == 0) {
      ledState = 1;
      digitalWrite(ledPin, ledState);
      notifyClients("esp32 toggling led");
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      //Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      //Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void inline initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

/*
String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if (ledState){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  return String();
}
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------ iBeacon Event
//------------------------------------------------------------------------------

void BeaconJoin(String uuid){
  //int index = -1;
  for(int i = 0; i < beacon_UUID.size(); i++){
    if(beacon_UUID[i] == uuid){
      //index = i;
      if(beacon_state[i] == 1){ // state == check
        beacon_state[i] = 0;
      }
      else if(beacon_state[i] == 2){ // state == exit
        beacon_state[i] = 0; // beacon joined back

        //Serial.println("-- beacon JOIN, uuid: " + uuid);
        // SEND JOIN EVENT TO THE WEBSOCKET <------------------
        SendUserEvent(uuid, "JOIN");
      }
      return; // beacon found and dealt with
    }
  }
  // new beacon that never joined
  beacon_UUID.push_back(uuid);
  beacon_state.push_back(0);
  //Serial.println("-- new beacon JOIN, uuid: " + uuid);
  // SEND JOIN EVENT TO THE WEBSOCKET <------------------
  SendUserEvent(uuid, "JOIN");
}

void BeaconExit(){
  for(int i = 0; i < beacon_state.size(); i++){
    if(beacon_state[i] == 0){ // state == join
      beacon_state[i] = 1;
    }
    else if(beacon_state[i] == 1){ // state == check
      beacon_state[i] = 2; // beacon failed to recover from [check] state, assume exit

      //Serial.println("-- beacon EXIT, uuid: " + beacon_UUID[i]);
      // SEND EXIT EVENT TO THE WEBSOCKET <------------------
      SendUserEvent(beacon_UUID[i], "EXIT");
    }
  }
}


class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {

    if (advertisedDevice.haveManufacturerData() == true) {
      String strManufacturerData = advertisedDevice.getManufacturerData();

      uint8_t cManufacturerData[100];
      memcpy(cManufacturerData, strManufacturerData.c_str(), strManufacturerData.length());

      if (strManufacturerData.length() == 25 && cManufacturerData[0] == 0x4C && cManufacturerData[1] == 0x00) {
        BLEBeacon oBeacon = BLEBeacon();
        oBeacon.setData(strManufacturerData);
        //Serial.printf("iBeacon Frame -> ");
        //Serial.printf(
        //  "ID: %04X Major: %d Minor: %d UUID: %s Power: %d\n", oBeacon.getManufacturerId(), ENDIAN_CHANGE_U16(oBeacon.getMajor()),
        //  ENDIAN_CHANGE_U16(oBeacon.getMinor()), oBeacon.getProximityUUID().toString().c_str(), oBeacon.getSignalPower()
        //);

        BeaconJoin(oBeacon.getProximityUUID().toString()); // JOIN beacon
      }
    }

  }
};


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------ Format user events
//------------------------------------------------------------------------------

void SendUserEvent(String uuid, String change){
  String data_str = "USER EVENT:" + uuid + "," + change;
  notifyClients(data_str);
}

/*
String MockUserEvent(){

  // fake user ID
  unsigned int data = millis() % 100000000;
  data += 100000000;
  String data_str = String(data);
  unsigned int str_length = data_str.length();
  data_str = data_str.substring(str_length - 8, str_length - 0);

  // fake join/leave
  if( (data >> 1) % 2 ){
    data_str = data_str + ",JOIN";
  }
  else{
    data_str = data_str + ",EXIT";
  }

  return data_str;
}
*/
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------ void setup()
//------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  delay(1000);
  //Serial.println("En train de se connecter au WiFi...");
  //Serial.println(ssid);
  WiFi.begin(ssid, password);

  int essais = 0;
  while (WiFi.status() != WL_CONNECTED && essais < 20) {
    delay(500);
    Serial.print(".");
    essais++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    //Serial.println("Conecte!");
    Serial.print("Adresse IP du ESP32: ");
    Serial.println(WiFi.localIP());
  } else {
    //Serial.println("Echec de la connexion.");
  }

  initWebSocket();

  server.begin();

  // START BLE BEACON
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------ void loop()
//------------------------------------------------------------------------------
void loop() {
  //unsigned int data = millis();
  //String data_str = String(data);

  //String data_str = MockUserEvent();
  //notifyClients("USER EVENT:" + data_str);

  ws.cleanupClients();

  BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);
  BeaconExit();
  pBLEScan->clearResults();  // delete results fromBLEScan buffer to release memory

  //digitalWrite(ledPin, ledState);

  //Serial.println("led state: " + String(ledState));

  //delay(2001 + (millis() % 5000));
  delay(1000);
}


