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

void setup() {
  // put your setup code here, to run once:

  // Setup a serial port so we can send (and recieve) text to a monitoring computer.
  // This is a "virtual" serial port that is sent via USB.
  Serial.begin(9600);
  Serial.println("Starting up...");

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

  // get and display the current temperature and humidity
  tempAndHumidityReading();

  // wait a second before doing it all again
  delay(1000);
}


// and here is the code for the tempAndHumidityReading function
void tempAndHumidityReading(){
  display.clearDisplay();
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
