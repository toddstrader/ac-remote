// TODO - something up here

#define DEBUG

#include <string.h>
#include "FS.h"
#include <ESP8266WiFi.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "ir_thing.h"

#define CONFIG_FILE "config.txt"

#define ONBOARD_LED 5 // Active low
#define IR_LED 2 // Active high
#define ADC 0 // 1.0V max

#define THING_PORT 4178
#define MAX_PAYLOAD (8 * 1024)

WiFiServer server(THING_PORT);
IRsend irsend(IR_LED);

void setup() {
  initHardware();
  initWifi();
}

void loop() {
  configure();
  checkWifi();
  checkClient();
  delay(100);
}

int ir_send(char * params) {
  char * saveprtr;
  int freq = atoi(strtok_r(params, ",", &saveprtr));
  int len = atoi(strtok_r(NULL, ",", &saveprtr));
  
  uint16_t data [len];
  
  for (int i = 0; i < len; i++) {
    char * tok = strtok_r(NULL, ",", &saveprtr);
    if (tok == NULL) {
      Serial.println("Ran out of tokens in parameters");
      return -1;
    }
    // TODO -- check bounds on tok
    int val = atoi(tok);
    if (val > UINT16_MAX) {
      Serial.println("Value exceeds uint16_t in parameters");
      return -1;
    }
    data[i] = val;
  }

  if (len) {
    irsend.sendRaw(data, len, freq);
    return 0;
  } else {
    int adc = analogRead(ADC);
#ifdef DEBUG
    Serial.print("read ADC -> ");
    Serial.println(adc);
#endif
    return adc;
  }
}

void display_prompt(config_state_t state) {
  switch(state) {
    case CFG_NAME: Serial.println("Enter device name: "); break;
    case CFG_SSID: Serial.println("Enter SSID: "); break;
    case CFG_PASS: Serial.println("Enter WiFi password: "); break;
  }
}

void checkClient() {
  WiFiClient client = server.available();
  if (client) {
    // Allocating this on the stack causes an exception after the first pass (??)
    static char param_buff [MAX_PAYLOAD];
    char * curr = param_buff;
    if (client.connected()) {
      while (client.available() > 0) {
        char c = client.read();
#ifdef DEBUG
        Serial.write(c);
#endif
        *curr = c;

        if (++curr >= param_buff + MAX_PAYLOAD) {
          Serial.println("Max payload exceeded");
          // TODO -- tell the client
          client.stop();
          return;
        }
      }

      delay(10);
    }

    *curr = '\0';
    // TODO -- handle failure
    int val = ir_send(param_buff);
    client.print(val);
    
    client.stop();
#ifdef DEBUG
    Serial.println("");
    Serial.println("Client disconnected");
#endif
  }
}

void configure() {
  static config_state_t state = CFG_NAME;

  if (Serial.available() > 0) {
    String str = Serial.readString();
    File file = SPIFFS.open(CONFIG_FILE, state == CFG_NAME ? "w" : "a");
    if (! file) {
      Serial.print("Unable to open config file: '");
      Serial.print(CONFIG_FILE);
      Serial.println("'");
      return;
    }

    file.println(str);
    file.close();

    switch(state) {
      case CFG_NAME: state = CFG_SSID; break;
      case CFG_SSID: state = CFG_PASS; break;
      case CFG_PASS:
        Serial.println("Configuration completed");
        state = CFG_NAME;
        initWifi();
        break;
    }
    display_prompt(state);
  }
}

void checkWifi() {
  static wl_status_t stat = WL_NO_SHIELD;
  wl_status_t new_stat = WiFi.status();
  
  if (new_stat != WL_CONNECTED) {
    digitalWrite(ONBOARD_LED, LOW);
  } else {
    digitalWrite(ONBOARD_LED, HIGH);
  }

  if (stat != new_stat) {
#ifdef DEBUG
    Serial.print("WiFi status = ");
    Serial.println(new_stat);
#endif
    if (new_stat == WL_CONNECTED) {
      Serial.println("WiFi connected");  
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    }
  }

  stat = new_stat;
}

void initWifi() {
  if (! SPIFFS.exists(CONFIG_FILE)) {
    Serial.print("Could not find config file: '");
    Serial.print(CONFIG_FILE);
    Serial.println("'");
    return;
  }

  File file = SPIFFS.open(CONFIG_FILE, "r");
  if (! file) {
    Serial.print("Could not open config file: '");
    Serial.print(CONFIG_FILE);
    Serial.println("'");
  }
  
  String thing_name = file.readStringUntil('\n');
  thing_name.trim();
  String ssid = file.readStringUntil('\n');
  ssid.trim();
  String password = file.readStringUntil('\n');
  password.trim();
#ifdef DEBUG
  Serial.println("Config data:");
  Serial.println(thing_name);
  Serial.println(ssid);
  Serial.println(password);
#endif

  String host_name = thing_name;
  host_name += "_thing";
  // hostname has a maximum length of 32
  host_name = host_name.substring(0, 31);
  WiFi.hostname(host_name);
  WiFi.begin(ssid.c_str(), password.c_str());
  server.begin();
}

void initHardware() {
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, HIGH);
  pinMode(IR_LED, OUTPUT);
  digitalWrite(IR_LED, LOW);
  Serial.begin(9600);
  Serial.println("I am an IR Thing");
  
  SPIFFS.begin();
#ifdef DEBUG
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  Serial.print("SPIFFS: ");
  Serial.print(fs_info.usedBytes);
  Serial.print(" of ");
  Serial.print(fs_info.totalBytes);
  Serial.println(" bytes used.");
  Serial.print("ADC = ");
  Serial.println(analogRead(ADC));
#endif

  irsend.begin();
         
  display_prompt(CFG_NAME);
}

