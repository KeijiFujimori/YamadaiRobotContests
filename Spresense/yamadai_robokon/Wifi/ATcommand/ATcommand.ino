#define BAUDRATE 115200

void setup() {
  Serial.begin(BAUDRATE);
  Serial2.begin(BAUDRATE);
  while(!Serial);
  while(!Serial2);
}

void loop() {
  if (Serial.available())
    Serial2.write(Serial.read());

  if (Serial2.available())
    Serial.write(Serial2.read());

}
