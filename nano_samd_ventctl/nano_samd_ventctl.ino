#define LED_PROG              13 // for signalling we're in NINA passthrough mode
#define LED_VENT              16 // for simulating vent valve (must be PWM pin)

int baud = 115200; // serial baud rate

/* NINA passthrough */
#define NINA_PTHRU            14 // when shorted to ground triggers NINA passthrough mode
void nina_setup() {
  SerialNina.begin(baud); // for communication with onboard WiFi "chipset" - which is an ESP32 no more no less
  pinMode(NINA_PTHRU, INPUT_PULLUP);
  pinMode(LED_PROG, OUTPUT); digitalWrite(LED_PROG, LOW);
  pinMode(NINA_GPIO0, OUTPUT);
  pinMode(NINA_RESETN, OUTPUT);

  /* power-on reset */
  digitalWrite(NINA_GPIO0, HIGH);
  digitalWrite(NINA_RESETN, LOW);
  delay(100);
  digitalWrite(NINA_RESETN, HIGH);
  delay(100);

  if(!digitalRead(NINA_PTHRU)) {
    digitalWrite(LED_PROG, HIGH);
    Serial.println(F("NINA passthrough mode - serial I/O will be redirected to NINA chipset."));
    int rts = -1, dtr = -1;
    while(1) {
      if(rts != Serial.rts()) {
        // Serial.print("RTS"); Serial.println(Serial.rts());
        digitalWrite(NINA_RESETN, Serial.rts() ? LOW : HIGH);
        rts = Serial.rts();
      }

      if(dtr != Serial.dtr()) {
        // Serial.print("DTR"); Serial.println(Serial.dtr());
        digitalWrite(NINA_GPIO0, Serial.dtr() ? LOW : HIGH);
        dtr = Serial.dtr();
      }

      if(Serial.available()) SerialNina.write(Serial.read());
      if(SerialNina.available()) Serial.write(SerialNina.read());

      if(Serial.baud() != baud) {
        rts = dtr = -1;
        baud = Serial.baud();
        SerialNina.begin(baud); // change ESP32 baud rate
      }
    }
  } else {
    Serial.println(F("Pull NINA_PTHRU to low during reset/power on to enter NINA passthrough mode."));
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(baud);
  while(!Serial) {
    if(millis() > 1500) break; // so we don't wait forever for non-existent serial interface
  }

  pinMode(LED_VENT, OUTPUT); analogWrite(LED_VENT, 0);
  nina_setup();

  Serial.println(F("Waiting for NINA..."));
  while(true) {
    while(!SerialNina.available());
    if(SerialNina.read() == '\x7f') break;
  }
  Serial.println(F("System is ready"));
}

/* PID control (heating) simulation */
#define SETPOINT                    26.0
#define P                           50.0
void control(float temp) {
  float err = SETPOINT - temp;
  int output = 0;
  if(err > 0) output = P * err;
  if(output > 255) output = 255;
  Serial.print(F("Heater vent output: ")); Serial.println(output);
  analogWrite(LED_VENT, output);
}

void loop() {
  // put your main code here, to run repeatedly:
  while(!SerialNina.available()); // wait for incoming data
  float temp = SerialNina.parseFloat();
  float hum = SerialNina.parseFloat();
  while(SerialNina.read() != '\n'); // flush newline character out so it doesn't break subsequent parses
  Serial.print(F("Temperature: ")); Serial.print(temp); Serial.print(F(" C, humidity: ")); Serial.print(hum); Serial.println(F(" %"));
  control(temp);
}
