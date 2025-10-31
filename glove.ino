// in1 and in2 are inputs to the h-bridges
const int in1 = 2;
const int in2 = 15;
// analogReadVoltage is where we read the voltage from for the flex resistor
const int analogVoltageRead = 35;
// voltage will initially start at 0
int voltage = 0;

void setup() {
  pinMode(analogVoltageRead, INPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  // read voltage
  voltage = analogRead(analogVoltageRead);
  // if voltage across 220 ohm resistor is low, the flex resistor is straight/high resistance mode
  if (voltage <= 30){
      // forwards
    Serial.println("forwards");
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  }
  // if voltage across 220 ohm resistor is high-ish, the flex resistor is bent/low resistance mode
  else if (voltage >= 1100){
    // backwards
    Serial.println("backwards");
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  }
  // if it is neither high nor low, it's in the dead/stop zone
  else{
    // stop
    Serial.println("stop");
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }
  // next steps, user this to control the motors, and then figure out how to send voltage from one esp to another
  }

