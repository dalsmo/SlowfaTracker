//Click here to get the library: http://librarymanager/All#SparkFun_LTE_Shield_Arduino_Library
#include <SparkFun_LTE_Shield_Arduino_Library.h>
// Create a SoftwareSerial object to pass to the LTE_Shield library
SoftwareSerial lteSerial(8, 9);
// Create a LTE_Shield object to use throughout the sketch
LTE_Shield lte;

// These values should remain the same:
const char HOLOGRAM_URL[] = "soffan.freaks.se";
const unsigned int HOLOGRAM_PORT = 443;

//gps stuff
boolean requestingGPS = false;
unsigned long lastRequest = 0;
#define MIN_REQUEST_INTERVAL 60000 // How often we'll get GPS in loop (in ms)
#define GPS_REQUEST_TIMEOUT 30 // Time to turn on GPS module and get a fix (in s)
#define GPS_REQUEST_ACCURACY 1  // Desired accuracy from GPS module (in meters)

//restart stuf
#include <Adafruit_SleepyDog.h>

int dots=0;

void processGpsRead(ClockData clck, PositionData gps, 
  SpeedData spd, unsigned long uncertainty) {

  Serial.println();
  Serial.println();
  Serial.println(F("GPS Data Received"));
  Serial.println(F("================="));
  Serial.println("Date: " + String(clck.date.month) + "/" + 
    String(clck.date.day) + "/" + String(clck.date.year));
  Serial.println("Time: " + String(clck.time.hour) + ":" + 
    String(clck.time.minute) + ":" + String(clck.time.second) + "." + String(clck.time.ms));
  Serial.println("Lat/Lon: " + String(gps.lat, 7) + "/" + String(gps.lon, 7));
  Serial.println("Alt: " + String(gps.alt));
  Serial.println("Uncertainty: " + String(uncertainty));
  Serial.println("Speed: " + String(spd.speed) + " @ " + String(spd.track));
  Serial.println();

  // something new
  Watchdog.reset();
  sendHologramMessage(gps.lat,gps.lon);
  Watchdog.reset();
  dots=0;
  requestingGPS = false;
}

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println(F("------------------starting!!!-----------------------"));
  
  if ( lte.begin(lteSerial, 9600) ) {
    Serial.println(F("LTE Shield connected!"));
  }
  // Set a callback to return GPS data once requested
  lte.setGpsReadCallback(&processGpsRead);

  Watchdog.enable(10000);// reset after mills
}

void loop() {
  // Poll as often as possible
  lte.poll();
  Watchdog.reset();
  
  if (!requestingGPS) {
    if ((lastRequest == 0) || (lastRequest + MIN_REQUEST_INTERVAL < millis())) {
        Serial.println(F("Requesting GPS data...this can take up to 10 seconds"));
        if (lte.gpsRequest(GPS_REQUEST_TIMEOUT, GPS_REQUEST_ACCURACY) == LTE_SHIELD_SUCCESS) {
          Serial.println(F("GPS data requested."));
          Serial.println("Wait up to " + String(GPS_REQUEST_TIMEOUT) + " seconds");
          requestingGPS = true;
          lastRequest = millis();
        } else {
          Serial.println(F("Error requesting GPS"));
          dots++;
        }
      }
  } else {
    // Print a '.' every ~1 second if requesting GPS data
    // (Hopefully this doesn't mess with poll too much)
    if ((millis() % 1000) == 0) {
      Serial.print('.');
      dots++;

      delay(1);
    }
  }
  
  if(dots>100){
        while(1){}
      }


}

void sendHologramMessage(float lat, float lon)
{
  int socket = -1;
  String hologramMessage;
  Serial.println(F("attempting to send hologram message"));

  // Construct a JSON-encoded Hologram message string:
  hologramMessage = "{\"trackerId\":\"slowfa\",\"lat\":" + String(lat,7) + ",\"long\":" +
    String(lon,7) + "}";
  
  // Open a socket
  socket = lte.socketOpen(LTE_SHIELD_TCP);
  // On success, socketOpen will return a value between 0-5. On fail -1.
  if (socket >= 0) {
    // Use the socket to connec to the Hologram server
    Serial.println("Connecting to socket: " + String(socket));
    if (lte.socketConnect(socket, HOLOGRAM_URL, HOLOGRAM_PORT) == LTE_SHIELD_SUCCESS) {
      // Send our message to the server:
      Serial.println("Sending: " + String(hologramMessage));
      if (lte.socketWrite(socket, hologramMessage) == LTE_SHIELD_SUCCESS)
      {
        // On succesful write, close the socket.
        if (lte.socketClose(socket) == LTE_SHIELD_SUCCESS) {
          Serial.println("Socket " + String(socket) + " closed");
        }
      } else {
        Serial.println(F("Failed to write"));
      }
    }
  }
}
