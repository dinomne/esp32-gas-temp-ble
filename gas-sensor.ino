#include <DHT.h>
 #include <BLEDevice.h>
 #include <BLEServer.h>
 #include <BLEUtils.h>
 #include <BLE2902.h>
 #include <iostream>
 #include <string>
 
  BLECharacteristic * pCharacteristic;
#define DHTPIN 15     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);
int humidity;
int temperature;
int co2_lvl;
int gas_lvl;
bool deviceConnected = false;

const int co2Pin = 33;
const int ledPin = 2;
const int gasPin = 35;
const int buzzer = 27;
const int coDanger= 3200;
const int gasDanger = 6000;
unsigned long previousMillisBuzz = 0;
//unsigned long previousMillisTemp = 0;  

const unsigned long intervalBuzz = 30UL*1000UL;   
//const unsigned long intervalTemp = 60UL*1000UL;   

 // Visit following link if you want to generate your own UUIDs:
 // https://www.uuidgenerator.net/
 #define SERVICE_UUID "2009347a-0f83-4794-9d22-97bfa82435fd" // UART service UUID
 #define CHARACTERISTIC_UUID_RX "2009347b-0f83-4794-9d22-97bfa82435fd"  //for write
 #define DHTDATA_CHAR_UUID "2009347c-0f83-4794-9d22-97bfa82435fd"   // for notify


 class MyServerCallbacks: public BLEServerCallbacks {
     void onConnect (BLEServer * pServer) {
       deviceConnected = true;
     };

     void onDisconnect (BLEServer * pServer) {
       deviceConnected = false;
     }
 };

 class MyCallbacks: public BLECharacteristicCallbacks {
     void onWrite (BLECharacteristic * pCharacteristic) {
       std :: string rxValue = pCharacteristic-> getValue ();
       Serial.println (rxValue [0]);

       if (rxValue.length ()> 0) {
         Serial.println ("*********");
         Serial.print ("Received Value:");

         for (int i = 0; i <rxValue.length (); i ++) {
           Serial.print (rxValue [i]);
         }
         Serial.println ();
         Serial.println ("*********");
       }

       // Do stuff based on the command received from the app
        if (rxValue.find("ON") != -1) {
          Serial.println("Turning ON!");
        }
        else if (rxValue.find("OFF") != -1) {
          Serial.println("Turning OFF!");
        }       
       }
 };

void setup() {
  Serial.begin(9600);
  Serial.println(F("Module Started"));
  pinMode(ledPin, OUTPUT);
  pinMode(buzzer, OUTPUT);
  dht.begin();

   BLEDevice :: init ("ESP32 DHT22");  // Device Name22

   // Set the device as BLE Server
   BLEServer * pServer = BLEDevice :: createServer ();
   pServer-> setCallbacks (new MyServerCallbacks ());

   // Create the UART service
   BLEService * pService = pServer-> createService (SERVICE_UUID);

   // Create a BLE Feature to send the data
   pCharacteristic = pService-> createCharacteristic (
                       DHTDATA_CHAR_UUID,
                       BLECharacteristic :: PROPERTY_NOTIFY
                     );
                      
   pCharacteristic-> addDescriptor (new BLE2902 ());

   // creates a BLE characteristic to receive the data
   BLECharacteristic * pCharacteristic = pService-> createCharacteristic (
                                          CHARACTERISTIC_UUID_RX,
                                          BLECharacteristic :: PROPERTY_WRITE
                                        );

   pCharacteristic-> setCallbacks (new MyCallbacks ());

   // Start the service
   pService-> start ();

   // Starts the discovery of ESP32
   pServer-> getAdvertising () -> start ();
   Serial.println ("Waiting for a client to connect ...");
}

void get_temp()
{

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temperature = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
 // float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
 // float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(temperature, humidity, false);

  Serial.print(F("Humidity: "));
  Serial.print(humidity);
  Serial.print(F("%  Temperature: "));
  Serial.print(temperature);
  Serial.println(F("°C "));
}


int getMeasurement(int pin) 
{
  int adcVal = analogRead(pin);
 
  float voltage = adcVal * (3.3 / 4095.0);
 
  if (voltage == 0)
  {
    return -1;
  }
  else if (voltage < 0.4)   {
    return -2;
  }
 
  else   {
    float voltageDiference = voltage - 0.4;    

    return (int) ((voltageDiference * 5000.0) / 1.6);

  }
}

boolean isItTime(long lastTime, long interval)
{
  if (millis() - lastTime >= interval )
  {
    return true;

  } else {
  return false;
  }
}
boolean sensor_operational(){
  // if sensor is misbehaving it will show the following value
  // check to aboid buzzing when sensors starts misbehaving
  if (co2_lvl == 9062 || gas_lvl == 9062 )
  {
    return false;

  } else {
  return true;
  }
}
void buzz()
{
    if (isItTime(previousMillisBuzz, intervalBuzz) && sensor_operational())
    {
       previousMillisBuzz  = millis();
       digitalWrite(buzzer, HIGH);
       delay(2000);
       digitalWrite(buzzer, LOW);
    }
}
void loop() {
  // Wait a few seconds between measurements.
   delay(5000);
   get_temp();
   co2_lvl = getMeasurement(co2Pin);
   gas_lvl = getMeasurement(gasPin);
   Serial.print(F("CO2: "));
   Serial.println(co2_lvl);

   Serial.print(F("Gas: "));
   Serial.println(gas_lvl);

   sendData();
   if ((getMeasurement(co2Pin) > coDanger) || (getMeasurement(gasPin) > gasDanger) ){
        digitalWrite(ledPin, HIGH);
        Serial.println("buzzin");
        buzz();
   } else {
        digitalWrite(ledPin, LOW);
   }

        
   
}

void sendData(){
   if (deviceConnected) {
     char humidityString [2];
     char temperatureString [2];
     char co2String [4];
     char gasString [4];

     dtostrf(humidity, 1, 2, humidityString);
     dtostrf(temperature, 1, 2, temperatureString);
     dtostrf (co2_lvl, 1, 2, co2String);
     dtostrf (gas_lvl, 1, 2, gasString);

     char dhtDataString[20];
     sprintf(dhtDataString, "%d,%d,%d,%d", temperature, humidity, co2_lvl, gas_lvl );

     pCharacteristic-> setValue (dhtDataString);
     pCharacteristic-> notify ();  // Send the value to the app!
     Serial.print ("*** Sent Data:");
     Serial.print (dhtDataString);
     Serial.println ("***");
   }
}
