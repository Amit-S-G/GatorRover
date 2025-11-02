#include <WiFi.h>

const char* ssid = "GatorRover";
const char* password = "12345678";

WiFiServer server(8080);

const int in1_left = 0;
const int in2_left = 15;
const int in1_right = 4;
const int in2_right = 2;

const int forwardThreshold = 30;
const int backwardThreshold = 1100;

int L = 0;
int R = 0;

void setup() {
  Serial.begin(115200);

  pinMode(in1_left, OUTPUT);
  pinMode(in2_left, OUTPUT);
  pinMode(in1_right, OUTPUT);
  pinMode(in2_right, OUTPUT);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Hotspot started! IP Address: ");
  Serial.println(IP);
  server.begin();
  Serial.println("Server started, waiting for glove");
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Glove connected");
    while (client.connected()) {
      if (client.available()) {
        String data = client.readStringUntil('\n');
        data.trim();
        int lIndex = data.indexOf("L:");
        int rIndex = data.indexOf("R:");
        if (lIndex >= 0 && rIndex > lIndex) {
          L = data.substring(lIndex + 2, rIndex).toInt();
          R = data.substring(rIndex + 2).toInt();
          Serial.printf("Received → L:%d  R:%d\n", L, R);
        }

        if (L <= forwardThreshold) {
          Serial.println("Left Wheel: forwards");
          digitalWrite(in1_left, HIGH);
          digitalWrite(in2_left, LOW);
        } else if (L >= backwardThreshold) {
          Serial.println("Left Wheel: backwards");
          digitalWrite(in1_left, LOW);
          digitalWrite(in2_left, HIGH);
        } else {
          Serial.println("Left Wheel: stop");
          digitalWrite(in1_left, LOW);
          digitalWrite(in2_left, LOW);
        }

        if (R <= forwardThreshold) {
          Serial.println("Right Wheel: forwards");
          digitalWrite(in1_right, HIGH);
          digitalWrite(in2_right, LOW);
        } else if (R >= backwardThreshold) {
          Serial.println("Right Wheel: backwards");
          digitalWrite(in1_right, LOW);
          digitalWrite(in2_right, HIGH);
        } else {
          Serial.println("Right Wheel: stop");
          digitalWrite(in1_right, LOW);
          digitalWrite(in2_right, LOW);
        }
      }
    }
    client.stop();
    Serial.println("Glove disconnected.");
  }
}
