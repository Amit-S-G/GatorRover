#include <SPI.h>
#include <RH_NRF24.h>

#define CE 4
#define CSN 5
#define LED_PIN 2  // LED indicator for loud sound detection

RH_NRF24 nrf24(CE, CSN);

const int analogVoltageRead_left = 35;
const int analogVoltageRead_right = 34;

// Adjust these values
int forwardThreshold = 30;
int backwardThreshold = 1800;

// Convert motor directions to single byte code
int toCode(int a, int b) {
  return (a + 1) * 3 + (b + 1);
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  pinMode(analogVoltageRead_left, INPUT);
  pinMode(analogVoltageRead_right, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Start with LED off
  
  if (!nrf24.init()) Serial.println("init failed");
  if (!nrf24.setChannel(67)) Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm)) Serial.println("setRF failed");
  
  Serial.println("Controller Ready");
}

void loop() {
  // Read joystick positions
  int voltage_left = analogRead(analogVoltageRead_left);
  int voltage_right = analogRead(analogVoltageRead_right);
  
  // Determine motor directions
  int a;
  int b;
  
  if (voltage_left <= forwardThreshold) {
    a = 1;
  } else if (voltage_left >= backwardThreshold) {
    a = -1;
  } else {
    a = 0;
  }
  
  if (voltage_right <= forwardThreshold) {
    b = 1;
  } else if (voltage_right >= backwardThreshold) {
    b = -1;
  } else {
    b = 0;
  }
  
  // Send motor command
  uint8_t code = (uint8_t)toCode(a, b);
  Serial.print("Sending code: ");
  Serial.print(code);
  Serial.print("  a=");
  Serial.print(a);
  Serial.print("  b=");
  Serial.print(b);
  
  uint8_t len = sizeof(code);
  nrf24.send(&code, len);
  nrf24.waitPacketSent();
  
  // Wait for response with sound detection status
  delay(20);  // Small delay for drone to respond
  
  if (nrf24.available()) {
    uint8_t status;
    uint8_t statusLen = sizeof(status);
    if (nrf24.recv(&status, &statusLen)) {
      Serial.print("  | Sound status: ");
      Serial.println(status);
      
      // Control LED based on sound detection
      if (status == 1) {
        digitalWrite(LED_PIN, HIGH);  // Loud sound detected - LED ON
      } else {
        digitalWrite(LED_PIN, LOW);   // No loud sound - LED OFF
      }
    }
  } else {
    Serial.println("  | No response");
  }
  
  delay(60);  // Total loop time ~80ms
}