#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <DFRobot_DHT11.h>
#include <WiFi.h>

DFRobot_DHT11 DHT;
#define DHT11_PIN 2

uint16_t hum;
int temp;
bool deviceConnected = false;
bool oldDeviceConnected = false;
String pwd = "";
String ssid = "";
BLEServer *pServer;
BLECharacteristic *tempChar, *humChar, *wifiChar;

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }

};

class charCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue();
    if (value.length() > 0) {
      if(ssid == "" && pwd == ""){
        Serial.print("SSID: ");
        ssid = value;
        Serial.println(ssid);
      }
      else if(pwd==""){
        pwd = value;
        Serial.print("Password: ");
        Serial.println(pwd);
        WiFi.begin(ssid, pwd);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        Serial.print("Wifi Connected Sucessfully!");
        wifiChar->setValue("WiFi Successfully connected");
        wifiChar->notify();
        pwd="";
        ssid="";
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("Weather Station");
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *DHTService = pServer->createService("00000002-0000-0000-FDFD-FDFDFDFDFDFD");
  //creating temperature characteristic
  tempChar = DHTService->createCharacteristic(BLEUUID((uint16_t)0x2A6E), BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
  tempChar->addDescriptor(new BLE2902());
  //creating humidity characteristic
  humChar = DHTService->createCharacteristic(BLEUUID((uint16_t)0x2A6F), BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
  humChar->addDescriptor(new BLE2902());
  //creating characteristic for transfering WiFi credentials
  wifiChar = DHTService->createCharacteristic(BLEUUID((uint16_t)0x2AF7), BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  wifiChar->setCallbacks(new charCallback());
  wifiChar->addDescriptor(new BLE2902());
  //Starting the service
  DHTService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID("00000002-0000-0000-FDFD-FDFDFDFDFDFD");
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");  
  if(deviceConnected){
    Serial.println("Device Connected");
  }
}

void loop() {
  DHT.read(DHT11_PIN);
  temp = DHT.temperature*100;
  hum = DHT.humidity*100;
  if(deviceConnected){
    humChar->setValue(hum);
    humChar->notify();
    tempChar->setValue(temp);
    tempChar->notify();
    delay(100);
  }
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;
  }    
  delay(1000);
}
