#include <PubSubClient.h>
#include <WiFi.h>
#include <Keypad.h>
#include "Freenove_WS2812_Lib_for_ESP32.h"

const char* ssid = "hotspot";
const char* password = "password";
const char* mqtt_server = "192.168.50.23";

WiFiClient espClient;
PubSubClient client(espClient);

char keys[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[4] = {14, 27, 26, 25}; // connect to the row pinouts of the keypad
byte colPins[4] = {13, 21, 22, 23};   // connect to the column pinouts of the keypad
Keypad myKeypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 4);

//For circle led strip
Freenove_ESP32_WS2812 strip = Freenove_ESP32_WS2812(8, 19, 0, TYPE_GRB);
int m_color[2][3] = {{255, 0, 0}, {0, 0, 255}};

//For middle LED
const byte ledPins[] = {15, 2, 4};    //define red, green, blue led pins
const byte chns[] = {0, 1, 2};        //define the pwm channels

void setupWifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.print(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  setupWifi();
  
  strip.begin();
  strip.setBrightness(10);

  for (int i = 0; i < 3; i++) {   //setup the pwm channels,1KHz,8bit
    ledcSetup(chns[i], 1000, 8);
    ledcAttachPin(ledPins[i], chns[i]);
  }
  ledcWrite(chns[0], 255);
  ledcWrite(chns[1], 255);
  ledcWrite(chns[2], 255);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("tictactoe/player1/move");
      client.subscribe("tictactoe/player2/move");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setLED(int pos, int color) {
  //translate game pos to led pos
  if (pos == 0) pos = 7;
  else if (pos == 1) pos = 0;
  else if (pos == 2) pos = 1;
  else if (pos == 3) pos = 6;
  //different rgb led in middle
  else if (pos == 4) {
    ledcWrite(chns[0], 255 - m_color[color][2]); //Common anode LED, low level to turn on the led.
    ledcWrite(chns[1], 255 - m_color[color][1]);
    ledcWrite(chns[2], 255 - m_color[color][0]);
    return;
  }
  else if (pos == 5) pos = 2;
  else if (pos == 6) pos = 5;
  else if (pos == 7) pos = 4; 
  else if (pos == 8) pos = 3;
 
  strip.setLedColorData(pos, m_color[color][0], m_color[color][1], m_color[color][2]);
  strip.show();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  char keyPressed = myKeypad.getKey();
  if (keyPressed) {
    client.publish("tictactoe/player1/input", &keyPressed);
  }
}

void callback(char* topic, byte* message, unsigned int length) {  
  if (strcmp(topic, "tictactoe/player1/move") == 0) {
    setLED(atoi((char *) message), 0);
  } else if (strcmp(topic, "tictactoe/player2/move") == 0) {
     setLED(atoi((char *) message), 1);
  }
}
