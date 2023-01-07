#define BAUDRATE 115200 //ボーレートとはシリアル通信のデータ転送速度
#define SSID "TP-Link_24A7"   //your wifi ssid here
#define PASS "01661026"   //your wifi key here
//  wifitest.adafruit.com/testwifi/index.html
#define HOST "1"
#define SENDDATA "SEND"
#define PORT  "333"

#define REPLYBUFFSIZ 0xFFFF
String mozi1 = "",  Receivedata = "", mozi2 = "", connectionID = "";
char replybuffer[REPLYBUFFSIZ];
uint8_t getReply(char *send, uint16_t timeout = 500, boolean echo = true);
uint8_t espreadline(uint16_t timeout = 500, boolean multiline = false);
boolean sendCheckReply(char *send, char *reply, uint16_t timeout = 500);

enum {WIFI_ERROR_NONE = 0, WIFI_ERROR_AT, WIFI_ERROR_RST, WIFI_ERROR_SSIDPWD, WIFI_ERROR_SERVER, WIFI_ERROR_UNKNOWN};

void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200);
 
  while (true) {
    byte err = setupWiFi();    
    if (err) {
      Serial.print("setup error:");  Serial.println((int)err);
      debugLoop();
    }
    else if ((int)err == 0)
      break;
  }

  getIP();
}

boolean ESP_GETpage(char *host, uint16_t port, uint16_t sdata) {
  String cmd = "AT+CIPSERVER=";
  cmd += host;
  cmd += ",";
  cmd += port;
  cmd.toCharArray(replybuffer, REPLYBUFFSIZ);

  getReply(replybuffer);

  if (strcmp(replybuffer, "OK") != 0) {
    while (true) {
      espreadline(500);  // this is the 'echo' from the data
      Serial.print("<--- "); Serial.println(replybuffer);
      //split for Receivedata
      mozi1 = String(strtok(replybuffer, ","));
      connectionID = String(strtok(NULL, ","));
      mozi1 = String(strtok(NULL, ":"));
      Receivedata = String(strtok(NULL, ":"));
      mozi2 = String(strtok(NULL, ":"));
      //Serial.println(mozi1); Serial.println(mozi2);

      if (mozi1 != "") {
        Serial.print("Connection ID = "); Serial.println(connectionID);
        Serial.print("Receivedata = "); Serial.println(Receivedata);
      }

      if (strstr(replybuffer, "+IPD"))
        break;
    }
  }

  delay(500);
  String request = "";
  request += sdata;
  cmd = "AT+CIPSEND=";
  cmd += connectionID;
  cmd += ",";
  cmd += 2 + request.length();
  cmd.toCharArray(replybuffer, REPLYBUFFSIZ);
  sendCheckReply(replybuffer, ">");
/*
  Serial.print("Sending: "); Serial.println(request.length());
  Serial.println(F("*********SENDING*********"));
  Serial.println(request);
  Serial.println(F("*************************"));
  request.toCharArray(replybuffer, REPLYBUFFSIZ);*/
  Serial2.println(request);

  while (true) {
    espreadline(3000);  // this is the 'echo' from the data
    Serial.print(">"); Serial.println(replybuffer); // probably the 'busy s...'

    if (strstr(replybuffer, "wrong syntax"))
      continue;
    else if (strstr(replybuffer, "ERROR"))
      continue;
    else if (strstr(replybuffer, "busy s..."))
      continue;
    else if (strstr(replybuffer, "OK"))
      break;
    else break;
  }

  if (! strstr(replybuffer, "SEND OK") ) return false;

}

void loop()
{
  uint16_t s = 3;//送りたいデータ（体重）の入力  
  uint16_t SendData = s;
  int count = 0;

  ESP_GETpage(HOST, 8080, SendData);

  Serial.println(F("**********REPLY***********"));
  Serial.println(replybuffer);
  Serial.println(F("**************************"));

  while (true) {
    espreadline(3000);  // this is the 'echo' from the data
    Serial.print(">"); Serial.println(replybuffer); // probably the 'busy s...'

    count++;

    int i = 0;
    char buf[20];
    int ch[5];

    if (strstr(replybuffer, "busy s..."))
      continue;
    else if (strstr(replybuffer, "Recv"))
      break;
    else if (strstr(replybuffer, "CONNECT"))
      break;
    else if (count >= 20)
      break;

  }
  //debugLoop();
  delay(200);
  //while (1);
}


boolean espReset() {
  getReply("AT+RST", 1000, true);
  if (! strstr(replybuffer, "OK")) return false;
  delay(500);
  // turn off echo
  getReply("ATE0", 250, true);
  return true;
}

boolean ESPconnectAP(char *s, char *p) {

  getReply("AT+CWMODE=1", 500, true);
  if (! (strstr(replybuffer, "OK") || strstr(replybuffer, "no change")) )
    return false;

  String connectStr = "AT+CWJAP=\"";
  connectStr += SSID;
  connectStr += "\",\"";
  connectStr += PASS;
  connectStr += "\"";
  connectStr.toCharArray(replybuffer, REPLYBUFFSIZ);
  getReply(replybuffer, 200, true);

  while (true) {
    espreadline(200);  // this is the 'echo' from the data
    if ((String)replybuffer == "") {
      Serial.print(".");
    } else {
      Serial.println("");
      Serial.print("<--- "); Serial.println(replybuffer);
    }
    if (strstr(replybuffer, "OK"))
      break;
  }
  return true;
}

byte setupWiFi() {
  // reset WiFi module
  Serial.println(F("Soft resetting..."));
  if (!espReset())
    return WIFI_ERROR_RST;

  delay(500);

  if (!ESPconnectAP(SSID, PASS))
    return WIFI_ERROR_SSIDPWD;

  Serial.println(F("Multiple Mode"));

  if (!sendCheckReply("AT+CIPMUX=1", "OK"))
    return WIFI_ERROR_SERVER;

  return WIFI_ERROR_NONE;
}

boolean getIP() {
  getReply("AT+CIFSR", 100, true);
  while (true) {
    espreadline(50);  // this is the 'echo' from the data
    Serial.print("<--- "); Serial.println(replybuffer);
    if (strstr(replybuffer, "OK"))
      break;
  }

  delay(100);

  return true;
}

uint8_t espreadline(uint16_t timeout, boolean multiline) {
  uint16_t replyidx = 0;

  while (timeout--) {
    if (replyidx > REPLYBUFFSIZ - 1) break;

    while (Serial2.available()) {
      char c =  Serial2.read();
      if (c == '\r') continue;
      if (c == 0xA) {
        if (replyidx == 0)   // the first 0x0A is ignored
          continue;

        if (!multiline) {
          timeout = 0;         // the second 0x0A is the end of the line
          break;
        }
      }
      replybuffer[replyidx] = c;
      // Serial.print(c, HEX); Serial.print("#"); Serial.println(c);
      replyidx++;
    }

    if (timeout == 0) break;
    delay(1);
  }
  replybuffer[replyidx] = 0;  // null term
  return replyidx;
}

uint8_t getReply(char *send, uint16_t timeout, boolean echo) {
  // flush input
  while (Serial2.available()) {
    Serial2.read();
    delay(1);
  }

  if (echo) {
    Serial.print("---> "); Serial.println(send);
  }
  Serial2.println(send);

  // eat first reply sentence (echo)
  uint8_t readlen = espreadline(timeout);

  //Serial.print("echo? "); Serial.print(readlen); Serial.print(" vs "); Serial.println(strlen(send));

  if (strncmp(send, replybuffer, readlen) == 0) {
    // its an echo, read another line!
    readlen = espreadline();
  }

  if (echo) {
    Serial.print ("<--- "); Serial.println(replybuffer);
  }
  return readlen;
}

boolean sendCheckReply(char *send, char *reply, uint16_t timeout) {

  getReply(send, timeout, true);

  return (strcmp(replybuffer, reply) == 0);
}

void debugLoop() {
  Serial.println("========================");
  //serial loop mode for diag
  while (1) {
    if (Serial.available()) {
      Serial2.write(Serial.read());
      //      delay(1);
    }
    if (Serial2.available()) {
      Serial.write(Serial2.read());
      //      delay(1);
    }
  }
}