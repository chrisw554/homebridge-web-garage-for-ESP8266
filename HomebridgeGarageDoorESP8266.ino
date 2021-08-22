//  Code for ESP8266 to work with the Homebridge Web Garage Plugin and operate a garage door. Using an ESP8266 board such as D1 Mini, Relay, and two reed switches: 
//  https://github.com/Tommrodrigues/homebridge-web-garage 


//Libraries
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

//
// Set your variables here...
//

// Wifi Info
const char* ssid = "ssid";
const char* password = "password";

//Homebridge URL
const char* homebridgeURL = "http://192.168.1.2:2000"; // Include port number - one per garage door opener. Example: "http://192.168.1.2:2000"


// GPIO assignments
#define PIN_OPERATOR_CONTROL 5 // Relay
#define PIN_SENSOR_CLOSED     14 // Sensor at bottom of track
#define PIN_SENSOR_OPENED     12 // Sensor at top of track

//
// That's all folks..
//



// Variables
int currentDoorState = 1;  // 0 Open 1 Closed 2 Opening 3 Closing
int targetDoorState = 1;
bool notifyHomebridge = true; //set to true to begin so Homebridge is notified wether the door is open or closed on starting
bool notifyHomebridgeTargetDoorState = true;
bool sensorInterrupt = false;
bool targetDoorStateChanged = false;


// Interrupt handler - Interrupts cannot be stored in flash; tell the compiler to put it in RAM
void ICACHE_RAM_ATTR handle_sensor_change(); 
// When one of the pins changes, toggle a variable to TRUE, so we can respond inside the main loop()
void handle_sensor_change() {
  sensorInterrupt = true;
}
  

// Begin Wifi Server
WiFiServer server(80);


//
// Setup
//
 
void setup() {
  Serial.begin(115200);
  delay(10);
  
  //Set Pin Modes
  pinMode(PIN_SENSOR_CLOSED, INPUT_PULLUP); // Set the sensor pins to input with pullup resistor
  pinMode(PIN_SENSOR_OPENED, INPUT_PULLUP); // 
  pinMode(PIN_OPERATOR_CONTROL, OUTPUT); // Set the control pin to output

  // Set interrupts to watch for changes in the open/close sensors
  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_CLOSED), handle_sensor_change, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR_OPENED), handle_sensor_change, CHANGE);
 
  // Connect to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 
  // Start the server
  server.begin();
  Serial.print("Server started: ");

  // Print the IP address
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  //Determine the state of the door on boot, assumes the door is either open or closed & not moving when this is run, if the door isn't open it is assumed to be closed:
  if ( digitalRead(PIN_SENSOR_OPENED) == LOW ) {
    // If PIN_SENSOR_OPENED is low, it's being pulled to ground, which means the switch at the top of the track is closed, which means the door is open
    Serial.println("Start Up Position Check: Door is open");
    currentDoorState = 0;
    targetDoorState = 0;
  } 
}


//
// Loop
//
 
void loop() {
 
  // Check if a client has connected
  WiFiClient client = server.available();

   //to notify homebridge of the current state & target state
   if (notifyHomebridge == true)  {
      notifyHomebridge = false;
      String notifyURL = String(homebridgeURL) + "/currentDoorState?value=" + String(currentDoorState);
      Serial.print("Notifying Homebridge of currentDoorState:");
      Serial.println(notifyURL); 
      //run the GET request
      HTTPClient http;
      http.begin(client,notifyURL);
      int httpResponseCode = http.GET();
      http.end();

  }
  if (notifyHomebridgeTargetDoorState == true)  {
      notifyHomebridgeTargetDoorState = false;
      String notifyURLtargetDoorState = String(homebridgeURL) + "/targetDoorState?value=" + String(targetDoorState);
      Serial.print("Notifying Homebridge of targetDoorState:");
      Serial.println(notifyURLtargetDoorState); 
      //run the GET request
      HTTPClient http;
      http.begin(client,notifyURLtargetDoorState);
      int httpResponseCode = http.GET();
      http.end();
  }
  if (!client && sensorInterrupt == false) {    //Loop execution halts here until these conditions ARE NOT met
    return;
  }
  if (client) {
    // Wait until the client sends some data
    Serial.println("New Client");
    while(!client.available()){
      delay(1);
    }
  
   if (client.available()) {
      // Read the first line of the request
      String request = client.readStringUntil('\r');
      Serial.println(request);
      client.flush();  
    
      if (request.indexOf("/setTargetDoorState?value=0") != -1)  {
        targetDoorState = 0;
        targetDoorStateChanged = true;
      }
    
      else if (request.indexOf("/setTargetDoorState?value=1") != -1)  {
        targetDoorState = 1;
        targetDoorStateChanged = true;
      }

      //If the targetDoorState has changed, check to see if the door should be triggered
      if (targetDoorStateChanged == true) {
        targetDoorStateChanged = false;

        if (currentDoorState != targetDoorState) {
          Serial.println("Door Operation Triggered");
          digitalWrite(PIN_OPERATOR_CONTROL, HIGH);
          delay(500); 
          digitalWrite(PIN_OPERATOR_CONTROL, LOW);
          }
        }
      
      //We must return something or Homekit displays Not Responding
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println(""); //  do not forget this one
    
      if (request.indexOf("/status") != -1)  {
        client.print("{ \"currentDoorState\": ");
        client.print(currentDoorState);
        client.print(", \"targetDoorState\": ");
        client.print(targetDoorState);
        client.println(" }");
      }
        
      delay(1);
      Serial.println("Client disonnected");
      Serial.println("");

    }
  }
  if (sensorInterrupt == true) { 
    sensorInterrupt = false;
    Serial.println("Sensor Interrupt triggered");
    
    // Read the sensors and use some logic to determine state
    if ( digitalRead(PIN_SENSOR_OPENED) == LOW ) {
      // If PIN_SENSOR_OPENED is low, it's being pulled to ground, which means the switch at the top of the track is closed, which means the door is open
      Serial.println("PIN_SENSOR_OPENED is LOW meaning Switch at top of track is closed");
      currentDoorState = 0;
    } 
    if ( digitalRead(PIN_SENSOR_CLOSED) == LOW ) {
      // If PIN_SENSOR_CLOSED is low, it's being pulled to ground, which means the switch at the bottom of the track is closed, which means the door is closed
      Serial.println("PIN_SENSOR_CLOSED is LOW meaning Switch at bottom of track is closed");
      currentDoorState = 1;
    } else {
      // If neither, then the door is in between switches, so we use the last known state to determine which way it's probably going
      if (currentDoorState == 1) {
        currentDoorState = 2; // Current door state was "closed" so we are probably now "opening"
      } else if ( currentDoorState == 0 ) {
        currentDoorState = 3; // Current door state was "opened" so we are probably now "closing"
      }
    
      // If it is traveling, then it might have been started by the button in the garage. Set the new target state:
      if ( currentDoorState == 2 ) {
        targetDoorState = 0;
      } else if ( currentDoorState == 3 ) {
        targetDoorState = 1;
      }
      
    }
    // ... and finaly notify homebridge
      notifyHomebridge = true;
      notifyHomebridgeTargetDoorState = true;
  }
}
