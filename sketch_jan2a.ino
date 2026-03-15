#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

const int pinMicrofon = 4;
const int pinPotentiometru = 5;
const int ledVerde = 3;
const int ledGalben = 9;
const int ledRosu = 10;

const int sampleWindow = 50;
unsigned int sample;

int pragManual = 2000;
bool folosimPotentiometru = true;
char modFunctionare = 'm';

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue(); 
      
      if (rxValue.length() > 0) {
        char comanda = rxValue[0];

        if (comanda == 'm') {
          modFunctionare = 'm';
          pTxCharacteristic->setValue("Mod: MICROFON\n");
          pTxCharacteristic->notify();
        }
        else if (comanda == 'd') {
          modFunctionare = 'd';
          pTxCharacteristic->setValue("Mod: DISCO\n");
          pTxCharacteristic->notify();
        }
        else if (comanda == 'p') {
           folosimPotentiometru = true;
           pTxCharacteristic->setValue("Control: POTENTIOMETRU\n");
           pTxCharacteristic->notify();
        }
        else if (comanda == '+') {
           folosimPotentiometru = false;
           pragManual -= 200;
           if (pragManual < 100) pragManual = 100;
           String mesaj = "Prag: " + String(pragManual) + "\n";
           pTxCharacteristic->setValue(mesaj.c_str());
           pTxCharacteristic->notify();
        }
        else if (comanda == '-') {
           folosimPotentiometru = false;
           pragManual += 200;
           String mesaj = "Prag: " + String(pragManual) + "\n";
           pTxCharacteristic->setValue(mesaj.c_str());
           pTxCharacteristic->notify();
        }
      }
    }
};

void setup() {
  pinMode(ledVerde, OUTPUT);
  pinMode(ledGalben, OUTPUT);
  pinMode(ledRosu, OUTPUT);
  pinMode(pinMicrofon, INPUT);
  pinMode(pinPotentiometru, INPUT);
  
  analogReadResolution(12);
  Serial.begin(115200);

  BLEDevice::init("Microfon_Smart_S3");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                       BLECharacteristic::PROPERTY_WRITE
                     );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("BLE Pornit! Astept conexiune...");
}

void loop() {
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); 
      pServer->startAdvertising(); 
      Serial.println("S-a deconectat. Revenim la setarile fizice.");
      oldDeviceConnected = deviceConnected;
      
      //Resetare la fizic
      folosimPotentiometru = true; 
      modFunctionare = 'm';
  }
  
  if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;
  }

  if (modFunctionare == 'd') {
    digitalWrite(ledVerde, HIGH); delay(100);
    digitalWrite(ledVerde, LOW); digitalWrite(ledGalben, HIGH); delay(100);
    digitalWrite(ledGalben, LOW); digitalWrite(ledRosu, HIGH); delay(100);
    digitalWrite(ledRosu, LOW);
  } 
  else {
    int pragCurent;
    if (folosimPotentiometru) {
      int valPot = analogRead(pinPotentiometru);
      pragCurent = map(valPot, 0, 4095, 100, 3000);
    } else {
      pragCurent = pragManual;
    }

    unsigned long startMillis = millis();
    unsigned int signalMax = 0;
    unsigned int signalMin = 4095;

    while (millis() - startMillis < sampleWindow) {
      sample = analogRead(pinMicrofon);
      if (sample < 4095) {  
         if (sample > signalMax) signalMax = sample;  
         else if (sample < signalMin) signalMin = sample;  
      }
    }
    int volum = signalMax - signalMin;

    digitalWrite(ledVerde, LOW);
    digitalWrite(ledGalben, LOW);
    digitalWrite(ledRosu, LOW);

    if (volum > (pragCurent / 3)) digitalWrite(ledVerde, HIGH);
    if (volum > (pragCurent * 2 / 3)) digitalWrite(ledGalben, HIGH);
    if (volum > pragCurent) digitalWrite(ledRosu, HIGH);
  }
}