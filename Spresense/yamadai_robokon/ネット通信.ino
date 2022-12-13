//SPRESENSEのWiFi（ESP8266）の接続
//ATコマンドを使用し、TCP/IP接続する
#define BAUDRATE 115200 //ボーレートとはシリアル通信のデータ転送速度


//シリアルデータを送信する際の通信速度をbps(ビット/秒)で設定する。
//2つのシリアルポート（Serial,Serial2)を異なる転送レートで初期化している
void setup() {
  Serial.begin(BAUDRATE); //BAUDRATE＝115200でポートを開く
  Serial2.begin(BAUDRATE);
  while (!Serial)
    ;
  while (!Serial2)
    ;
}

void loop() {
  if (Serial.available()) //受信したデータのバイト数を返す関数
    Serial2.write(Serial.read());//「Serial.write」バイナリーデータを送信する
                                //「Serial.read()」受信したシリアルデータを読む
  if (Serial2.available())
Serial.write(Serial2.read());
}