#include "esp_camera.h"
#include "camera_pins.h"
#include "secrets.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// WARNING:
// YOU HAVE TO USE A DIFFERENT VERSION OF "esp32" FOR THIS CAMERA
// TO WORK. DOWNGRADE FROM 3.3.0 to 2.0.17

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




void setup() {
  Serial.begin(115200);
  client.setInsecure();

  Serial.print("Connecting to WiFi.");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }

  Serial.println();
  camera_init();

}



void loop() {
  camera_capture();
  delay(500); // send a frame every 5 seconds
}