#include <SPI.h>
#include <RH_NRF24.h>

#define CE 4
#define CSN 5

RH_NRF24 nrf24(CE,CSN);

const int analogVoltageRead_left = 35;
const int analogVoltageRead_right = 34;

// Adjust these values
int forwardThreshold = 30;
int backwardThreshold = 1800;

int toCode(int a, int b) {
  return (a + 1) * 3 + (b + 1);
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(analogVoltageRead_left, INPUT);
  pinMode(analogVoltageRead_right, INPUT);

  if (!nrf24.init()) Serial.println("init failed");
  if (!nrf24.setChannel(67)) Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm)) Serial.println("setRF failed");
}

void loop() {
  int voltage_left = analogRead(analogVoltageRead_left);
  int voltage_right = analogRead(analogVoltageRead_right);

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

  uint8_t code = (uint8_t)toCode(a,b);
  Serial.print("sending code: ");
  Serial.print(code);
  Serial.print("  a=");
  Serial.print(a);
  Serial.print("  b=");
  Serial.println(b);

  uint8_t len = sizeof(code);
  nrf24.setModeIdle();
  nrf24.send(&code, len);
  nrf24.waitPacketSent();
  
  // listen for loud noise detection
  nrf24.setModeRx();
  unsigned long start = millis();
  while (millis() - start < 20) {        // small 20ms window
    if (nrf24.available()) {
      uint8_t loud;
      uint8_t len = 1;
      if (nrf24.recv(&loud, &len)) {
        if (loud == 1) {
          Serial.println("‼ Drone reports TOO LOUD !");
        }
      }
    }
  }
  delay(80);
}
