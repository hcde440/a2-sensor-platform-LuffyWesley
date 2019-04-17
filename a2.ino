// The code does three things:
// 1. Gets data from adafruit.io; if the switch if turned on (1) then the LED will light up and vice versa.
// 2. Gets a city's humudity and temperature data and sends it to adafruit.io.
// 3. Gets and sends humidity data from the Si7021 sensor

#include "config.h" // edit the config.h tab and enter your credentials
#include <ESP8266WiFi.h>// provides the ability to connect the arduino with the WiFi
#include <ESP8266HTTPClient.h> //provides the ability to interact with websites
#include <ArduinoJson.h> //provides the ability to parse and construct JSON objects
#include "Adafruit_Si7021.h" // access to the sensor's library

Adafruit_Si7021 sensor = Adafruit_Si7021(); 
const char* city = "Nairobi"; // Change the name of the city to whatever you want

#define LED_PIN 13 // digital pin 13

AdafruitIO_Feed *digital = io.feed("led"); // set up the 'led' feed
AdafruitIO_Feed *city_humidity = io.feed("city_humidity"); // set up the 'city_humidity' feed
AdafruitIO_Feed *humidity = io.feed("humidity"); // set up the 'humidity' feed
AdafruitIO_Feed *city_temp = io.feed("city_temp"); // set up the 'city_temp' feed

typedef struct {
  String hd;
  String tp;
} MetData;    //then we give our new data structure a name so we can use it in our code

MetData conditions; //we have created a MetData type, but not an instance of that type, so we create the variable 'conditions' of type MetData

void setup() {
 
  pinMode(LED_PIN, OUTPUT); // set led pin as a digital output
 
  Serial.begin(115200); // start the serial connection

  // Prints the results to the serial monitor
  Serial.print("This board is running: ");  //Prints that the board is running
  Serial.println(F(__FILE__));
  Serial.print("Compiled: "); //Prints that the program was compiled on this date and time
  Serial.println(F(__DATE__ " " __TIME__));

  //while arduino is connected to wifi print . to serial monitor
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println(); Serial.println("WiFi connected"); Serial.println(); //prints that we are connected to the wifi
  Serial.print("Your ESP has been assigned the internal IP address "); //prints the internal IP address
  Serial.println(WiFi.localIP()); // gets localIP address
 
  while(! Serial); // wait for serial monitor to open
 
  // connect to io.adafruit.com
  Serial.print("Connecting to Adafruit IO");
  io.connect();
 
  // set up a message handler for the 'led' feed. The handleMessage function (defined below)
  // will be called whenever a message is received from adafruit io.
  digital->onMessage(handleMessage);
 
  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
 
  // we are connected
  Serial.println();
  Serial.println(io.statusText());

  getMet(); //calles the getMet function to get weather data

  //prints a series of text describing the weather based on the MetData slots
  Serial.println();
  Serial.print(String("OpenWeather states that ") + city + " has a humidity of " + conditions.hd + "%, ");
  Serial.println("and a temp of " + conditions.tp + "F.");
  
  // Si7021 test!
  if (!sensor.begin()) {
    Serial.println("Did not find Si7021 sensor!");
    while (true)
      ;
  }
}

void loop() {
  // io.run(); is required for all sketches. It should always be present at the top of your loop
  // function. it keeps the client connected to io.adafruit.com, and processes any incoming data.
  io.run();

  // prints humidity and temperature data gathered from the Si7021 sensor
  Serial.print("Humidity (%): ");
  Serial.print(sensor.readHumidity(), 2); // 2 decimal places
  Serial.print("\tTemperature (C): ");
  Serial.println(sensor.readTemperature(), 2);

  humidity->save(sensor.readHumidity(), 2); // save city humidity to Adafruit IO
  delay(5000);
}

// this function is called whenever an 'led' feed message is received from Adafruit IO. it was attached to
// the 'led' feed in the setup() function above.
void handleMessage(AdafruitIO_Data *data) {
  Serial.print("received <- ");
 
  if(data->toPinLevel() == HIGH)
    Serial.println("HIGH"); // if feed send a 1 print
  else
    Serial.println("LOW"); // if feed sends a 0 print
 
  digitalWrite(LED_PIN, data->toPinLevel()); // write the current state to the led (1 = on and 0 = off)
}

void getMet() { 
  HTTPClient theClient;
  Serial.println("Making getMet HTTP request");
  theClient.begin(String("http://api.openweathermap.org/data/2.5/weather?q=") + city + "&units=imperial&appid=" + weatherKey);//return weather as .json object
  int httpCode = theClient.GET();

  //checks wether got an error while trying to access the website/API url
  if (httpCode > 0) {
    if (httpCode == 200) {
      Serial.println("Received getMet HTTP payload.");
      String payload = theClient.getString();
      DynamicJsonBuffer jsonBuffer;
      Serial.println("Parsing getMet...");
      JsonObject& root = jsonBuffer.parseObject(payload);

      // Test if parsing succeeds.
      if (!root.success()) {
        Serial.println("parseObject() failed in getMet().");
        return;
      }
      //collects values from JSON keys and stores them as strings because the slots in MetData are strings
      conditions.hd = root["main"]["humidity"].as<String>();
      conditions.tp = root["main"]["temp"].as<String>();
      city_humidity->save(conditions.hd); // save city humidity to Adafruit IO
      city_temp->save(conditions.tp); // save city temp to Adafruit IO
    }
  }
  else {
    Serial.printf("Something went wrong with connecting to the endpoint in getMet().");//prints the statement in case of failure connecting to the end point
  }
}