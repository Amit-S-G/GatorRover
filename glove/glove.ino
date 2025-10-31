// Left Wheel H-bridge inputs
const int in1_left = 2;
const int in2_left = 15;

// Right Wheel H-bridge inputs
const int in1_right = 4;
const int in2_right = 16;

// Analog voltage read pins for flex resistors
const int analogVoltageRead_left = 35;
const int analogVoltageRead_right = 34;

// Voltage variables
int voltage_left = 0;
int voltage_right = 0;

// Threshold values for flex sensor control
const int forwardThreshold = 30;    // Below this = forwards
const int backwardThreshold = 1100; // Above this = backwards
// Between these values = stop/dead zone

void setup() {
  // Left wheel pins
  pinMode(analogVoltageRead_left, INPUT);
  pinMode(in1_left, OUTPUT);
  pinMode(in2_left, OUTPUT);
  
  // Right wheel pins
  pinMode(analogVoltageRead_right, INPUT);
  pinMode(in1_right, OUTPUT);
  pinMode(in2_right, OUTPUT);
  
  Serial.begin(115200);
}

void loop() {
  // Read voltages from both sensors
  voltage_left = analogRead(analogVoltageRead_left);
  voltage_right = analogRead(analogVoltageRead_right);
  
  // Control Left Wheel
  if (voltage_left <= forwardThreshold){
    // forwards
    Serial.println("Left Wheel: forwards");
    digitalWrite(in1_left, HIGH);
    digitalWrite(in2_left, LOW);
  }
  else if (voltage_left >= backwardThreshold){
    // backwards
    Serial.println("Left Wheel: backwards");
    digitalWrite(in1_left, LOW);
    digitalWrite(in2_left, HIGH);
  }
  else{
    // stop
    Serial.println("Left Wheel: stop");
    digitalWrite(in1_left, LOW);
    digitalWrite(in2_left, LOW);
  }
  
  // Control Right Wheel
  if (voltage_right <= forwardThreshold){
    // forwards
    Serial.println("Right Wheel: forwards");
    digitalWrite(in1_right, HIGH);
    digitalWrite(in2_right, LOW);
  }
  else if (voltage_right >= backwardThreshold){
    // backwards
    Serial.println("Right Wheel: backwards");
    digitalWrite(in1_right, LOW);
    digitalWrite(in2_right, HIGH);
  }
  else{
    // stop
    Serial.println("Right Wheel: stop");
    digitalWrite(in1_right, LOW);
    digitalWrite(in2_right, LOW);
  }
  
  delay(50); // Small delay to prevent overwhelming serial output
}