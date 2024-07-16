#include <BluetoothSerial.h>
#include <DHT.h>

#define LED             13 // connection indicator

/* DHT22 */
#define DHT_PIN         33
#define DHT_TYPE        DHT22
DHT dht(DHT_PIN, DHT_TYPE);

/* Bluetooth interface */
#define BT_NAME         "SensorNode"
BluetoothSerial SerialBT;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED, OUTPUT); digitalWrite(LED, LOW);
  Serial.begin(115200);
  SerialBT.begin(BT_NAME);

  dht.begin();

  Serial.println(F("BT device name: " BT_NAME));
  Serial.print(F("BT device address: ")); Serial.println(SerialBT.getBtAddressString());
}

#define UPDATE_INTERVAL               5000

void loop() {
  // put your main code here, to run repeatedly:
  if(!SerialBT.connected()) {
    digitalWrite(LED, LOW);
    Serial.println(F("Waiting for connection..."));
    while(!SerialBT.connected());
  } else {
    digitalWrite(LED, HIGH);
    Serial.println(F("Connected"));
    while(SerialBT.connected()) {
      float hum = dht.readHumidity(), temp = dht.readTemperature();
      Serial.printf_P(PSTR("Temp: %.2f C, humidity: %.2f %%\r\n"), temp, hum);
      SerialBT.printf_P(PSTR("%.2f,%.2f\n"), temp, hum);
      delay(UPDATE_INTERVAL);
    }
  }
}
