// camera info
#include "esp_camera.h"
#include "camera_pins.h"
#include "secrets.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#define PHOTO_RATE_MS 5000

// wheel info
#include <SPI.h>
#include <RH_NRF24.h>

#define CE 4
#define CSN 5


// WARNING:
// YOU HAVE TO USE A DIFFERENT VERSION OF "esp32" FOR THIS CAMERA
// TO WORK. DOWNGRADE FROM 3.3.0 to 3.2.0

// ------------ CAMERA SETUP ------------------
unsigned long last_photo_send = millis();
WiFiClientSecure client;

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

    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, // server REQUIRES jpeg format
    .frame_size = FRAMESIZE_SVGA, //QQVGA through UXGA (any 4:3 aspect ratio). SVGA is 800 x 600

    .jpeg_quality = 10, //0-63 scale where lower == higher quality
    .fb_count = 1,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY
};

esp_err_t camera_init(){
    //power up the camera if PWDN pin is defined
    if(CAM_PIN_PWDN != -1){
        pinMode(CAM_PIN_PWDN, OUTPUT);
        digitalWrite(CAM_PIN_PWDN, LOW);
    }

    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

esp_err_t camera_capture(){
    //acquire a frame
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera Capture Failed");
        return ESP_FAIL;
    }

    Serial.println("Got image. Attempting to send..");

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

    esp_camera_fb_return(fb);
    return ESP_OK;
}

// ------------ WHEEL SETUP ------------------
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

void do_wheel_update(){
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
}

void setup() {

  // CAMERA SETUP -------------------------------------
  Serial.begin(115200);

  Serial.print("Connecting to WiFi.");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }
  Serial.println();

  client.setInsecure();

  camera_init();
  
  
  // MOTOR SETUP -------------------------------------
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
  delay(50);
  while(!nrf24.init())
  if (!nrf24.setChannel(67)) Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm)) Serial.println("setRF failed");
  
  Serial.println("NRF24 Receiver Ready");
}

void loop() {
  if(millis() - last_photo_send > PHOTO_RATE_MS){ // only send a photo every [rate] milliseconds
    last_photo_send = millis();
    camera_capture();
  }
  do_wheel_update(); // always do a wheel update
  delay(50); // pause for 1/20th second for motor stability
}
