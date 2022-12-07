void setup(){
  Serial.begin(115200);   // 通信速度を初期化
  int tpre = 0;             // 基準値を指定
  while(1){
    int t = millis();       // 秒数を取っていく
    if((t-tpre)>1000){      // 目標値と起動時間の差が1秒以上という条件
    Serial.println(t);    // 秒数を表示
    tpre = t;             // 基準値を更新
  }
  }
}

void loop(){
}
