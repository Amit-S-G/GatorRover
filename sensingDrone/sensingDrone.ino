/*
 * Integrated Drone Control System
 * 
 * Combines two subsystems on a single ESP32:
 * 1. I2S microphone for sound detection
 * 2. NRF24L01 wireless receiver for motor control (with sound status feedback)
 */

#include <driver/i2s.h>
#include <SPI.h>
#include <RH_NRF24.h>

// Microphone I2S pins (INMP441 or similar)
#define I2S_WS 25                 // Word Select (LRCLK)
#define I2S_SD 33                 // Serial Data (DOUT)
#define I2S_SCK 32                // Serial Clock (BCLK)
#define I2S_PORT I2S_NUM_0

// Audio processing settings
#define bufferLen 64              // Samples per DMA buffer
#define THRESHOLD 350             // Amplitude threshold for loud sound detection
#define WINDOW_MS 500             // Averaging window in milliseconds

// NRF24L01 radio pins
#define CE 4                      // Chip Enable
#define CSN 5                     // Chip Select

// Microphone state
int16_t sBuffer[bufferLen];       // Audio sample buffer
unsigned long windowStart = 0;    // Current window start time
uint32_t sumAbs = 0;              // Accumulated amplitude
uint32_t sampleCount = 0;         // Samples in current window
bool loudSoundDetected = false;   // Status flag for controller

// Motor control
RH_NRF24 nrf24(CE, CSN);

// Left Wheel H-bridge inputs
const int in1_left = 0;
const int in2_left = 15;
// Right Wheel H-bridge inputs
const int in1_right = 13;
const int in2_right = 2;

// Motor states: -1 = reverse, 0 = stop, 1 = forward
int leftWheel = 0;
int rightWheel = 0;

// Initialize I2S driver with audio configuration
void i2s_install(){
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = bufferLen,
    .use_apll = false
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

// Configure I2S pin mappings
void i2s_setpin(){
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,           // No output (mic only)
    .data_in_num = I2S_SD
  };
  i2s_set_pin(I2S_PORT, &pin_config);
}

// Read audio samples and detect loud sounds using rolling average
void process_microphone(){
  size_t bytesIn = 0;
  // Non-blocking read so we don't interfere with motor control
  esp_err_t result = i2s_read(I2S_PORT, &sBuffer, bufferLen * sizeof(int16_t), &bytesIn, 0);
  
  if (result == ESP_OK && bytesIn > 0) {
    int samples_read = bytesIn / sizeof(int16_t);
    // Accumulate absolute amplitude values
    for (int i = 0; i < samples_read; i++) {
      sumAbs += abs(sBuffer[i]);
      sampleCount++;
    }
  }
  
  // Check if window period has elapsed
  if (millis() - windowStart >= WINDOW_MS) {
    if (sampleCount > 0) {
      uint32_t averageAmplitude = sumAbs / sampleCount;
      Serial.print("Average amplitude: ");
      Serial.println(averageAmplitude);
      
      if (averageAmplitude > THRESHOLD) {
        Serial.println("🔊 LOUD SOUND DETECTED!");
        loudSoundDetected = true;
      } else {
        loudSoundDetected = false;
      }
    }
    
    // Reset for next window
    windowStart = millis();
    sumAbs = 0;
    sampleCount = 0;
  }
}

// Decode motor command: converts 0-8 code into motor directions
// Code = (left+1)*3 + (right+1), where left/right are -1,0,1
void fromCode(uint8_t code, int &leftWheel, int &rightWheel) {
  leftWheel  = (code / 3) - 1;
  rightWheel = (code % 3) - 1;
}

// Check for wireless commands and update motor outputs
void do_wheel_update(){
  // Check if data available from transmitter
  if (nrf24.available()) {
    uint8_t code;
    uint8_t len = sizeof(code);
    if (nrf24.recv(&code, &len)) {
      fromCode(code, leftWheel, rightWheel);
      
      Serial.print("Received - LEFT: ");
      Serial.print(leftWheel);
      Serial.print(" | RIGHT: ");
      Serial.println(rightWheel);
      
      // Send back sound detection status
      uint8_t status = loudSoundDetected ? 1 : 0;
      nrf24.send(&status, sizeof(status));
      nrf24.waitPacketSent();
      
      Serial.print("Sent sound status: ");
      Serial.println(status);
    }
  }
  
  // Update left motor H-bridge
  if (leftWheel == 1) {
    digitalWrite(in1_left, HIGH);
    digitalWrite(in2_left, LOW);
  }
  else if (leftWheel == -1) {
    digitalWrite(in1_left, LOW);
    digitalWrite(in2_left, HIGH);
  }
  else {
    digitalWrite(in1_left, LOW);
    digitalWrite(in2_left, LOW);
  }
  
  // Update right motor H-bridge
  if (rightWheel == 1) {
    digitalWrite(in1_right, HIGH);
    digitalWrite(in2_right, LOW);
  }
  else if (rightWheel == -1) {
    digitalWrite(in1_right, LOW);
    digitalWrite(in2_right, HIGH);
  }
  else {
    digitalWrite(in1_right, LOW);
    digitalWrite(in2_right, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize I2S microphone
  Serial.println("Setup I2S microphone...");
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
  windowStart = millis();
  Serial.println("Microphone ready!");
  
  // Setup motor control pins
  pinMode(in1_left, OUTPUT);
  pinMode(in2_left, OUTPUT);
  pinMode(in1_right, OUTPUT);
  pinMode(in2_right, OUTPUT);
  
  // Initialize motors to stopped state
  digitalWrite(in1_left, LOW);
  digitalWrite(in2_left, LOW);
  digitalWrite(in1_right, LOW);
  digitalWrite(in2_right, LOW);
  
  // Initialize NRF24 wireless receiver
  if (!nrf24.init()) Serial.println("NRF24 init failed");
  if (!nrf24.setChannel(67)) Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm)) Serial.println("setRF failed");
  
  Serial.println("NRF24 Receiver Ready");
  Serial.println("\nAll systems initialized!\n");
}

void loop() {
  // Continuously monitor microphone (non-blocking)
  process_microphone();
  
  // Always check for and process motor commands
  do_wheel_update();
  
  delay(50); // Brief delay for system stability
}