/*
 * Display test & discovery progam for MakeItZone's Environmental Sensors group.
 * Target platform is the Heltec ESP32 LoRa v2
*/

// Bring in pre-created pieces code that does a bunch of work for us (libraries)
#include <Wire.h> // i2c
#include <Adafruit_GFX.h> // generic (abstract) drawing functions
#include <Adafruit_SSD1306.h> // interface for our specific screen

#include <Adafruit_Sensor.h> // abstract functions for sensors
#include <Adafruit_BME280.h> // specific code to access the BME200

// Wifi Manager
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";


//Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

IPAddress localIP;
//IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
IPAddress localGateway;
//IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 0, 0);

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

// Set LED GPIO
const int ledPin = 2;
// Stores LED state

String ledState;
String bw_temp;


// Constants for the OLED Screen
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define SCREEN_I2C_SDA 4
#define SCREEN_I2C_SCL 15
#define SCREEN_RESET 16
#define VEXT 21
#define SCREEN_ADDRESS 0x3C

// create instances of i2c interface and SSD1306 objects
TwoWire SCREENI2C = TwoWire(0);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SCREENI2C, SCREEN_RESET);

// Constants for the BME280 Sensor
#define SENSOR_I2C_SDA 17
#define SENSOR_I2C_SCL 22
TwoWire SENSORI2C = TwoWire(1); //second, separate, I2C bus

// create a global varioable to holder our BME280 interface object
Adafruit_BME280 bme;

/*
 * ===============================================================
 * End of global declarations\
*/

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}

// Initialize WiFi
bool initWiFi() {
  if(ssid=="" || ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());


  if (!WiFi.config(localIP, localGateway, subnet)){
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

// Replaces placeholder with LED state value
String processor(const String& var) {
  if(var == "STATE") {
    if(digitalRead(ledPin)) {
      ledState = String(bme.readTemperature());
    }
    else {
      ledState = String(bme.readHumidity());
    }
    return ledState;
  }
  return String();
}

void setup() {
  // put your setup code here, to run once:

  // Setup a serial port so we can send (and recieve) text to a monitoring computer.
  // This is a "virtual" serial port that is sent via USB.
  Serial.begin(115200);
  Serial.println("Starting up...");

  initSPIFFS();

  // Set GPIO 2 as an OUTPUT
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Load values saved in SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile (SPIFFS, gatewayPath);
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);


  if(initWiFi()) {
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
    server.serveStatic("/", SPIFFS, "/");
    
    // Route to set GPIO state to HIGH
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(ledPin, HIGH);
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });

    // Route to set GPIO state to LOW
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(ledPin, LOW);
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
    server.begin();
  }
  else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", SPIFFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(SPIFFS, gatewayPath, gateway.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }

  // Setup the screen

  // On the Heltec ESP32 LoRa v2 the power for the display can be turned off.
  // Turn on the screen
  pinMode(VEXT,OUTPUT);
	digitalWrite(VEXT, LOW);
  // and give a slight delay for it to initialize
  delay(100);

  // start communications with the screen
  SCREENI2C.begin(SCREEN_I2C_SDA, SCREEN_I2C_SCL, 100000);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // setup hardware pins for use to communicate to the sensor
  pinMode(SENSOR_I2C_SCL, OUTPUT);
  digitalWrite(SENSOR_I2C_SCL, HIGH);
  pinMode(SENSOR_I2C_SDA, OUTPUT);
  digitalWrite(SENSOR_I2C_SDA, HIGH);

  // initialize the sensor
  Serial.println("Starting BME280...");
  display.println("Starting BME280...");
  display.display();
  SENSORI2C.begin(SENSOR_I2C_SDA, SENSOR_I2C_SCL, 400000);
  bool status = bme.begin(0x76, &SENSORI2C);  

  // Check that we can communicate with the sensor.
  // If we can't, output a message and go no further.
  // This kind of error handling is important.
  // Can you find a similar example elsewhere in the code?
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    display.println("No BME- check wiring!");
    display.display();
    while (1); // same as "while (true)" - i.e. do nothing more, forever
  }


}

// Function Predeclaration
// C++ only takes into account the code it's seens so far- it won't "read ahead"
// We can either put in all the code we're going to use before we use it
// OR we can pre-declare functions. 
// This tells the compiler "there is going to be"some code that has this name,
// with these parameters, and returns this kind of value" and to set a place holder for later.
// At a later step, during the linking stage, all these "placeholders" are filled in.
// BTW: a function allows you to break up your code, e.g. to allow the re-use of
// some functionality from different parts of your program.
void tempAndHumidityReading(void);

void loop() {
  // put your main code here, to run repeatedly:

  // Output a message to the programmers computer so we know the code is running
  Serial.println("run...");

  display.clearDisplay();

  // get and display the current temperature and humidity
  humidReading();
  tempReading();

  // wait a second before doing it all again
  delay(1000);
}


// and here is the code for the tempAndHumidityReading function
void humidReading(){
  // display temperature
  display.setTextSize(1);
  // the x (horizontal) and y (vertical) location for the next
  // thing to be put on the screen
  display.setCursor(0,0);
  display.print("Temperature: ");
  display.setTextSize(2);
  display.setCursor(0,10);
  display.print(String(bme.readTemperature()));
  display.print(" ");
  display.setTextSize(1);
  // the next two lines display a degree symbol
  display.cp437(true);
  display.write(167);
  display.setTextSize(2);
  display.print("C");

  display.display();
}
// and here is the code for the tempAndHumidityReading function
void tempReading(){
  display.setCursor(0,0);
  // display humidity
  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("Humidity: ");
  display.setTextSize(2);
  display.setCursor(0, 45);
  display.print(String(bme.readHumidity()));
  display.print(" %"); 

  display.display();
}
