/*
  Weer Web Server

  A simple web server that shows the temperature.
  using an Arduino Wiznet Ethernet shield.

  Circuit:
   Ethernet shield attached to pins 10
   BME 280 Sensor

  25 May 2019
  by Marijn Jansen
  
  Based on Web Server Example by:
  created 18 Dec 2009
  by David A. Mellis
  modified 9 Apr 2012
  by Tom Igoe
  modified 02 Sept 2015
  by Arturo Guadalupi


*/

#include <SPI.h>
#include <Ethernet.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define I2C_ADDRESS 0x76
#define MINUTE 60000


// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[6] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

Adafruit_BME280 bme;
String request_header;

unsigned long read_time;

void setup_BME280();
void setup_ethernet();

void send_header(EthernetClient client);

void setup() {
  request_header = "";
  
  Ethernet.init(10);
  
  Serial.begin(115200);
  Serial.println(F("BME280 REST webserver"));

  // start the Ethernet connection:
  setup_ethernet();

  // Setup BME280
  setup_BME280();

  // start the server
  server.begin();
  
  Serial.print(F("Server is at "));
  Serial.println(Ethernet.localIP());
}

void setup_ethernet() {
  Serial.println(F("Initialize Ethernet with DHCP:"));
  
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed to configure Ethernet using DHCP"));
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println(F("Ethernet shield was not found.  Sorry, can't run without hardware. :("));
    } else if (Ethernet.linkStatus() == Unknown) {
      Serial.println(F("Link status unknown. Link status detection is only available with W5200 and W5500."));
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println(F("Ethernet cable is not connected."));
    }
    while (true) {
      delay(1);
    }
  }
}

void setup_BME280() {
  bool bme_status;
  bme_status = bme.begin(I2C_ADDRESS);
  if (!bme_status) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    while (true) {
      delay(1);
    }
  }

  Serial.println(F("BME280 Connected"));

  Serial.println(F("-- Weather Station Scenario --"));
  Serial.println(F("forced mode, 1x temperature / 1x humidity / 1x pressure oversampling,"));
  Serial.println(F("filter off"));
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF);

  bme.takeForcedMeasurement();
}

void measure() {
  unsigned long current_time = millis();
  if (abs(current_time - read_time) > MINUTE) {
    Serial.println(F("New measurement"));
    read_time = current_time;
    bme.takeForcedMeasurement();
  }
}

void send_json(EthernetClient client) {
  send_header(client);

  client.print("{");
  client.print("\n\t\"temperature\": ");
  client.print(bme.readTemperature());
  client.print(",\n\t\"pressure\"   : ");
  client.print(bme.readPressure() / 100.0F);
  client.print(",\n\t\"humidity\"   : ");
  client.print(bme.readHumidity());
  client.print("\n}");
  client.println();
}

void send_temperature(EthernetClient client) {
  send_header(client);
  client.println(bme.readTemperature());
}

void send_hum(EthernetClient client) {
  send_header(client);
  client.println(bme.readHumidity());
}

void send_press(EthernetClient client) {
  send_header(client);
  client.println(bme.readPressure() / 100.0F);
}

void send_header(EthernetClient client) {
    // send a standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
}

void send_404(EthernetClient client) {
  client.println("HTTP/1.1 404 Not Found");
  client.println("Connection: close");
  client.println();
}

void loop() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    measure();
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;

    boolean jsonRequest = false;
    boolean tempRequest = false;
    boolean humRequest = false;
    boolean pressRequest = false;
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request_header += c;
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          if (jsonRequest) {
            send_json(client);
          } else if (tempRequest) {
            send_temperature(client);
          } else if (humRequest) {
            send_hum(client);
          } else if (pressRequest) {
            send_press(client);
          } else {
            send_404(client);
          }
          break;
        }
        
        if (c == '\n') {
          // you're starting a new line
          if (request_header == "GET / HTTP/1.1\r\n") {
            jsonRequest = true;
          } else if (request_header == "GET /temp HTTP/1.1\r\n") {
            tempRequest = true;
          } else if (request_header == "GET /hum HTTP/1.1\r\n") {
            humRequest = true;
          } else if (request_header == "GET /press HTTP/1.1\r\n") {
            pressRequest = true;
          }
          
          request_header = "";
          
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println(F("client disconnected"));
    request_header = "";
  }
}
