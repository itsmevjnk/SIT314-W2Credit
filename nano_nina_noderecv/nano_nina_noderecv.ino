/* NINA ESP32 config (https://forum.arduino.cc/t/can-the-nano-iot-33-run-full-bluetooth-not-ble/626990/7):
 *  - Board: ESP32 Dev Module
 *  - Upload Speed: 115200
 *  - CPU Frequency: any
 *  - Flash Frequency: 40MHz
 *  - Flash Mode: DIO/DOUT
 *  - Flash Size: 2MB (16Mb)
 *  - Partition Scheme: Minimal (1.3MB APP/700KB SPIFFS)
 */

#include <BluetoothSerial.h>
BluetoothSerial SerialBT;

#include <WiFi.h>
#include <HTTPClient.h>

#define BT_MSTR_NAME                "NinaMaster" // the name for this device (master)
// #define BT_DEV_NAME                 "SensorNode" // the name of the device we want to connect to
#define BT_DEV_ADDRESS              {0x58, 0xBF, 0x25, 0x85, 0x29, 0x7E}

/* WiFi config - for relaying to server */
#define WIFI_SSID                   "YOUR_SSID"
#define WIFI_PASSWORD               "YOUR_WIFI_PASSWORD"
#define WIFI_HTTP_PATH              "http://YOUR_IP_ADDRESS:8088/sensor"

WiFiClient client;
HTTPClient http;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  SerialBT.begin(BT_MSTR_NAME, true);
  Serial.println(F("BT master name: " BT_MSTR_NAME));

  Serial.print(F("Waiting for BT connection."));
#ifdef BT_DEV_NAME
  bool connected = SerialBT.connect(BT_DEV_NAME); // TODO: add dropout recovery
#else
  uint8_t address[] = BT_DEV_ADDRESS;
  bool connected = SerialBT.connect(address); // TODO: add dropout recovery
#endif
  if(!connected) {
    while(!SerialBT.connected(10000)) Serial.print('.');
  }
  Serial.println();

  Serial.print(F("Waiting for WiFi connection."));
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println();
  http.begin(client, WIFI_HTTP_PATH);
  http.addHeader("Content-Type", "application/json");
  
  Serial.println(F("OK"));
  Serial.write('\x7F'); // signal to SAMD that we're ready
}

/* sensor data reception */
#define DATA_BUF_LEN                256
char data_buf[DATA_BUF_LEN + 1];
int data_i = 0;

void loop() {
  // put your main code here, to run repeatedly:
  // if(Serial.available()) SerialBT.write(Serial.read()); // unlikely we'll ever use this
  if(SerialBT.available()) {
    char c = SerialBT.read();
    Serial.write(c);
    if(c == '\n' || data_i >= DATA_BUF_LEN) {
      data_buf[data_i] = '\0'; // null terminate
      data_i = 0;
      
      /* process data */
      if(WiFi.status() == WL_CONNECTED) {
        float temp, hum; sscanf(data_buf, "%f,%f", &temp, &hum);
        sprintf_P(data_buf, PSTR("{\"temperature\":\"%.2f\",\"humidity\":\"%.2f\"}"), temp, hum);
        http.POST(data_buf);
        // http.end();
      }
    } else data_buf[data_i++] = c;
  } else delay(20); // give idle task time to run
}
