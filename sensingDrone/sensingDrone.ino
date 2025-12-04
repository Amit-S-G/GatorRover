#include <driver/i2s.h>
#include "esp_camera.h"
#include "camera_pins.h"
#include "secrets.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
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

// Camera settings
#define PHOTO_RATE_MS 2000        // Photo capture interval

// NRF24L01 radio pins
#define CE 4                      // Chip Enable
#define CSN 5                     // Chip Select

// Microphone state
int16_t sBuffer[bufferLen];       // Audio sample buffer
unsigned long windowStart = 0;    // Current window start time
uint32_t sumAbs = 0;              // Accumulated amplitude
uint32_t sampleCount = 0;         // Samples in current window

// Camera state
unsigned long last_photo_send = 0;
WiFiClientSecure client;

// Motor control
RH_NRF24 nrf24(CE, CSN);

// H-bridge pins (changed from GPIO 0,2 to avoid camera conflicts)
const int in1_left = 14;
const int in2_left = 15;
const int in1_right = 13;
const int in2_right = 12;

// Motor states: -1 = reverse, 0 = stop, 1 = forward
int leftWheel = 0;
int rightWheel = 0;

// Camera configuration for ESP32-CAM (OV2640 sensor)
static camera_config_t camera_config = {
    .pin_pwdn  = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,
    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,
    .xclk_freq_hz = 20000000,     // 20MHz for OV2640
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG, // Server requires JPEG
    .frame_size = FRAMESIZE_SVGA,   // 800x600 resolution
    .jpeg_quality = 10,             // Lower = higher quality (0-63)
    .fb_count = 1,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY
};

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
        Serial.println("LOUD SOUND DETECTED!");
      }
    }
    
    // Reset for next window
    windowStart = millis();
    sumAbs = 0;
    sampleCount = 0;
  }
}

// Power up and initialize camera
esp_err_t camera_init(){
    // Power down pin control (if available)
    if(CAM_PIN_PWDN != -1){
        pinMode(CAM_PIN_PWDN, OUTPUT);
        digitalWrite(CAM_PIN_PWDN, LOW);
    }
    
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        Serial.println("Camera Init Failed");
        return err;
    }
    return ESP_OK;
}

// Capture image and POST to server via HTTPS
esp_err_t camera_capture(){
    // Get frame buffer from camera
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera Capture Failed");
        return ESP_FAIL;
    }
    
    Serial.println("Got image. Attempting to send..");
    
    // Send JPEG via HTTPS POST
    HTTPClient http;
    http.begin(client, SERVER_URL);
    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("Authorization", String("Bearer ") + ESP32_API_KEY);
    
    int httpResponseCode = http.POST(fb->buf, fb->len);
    if (httpResponseCode > 0) {
        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    } else {
        Serial.printf("Error sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
    Serial.println("Sent image!");
    
    // Return frame buffer to driver for reuse
    esp_camera_fb_return(fb);
    return ESP_OK;
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
  
  // Connect to WiFi for camera streaming
  Serial.print("Connecting to WiFi.");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }
  Serial.println(" Connected!");
  client.setInsecure();
  
  // Initialize camera
  Serial.println("Initializing camera...");
  camera_init();
  Serial.println("Camera ready!");
  
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
  
  last_photo_send = millis();
}

void loop() {
  // Continuously monitor microphone (non-blocking)
  process_microphone();
  
  // Capture and send photos at regular intervals
  if(millis() - last_photo_send > PHOTO_RATE_MS){
    last_photo_send = millis();
    camera_capture();
  }
  
  // Always check for and process motor commands
  do_wheel_update();
  
  delay(50); // Brief delay for system stability
}