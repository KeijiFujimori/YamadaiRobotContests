#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8366WebServer server(80);

void top() {
  server.send(200,"text/html","<center>ようこそ")；
}

void setup(){
  WiFi.softAP("ヤッホー")；
  server.on("/",top);
  server.begin();
}