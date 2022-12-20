#include "SPI.h"
// #include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define TFT_DC 9
#define TFT_CS -1
#define TFT_RST 8

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);

bool bButtonPressed = false;                                             // bool変数は偽であると定義
int intPin = 4;

void changeState () {
  bButtonPressed = true;
}

void setup() {
  attachInterrupt(digitalPinToInterrupt(intPin) ,changeState ,FALLING);  // 割り込み：(割り込みのトリガーとなるpin、割り込み発生時に実行する関数、mode:トリガー)
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(3);                                                                                 // ディスプレイの向きを3番にする、1番と検討
  
  while(1){
      tft.fillRect(0,0, 320, 240, ILI9341_BLACK);                              // fillReact(x:左上からのx座標,y:左上からのy座標,w:幅,h:高さ,色)
      tft.setTextSize(10);                                                      // テキストの文字の大きさ
      String str = "?";
      int len = str.length();                                                  // 整数lenを引数strの文字数として代入
      int sx = 120 - len/2*12;                                                 // 文字の始まる列を計算によってもとめている
  
      if (sx < 0) sx = 0;                                                      // もしマイナスを取ることがあったらそれは0と見ていい
      tft.setCursor(sx, 80);                                                  // setCursor(col.row) col:列 row:行
      tft.setTextColor(ILI9341_YELLOW);
      tft.println(str);                                                        // ディスプレイに文字型を出力
      if(bButtonPressed == true){
        str = "123.5kg";
        tft.fillRect(0,0, 320, 240, ILI9341_BLACK);
        if (sx < 0) sx = 0;
        tft.setTextSize(7);
        tft.setCursor(50, 80);
        tft.setTextColor(ILI9341_YELLOW);                                        // テキストの色を引数であるcolorにする
        tft.println(str);
        delay(1000);
      
        bButtonPressed = false; 
    }
  }
}


void loop(void) {
}
