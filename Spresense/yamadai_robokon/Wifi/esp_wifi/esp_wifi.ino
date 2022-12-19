




void top() {
  server.send(200,"text/html","<center>ようこそ</center>");
}

void setup(){
  WiFi.softAP("ヤッホー");
  server.on("/",top);
  server.begin();
}

void loop(){
  server.handleClient();
}
