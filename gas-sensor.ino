#include <DHT.h>
#include "WiFi.h"
#include <iostream>
#include <string>

#define DHTPIN 15     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

// WIFi/HTTP settings
const char* ssid = "********";
const char* password = "******";
WiFiServer server(80);
String header;
bool deviceConnected = false;

//current sensor values
int humidity;
int temperature;
int co2_lvl;
int gas_lvl; 

//pins on esp32
const int co2Pin = 33;
const int ledPin = 2;
const int gasPin = 35;
const int buzzer = 27;

//variables for buzzing
const int coDanger= 3200;
const int gasDanger = 6000; 
unsigned long previousMillisBuzz = 0;
const unsigned long intervalBuzz = 30UL*1000UL;  //30 sec 



   
void setup() {
  Serial.begin(9600);
  Serial.println(F("Module Started"));
  pinMode(ledPin, OUTPUT);
  pinMode(buzzer, OUTPUT);
  dht.begin();
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

}

void get_temp(){

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temperature =dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)){
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  float hic = dht.computeHeatIndex(temperature, humidity, false);


int getMeasurement(int pin){
  int adcVal = analogRead(pin);
  float voltage = adcVal * (3.3 / 4095.0);
 
  if (voltage == 0){
    return -1;
  }
  else if (voltage < 0.4) {
    return 0;
  }
  else {
    float voltageDiference = voltage - 0.4;    
    return (int) ((voltageDiference * 5000.0) / 1.6);
  }
}


boolean isItTime(long lastTime, long interval)
{
  if (millis() - lastTime >= interval ){
    return true;

  } else {
  return false;
  }
}
boolean sensor_operational(){
  // if sensor is misbehaving it will show the following value
  // check to aboid buzzing when sensors starts misbehaving
  if (co2_lvl == 9062 || gas_lvl == 9062 ){
    return false;
  } else {
  return true;
  }
}

void buzz(){
    if (isItTime(previousMillisBuzz, intervalBuzz) && sensor_operational()){
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
 //  Serial.print(F("CO2: "));
  // Serial.println(co2_lvl);

 //  Serial.print(F("Gas: "));
  // Serial.println(gas_lvl);
   web();
   if ((getMeasurement(co2Pin) > coDanger) || (getMeasurement(gasPin) > gasDanger) ){
        digitalWrite(ledPin, HIGH);
        Serial.println("buzzin");
        buzz();
   } else {
        digitalWrite(ledPin, LOW);
   }  
}


   
void web(){
   String co2Str = String(getMeasurement(co2Pin));
   String gasStr = String(getMeasurement(gasPin));
   String humidityStr = String(dht.readHumidity());
   String temperatureStr =String(dht.readTemperature());
   WiFiClient client = server.available();   // Listen for incoming clients
   if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the table 
            
            // Web Page Heading
            client.println("</style></head><body>");
            client.println("<table><tr><th>MEASUREMENT</th><th>VALUE</th></tr>");
            client.print("<tr><td>Temp. Celsius</td><td><span class=\"temperature\">");
            client.print(temperatureStr);
            client.println("</span> C</td></tr>");   
                
            client.print("<tr><td>Carbon Monoxide</td><td><span class=\"co_level\">");
            client.print(co2Str);
            client.println("</span> ppm</td></tr>");
            
            client.print("<tr><td>Natural Gas</td><td><span class=\"gas_level\">");
            client.print(gasStr);
            client.println("</span> ppm</td></tr>"); 
            
            client.print("<tr><td>Humidity</td><td><span class=\"humidity\">");
            client.print(humidityStr);
            client.println("</span> %</td></tr>"); 
            
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}