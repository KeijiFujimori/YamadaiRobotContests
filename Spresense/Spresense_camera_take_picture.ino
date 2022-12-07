#include <Camera.h>                                                                        // カメラを使う時に必要
#include <SPI.h>                                                                           // SPI通信するのに必要
#include <SDHCI.h>                                                                         // SDカード使うときに必要
#include "Adafruit_GFX.h"                                                                  // ディスプレイコントローラ
#include "Adafruit_ILI9341.h"                                                              // ディスプレイ使う時に必要

                                                                                           // ディスプレイのピンを定義
#define TFT_RST 8                                                                          // Reset
#define TFT_DC  9                                                                          // Digtal connect
#define TFT_CS  10                                                                         // Slave select


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS ,TFT_DC ,TFT_RST);                          // Adafruit_ILI9341クラスをdisplayインスタンスにした


SDClass theSD;                                                                             // SDクラスをインスタンスにした
bool bButtonPressed = false;                                                               // bool変数は偽であると定義
int gCounter = 0;                                                                          // 整数の変数bButtonPressedを0と定義

//******************************************************//
//            changeState関数：bButtonPressedを真にする    //
//******************************************************//
void changeState () {
  bButtonPressed = true;                                                                 // bButtonPressedを真にする
}

//**********************************************************//
//           CamCB関数：ビデオの画像を取得する                    //
//**********************************************************//
void CamCB(CamImage img){
  if (img.isAvailable()){                                                                // 利用可能な画像かをチェック、いいならtrueをかえす
    img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);                                      // 変換するピクセルフォーマット RGB565にカラー変更
    tft.drawRGBBitmap(0, 0                                                               // 開始点
    , (uint16_t *)img.getImgBuff()                                                       // メモリ取得(16bit)
    , 320, 240);                                                                         //描画開始点　横　縦

    // 記録している間LCDにshootingを赤字で表示
    if (bButtonPressed) {
      tft.setTextSize(2);                  // 文字の大きさ
      tft.setCursor(100, 200);             // x軸とy軸 ここから文字を出力
      tft.setTextColor(ILI9341_RED);       // 文字の色
      tft.println("Shooting");             // ディスプレイに書き込む文字
    }
  }
}


int intPin = 4;                            // 整数 intPinを4と定義

//*******************************//
// 起動すると1回立ち上がる関数        //
//*******************************//
void setup() {
  pinMode(LED0, OUTPUT);                   // LED0に出力
  pinMode(intPin, INPUT_PULLUP);           //プラアップ：INPUTとなると共にボタンを押してないときに5vに繋がる

  theSD.begin();                           // SDインスタンスを初期化
  Serial.begin(115200);                    // 通信速度115200
  tft.begin();                             // ディスプレイを初期化
  tft.setRotation(3);                      // ディスプレイの向き
  theCamera.begin();                       // カメラを初期化
  theCamera.startStreaming(true, CamCB);   // カメラのストリーミング開始

  theCamera.setStillPictureImageFormat(                                  // setStillPictureImageFormatで設定した写真フォーマットに従って写真を撮る
    CAM_IMGSIZE_QUADVGA_H ,CAM_IMGSIZE_QUADVGA_V                         // 静止画写真の横サイズ , 静止画写真の縦サイズ 
   ,CAM_IMAGE_PIX_FMT_JPG);                                              // 静止画ピクセルフォーマット jpeg

  attachInterrupt(digitalPinToInterrupt(intPin) ,changeState ,FALLING);  // 割り込み：(割り込みのトリガーとなるpin、割り込み発生時に実行する関数、mode:トリガー)
                                                                         // mode→LOW:ピンがLOWのとき, CHANGE:ピンが変化したとき, RISING:ピンがLOWからHIGHに変わったとき
                                                                         // FALLING:ピンの状態がHIGHからLOWに変わったとき, HIGH: ピンがHIGHのとき
}

void loop() {
  if (!bButtonPressed) return;                                           // もし、bButtonPressedが真んでなかったらloop関数終了
  Serial.println("button pressed");                                      // ボタンを押したと表示
  digitalWrite(LED0, HIGH);                                              // LED0を光らせる
  
  theCamera.startStreaming(false, CamCB);                                // falseでプレビューを停止
  CamImage img = theCamera.takePicture();                                // カメラで写真を撮る、それをCamImage imgに代入
  if (img.isAvailable()) {
      char filename[16] = {0};                                           // char型16個の配列全部0
      sprintf(filename, "PICT%03d.JPG", gCounter);                       // ファイルネームに書式を入手した文字列を代入して表示
      File myFile = theSD.open(filename,FILE_WRITE);                     // ファイルへの書き込みを目的にファイルnameを開く
      myFile.write(img.getImgBuff(), img.getImgSize());                  // ファイルにかきこみ
      myFile.close();                                                    // ファイルを閉じる
      ++gCounter;                                                        // gCounterに1を足す
  }
  bButtonPressed = false;                                                // bButtonPressedをfalseにする
  theCamera.startStreaming(true, CamCB);                                 // trueでプレビューを再開する
  digitalWrite(LED0, LOW);                                               // LED0の光を消す
}
