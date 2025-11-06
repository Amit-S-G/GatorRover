#include <SPI.h>
#include <RH_NRF24.h>

#define CE 4
#define CSN 5

RH_NRF24 nrf24(CE, CSN);

// Left Wheel H-bridge inputs
const int in1_left = 0;
const int in2_left = 15;

// Right Wheel H-bridge inputs
const int in1_right = 13; 
const int in2_right = 2;

// Motor control variables
int leftWheel = 0;   // -1 = backwards, 0 = stop, 1 = forwards
int rightWheel = 0;  // -1 = backwards, 0 = stop, 1 = forwards

void fromCode(uint8_t code, int &leftWheel, int &rightWheel) {
  leftWheel  = (code / 3) - 1;
  rightWheel = (code % 3) - 1;
}

void setup() {
  // Motor control pins
  pinMode(in1_left, OUTPUT);
  pinMode(in2_left, OUTPUT);
  pinMode(in1_right, OUTPUT);
  pinMode(in2_right, OUTPUT);
  
  // Initialize motors to stop
  digitalWrite(in1_left, LOW);
  digitalWrite(in2_left, LOW);
  digitalWrite(in1_right, LOW);
  digitalWrite(in2_right, LOW);
  
  Serial.begin(115200);
  while (!Serial);
  
  if (!nrf24.init()) Serial.println("init failed");
  if (!nrf24.setChannel(67)) Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm)) Serial.println("setRF failed");
  
  Serial.println("NRF24 Receiver Ready");
}

void loop() {
  // Check for incoming radio messages
  if (nrf24.available()) {
    uint8_t code;
    uint8_t len = sizeof(code);
    if (nrf24.recv(&code, &len)) {
      fromCode(code, leftWheel, rightWheel);
      
      Serial.print("Received - LEFT: ");
      Serial.print(leftWheel);
      Serial.print(" | RIGHT: ");
      Serial.println(rightWheel);
    }
  }
  
  // Control Left Wheel based on received command
  if (leftWheel == 1) {
    // forwards
    Serial.println("Left Wheel: forwards");
    digitalWrite(in1_left, HIGH);
    digitalWrite(in2_left, LOW);
  }
  else if (leftWheel == -1) {
    // backwards
    Serial.println("Left Wheel: backwards");
    digitalWrite(in1_left, LOW);
    digitalWrite(in2_left, HIGH);
  }
  else {
    // stop
    Serial.println("Left Wheel: stop");
    digitalWrite(in1_left, LOW);
    digitalWrite(in2_left, LOW);
  }
  
  // Control Right Wheel based on received command
  if (rightWheel == 1) {
    // forwards
    Serial.println("Right Wheel: forwards");
    digitalWrite(in1_right, HIGH);
    digitalWrite(in2_right, LOW);
  }
  else if (rightWheel == -1) {
    // backwards
    Serial.println("Right Wheel: backwards");
    digitalWrite(in1_right, LOW);
    digitalWrite(in2_right, HIGH);
  }
  else {
    // stop
    Serial.println("Right Wheel: stop");
    digitalWrite(in1_right, LOW);
    digitalWrite(in2_right, LOW);
  }
  
  delay(50); // Small delay for stability
}