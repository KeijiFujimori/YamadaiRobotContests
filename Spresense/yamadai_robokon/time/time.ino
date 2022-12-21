void setup(){
  Serial.begin(115200);   // 通信速度を初期化
}

void loop(){
  int t = millis();       // 秒数を取っていく
  Serial.println(t);    // 秒数を表示
}
