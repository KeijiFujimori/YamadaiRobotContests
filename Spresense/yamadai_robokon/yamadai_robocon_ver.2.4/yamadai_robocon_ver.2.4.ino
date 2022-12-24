/* 以前の数値と現在の数値を比較し、同じであれば数字のみ出力する */
/* pin4を押すとストリーミングになる */
/* 音楽の実装完了 */

#include <Camera.h>               // カメラ使う時に必要
#include <SPI.h>                  // SPI通信するときに必要
#include <EEPROM.h>               // SPI通信するのに必要
#include <DNNRT.h>                // AIを組み込む時に必要
#include "Adafruit_ILI9341.h"     // ディスプレイ使う時に必要

#include <SDHCI.h>         // SDカードを使うときに必要
#include <Audio.h>         // オーディオ機能を使うときに必要
SDClass theSD;             // クラスをインスタンスにしてオブジェクト指向

AudioClass *theAudio;      // クラスをデータを格納するポインターとしてインスタンスにした
int tpre = 0;              // millis関数を使うので基準値に0を代入しておく

bool bButtonPressed = false;                                             // bool変数は偽であると定義
int intPin = 4;
int flag = 0;              // camcbの信号をloopに送る変数

err_t err = 0;
String Result_pre = ("0");

#define id AudioClass::Player0

/* LCD Settings */                                                        // ディスプレイのピンを定義
#define TFT_RST 8                                                         // リセットを8番ピン
#define TFT_DC  9                                                         // ディジタルを9番ピン
#define TFT_CS  10                                                        // SPI通信のスレーブは10番ピン

// 出力する画像サイズ
#define DNN_IMG_W (28)
#define DNN_IMG_H (28)

//入力する画像サイズ
#define CAM_IMG_W (320)
#define CAM_IMG_H (240)

// クリッピングを開始する開地点と幅と高さ

// 一個目
#define CAM_CLIP_X (73)
#define CAM_CLIP_Y (58)
#define CAM_CLIP_W (56)
#define CAM_CLIP_H (112)

// 2個目
#define CAM_CLIP_X_1 (124)
#define CAM_CLIP_Y_1 (58)
#define CAM_CLIP_W_1 (56)
#define CAM_CLIP_H_1 (112)

// 3個目
#define CAM_CLIP_X_2 (177)
#define CAM_CLIP_Y_2 (58)
#define CAM_CLIP_W_2 (56)
#define CAM_CLIP_H_2 (112)

// 4個目
#define CAM_CLIP_X_3 (230)
#define CAM_CLIP_Y_3 (58)
#define CAM_CLIP_W_3 (56)
#define CAM_CLIP_H_3 (112)

int SETFINE = 0;
int index_0;
int index_1;
int index_2;
int index_3;

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);     // Adafruit_ILI9341クラスをtftインスタンスにした

uint8_t buf[DNN_IMG_W*DNN_IMG_H];                                           // unit8_tは変数の型：8ビットの非負整数を格納

DNNRT dnnrt;                                                                // DNNRTクラスをdnnrtインスタンスと宣言
                                                                            // 推論させたいデータを渡す
DNNVariable input(DNN_IMG_W*DNN_IMG_H);                                     // 入力データ用のバッファ　DNNVariableは独自の変数の型 データ列のメモリを確保　変数を設定
File myFile;               // FileインスタスをmyFileクラスとした  
const char label[11] = {'0','1','2','3','4','5','6','7','8','9',' '};

//******************************************************//
//            changeState関数：bButtonPressedを真にする    //
//******************************************************//
void changeState () {
  if(bButtonPressed == false){
    bButtonPressed = true;
    }else{
      bButtonPressed = false;
      }
}

//****************************************************************************//
//                        クリップサイズと推論実行を担う関数                         //
//****************************************************************************//
int Number_Re(CamImage img, int O_X, int O_Y) {
  CamImage small;
  CamErr err = img.clipAndResizeImageByHW(small
                     , O_X, O_Y
                     , O_X + CAM_CLIP_W -1
                     , O_Y + CAM_CLIP_H -1
                     , DNN_IMG_W, DNN_IMG_H);

  if (!small.isAvailable()){
    putStringOnLcd("Clip and Reize Error:" + String(err), ILI9341_RED);
    return;
  }

  // 認識用モノクロ画像を設定
  small.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);                              // ニューラルコンソールの画像がモノラルだったのでここでモノラル化する 便宜的にRGB565に変換 yuvでもいいらしい
  uint16_t* tmp = (uint16_t*)small.getImgBuff();

  float *dnnbuf = input.data();                                                  // input.data()で中身のポインタを取得
  float f_max = 0.0;                                                             // 最大値を定義
                                                                                 // 入力用画像データを認識用入力データにコピー  
  for (int n = 0; n < DNN_IMG_H*DNN_IMG_W; ++n) {                                // RGB→5bit,6bit,5bitとなっている 今回はGの6bitでビットマスクをかけてGだけの値を出すようにした0～64の値
    dnnbuf[n] = (float)((tmp[n] & 0x07E0) >> 5);
    if (dnnbuf[n] > f_max) f_max = dnnbuf[n];                                    // 前処理、画像のなかの最大値を定義
  }
  for (int n = 0; n < DNN_IMG_W*DNN_IMG_H; ++n) {
    dnnbuf[n] /= f_max;                                                           // 最大値で割って正規化
  }
  
  // 推論の実行
  dnnrt.inputVariable(input, 0);
  dnnrt.forward();
  DNNVariable output = dnnrt.outputVariable(0);
  int index = output.maxIndex();

  return index;     // 返す値
}

//**************************************************************************************************************************************************************//
//                                                   文字の下の黒いところを処理する関数                                                                               //
//**************************************************************************************************************************************************************//

void putStringOnLcd(String str, int color) {
  int len = str.length();                                                    // 整数lenを引数strの文字数として代入
  tft.fillRect(0,224, 320, 240, ILI9341_BLACK);                              // fillReact(x:左上からのx座標,y:左上からのy座標,w:幅,h:高さ,色)
  tft.setTextSize(2);                                                        // テキストの文字の大きさ
  int sx = 160 - len/2*12;                                                   // 文字の始まる列を計算によってもとめている
  if (sx < 0) sx = 0;                                                        // もしマイナスを取ることがあったらそれは0と見ていい
  tft.setCursor(sx, 225);                                                    // setCursor(col.row) col:列 row:行
  tft.setTextColor(color);                                                   // テキストの色を引数であるcolorにする
  tft.println(str);                                                          // ディスプレイに文字型を出力
}



//****************************************************************************************************************//
//                                  赤枠の関数                                                                      //
//****************************************************************************************************************//
void drawBox(uint16_t* imgBuf, int offset_x, int offset_y, int width, int height, int thickness, int color) {
  /* Draw target line */                                                     // 目的の線を引く
  for (int x = offset_x; x < offset_x+width; ++x) {                          // for文で上で定義したクリップ枠線を引いていく(x座標)
    for (int n = 0; n < thickness; ++n) {
      *(imgBuf + CAM_IMG_W*(offset_y+n) + x)           = color;
      *(imgBuf + CAM_IMG_W*(offset_y+height-1-n) + x) = color;
    }
  }
  for (int y = offset_y; y < offset_y+height; ++y) {                         // for文で上で定義したクリップ枠線を引いていく(y座標) 
    for (int n = 0; n < thickness; ++n) {                                    // ラインの太さLINE_THICKNESSは上記で定義している for文で繰り返す
      *(imgBuf + CAM_IMG_W*y + offset_x+n)           = color;                // CamCBからのimgBufを受け取る
      *(imgBuf + CAM_IMG_W*y + offset_x + width-1-n) = color;
    }
  }  
}


//*******************************************************************************************************************************************//
//                                                      ビデオ画像と学習済みデータを習得するためのコールバック関数                                      //
//******************************************************************************************************************************************//
void CamCB(CamImage img) {
  
  /* 割り込み：(割り込みのトリガーとなるpin、割り込み発生時に実行する関数、mode:トリガー) */
  attachInterrupt(digitalPinToInterrupt(intPin) ,changeState ,FALLING);

  /* ストリーミング可能かどうかの処理 */
  if (!img.isAvailable()) {
    Serial.println("Image is not available. Try again");
    return;
  }

  
  /* カメラ画像の切り抜きと縮小 */
  index_0 = Number_Re(img, CAM_CLIP_X, CAM_CLIP_Y);
  index_1 = Number_Re(img, CAM_CLIP_X_1, CAM_CLIP_Y_1);
  index_2 = Number_Re(img, CAM_CLIP_X_2, CAM_CLIP_Y_2);
  index_3 = Number_Re(img, CAM_CLIP_X_3, CAM_CLIP_Y_3);

  /* 変数の文字列を定義 */
  String gStrResult = String(label[index_0]) + String(label[index_1]) + String(label[index_2]) + String(".") + String(label[index_3]) + String(" ") + String("kg");
  
  img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);    // ディスプレイに出力するためにRGB565に変換
  uint16_t* imgBuf = (uint16_t*)img.getImgBuff();    // 毎回枠を書かないようにimgBufをつくる　getImgBuff()で画像データのバッファアドレスを取得
 
  /* drawBox関数で座標や色を指定、赤い枠を書く */
  drawBox(imgBuf, CAM_CLIP_X,   CAM_CLIP_Y,   CAM_CLIP_W,   CAM_CLIP_H,   5, ILI9341_RED); 
  drawBox(imgBuf, CAM_CLIP_X_1, CAM_CLIP_Y_1, CAM_CLIP_W_1, CAM_CLIP_H_1, 5, ILI9341_RED); 
  drawBox(imgBuf, CAM_CLIP_X_2, CAM_CLIP_Y_2, CAM_CLIP_W_2, CAM_CLIP_H_2, 5, ILI9341_RED); 
  drawBox(imgBuf, CAM_CLIP_X_3, CAM_CLIP_Y_3, CAM_CLIP_W_3, CAM_CLIP_H_3, 5, ILI9341_RED);

  /* 4ピンを押すとストリーミングが始まる */
  if(bButtonPressed == false){
    tft.fillRect(0,0, 320, 240, ILI9341_BLACK);
    tft.setTextSize(10);
    tft.setCursor(120, 80);                                                  // setCursor(col.row) col:列 row:行
    tft.setTextColor(ILI9341_YELLOW);
    tft.println("?");       // ディスプレイに文字型を出力

    int t = millis();      // 秒数を取っていく

    /* 秒数の差を出し続ける、条件を越えたら次の条件式に移る */
    if((t-tpre)>500){

      /* 前のデータと後のデータが同じになれば変数を変えてloopに渡し、カメラ終了 */
      if(Result_pre == gStrResult){ 
        flag = 1;                                // loop関数に渡す
        theCamera.startStreaming(false, CamCB);  // ストリーミングを終了
        
      }else{
        Serial.println("one more time");         // 達成しなかったらもう一回と表示
        }
        tpre = t;                                // 基準値を更新
        Result_pre = gStrResult;                 // Result_preにgStrResultの文字列を格納
        
  }
 }else{
  tft.drawRGBBitmap(0, 0, (uint16_t *)img.getImgBuff(), 320, 224);           // 書いた画像をそのまま転送
  putStringOnLcd(gStrResult, ILI9341_YELLOW);                                // 結果を出力する
 }
}


//*****************************************************************************************************************************************************************//
//                                                    プログラム起動時に呼ばれる関数                                                                                     //
//*****************************************************************************************************************************************************************//
void setup() { 
  Serial.begin(115200);                            // シリアル通信を初期化し、通信速度を設定
  pinMode(intPin,INPUT_PULLUP);                    // pullupによって5v流れてる ボタン押されたら0v

  /* ディスプレイ設定 */
  tft.begin();                                     // ディスプレイを初期化
  tft.setRotation(3);                              // ディスプレイの向きを3番にする、1番と検討
  
  while (!theSD.begin()) { putStringOnLcd("Insert SD card", ILI9341_RED); } // エラーの処理、putStringOnLcd関数に引数Insert SD cardを赤文字で返して出力
  File nnbfile = theSD.open("model.nnb");         // 学習済みデータの読み込み

  /* AIの処理 */
  int ret = dnnrt.begin(nnbfile);                // DNNRT(組み込みAIライブラリ)を学習済みモデルで初期化（ランタイムライブラリ）
  if (ret < 0) {
    putStringOnLcd("dnnrt.begin failed" + String(ret), ILI9341_RED);        // エラーの処理、もしmodel.nnbファイルがなかったら初期化失敗と表示
    return;
  }
  
  /* オーディオ処理 */
  theAudio = AudioClass::getInstance(); 
  theAudio->begin();                                                     // オーディオを初期化してaudio_attention_cbを呼び出す()の中は空欄でもいいらしい
  theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);                    // クロックモードの設定、AS_CLKMODE_NORMAL:49Hz以下とAS_CLKMODE_HIRES:90Hz異常の2つの設定がある
  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_LINEOUT);           // 出力先：スピーカ出力に設定、出力サイズ:スピーカ出力をラインアウトレベル設定(ジャックを使うならこれ)
  theAudio->setVolume(-60);
  err = theAudio->initPlayer(id, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO); 
  /* 1曲目 */
  myFile = theSD.open("Sound0.mp3");   // ファイルを開く
  puts("file open");
  err = theAudio->writeFrames(id, myFile);  // とりあえずなんかFIFOに入れておく
  theAudio->startPlayer(id);
  
  /* カメラ処理 */
  theCamera.begin();                       // 液晶ディスプレイの開始 
  theCamera.startStreaming(true, CamCB);   // カメラのストリーミング開始CamCBに返す認識処理もCamCBで行っている
}

void loop() {
  /* 1曲目のデータを送り続ける */
  err = theAudio->writeFrames(id, myFile);  // FIFOに書き込み続ける

  /* CamCBのフラグを取得したら曲をストップして2曲目へ */
  if(flag == 1){
    theAudio->stopPlayer(id, AS_STOPPLAYER_NORMAL);     // オーディオをストップ 
    myFile.close();                                     // ファイルを閉じて終了

    /* 2曲目 */
    myFile = theSD.open("Sound1.mp3"); 
    err = theAudio->writeFrames(id, myFile);   // SDカードからデータを読み込んでファイルに入れる
    theAudio->startPlayer(id);                 // SpresenseのFIFOの中のデータを引っ張って再生
    
    /* ディスプレイに数値を出力 */
    tft.fillRect(0,0, 320, 240, ILI9341_BLACK);
    tft.setTextSize(6);                      // テキストのサイズ
    tft.setCursor(20, 90);                   // setCursor(col.row) col:列 row:行
    tft.setTextColor(ILI9341_YELLOW);        // テキストカラー
    tft.println(Result_pre);                 // ディスプレイに数値を出力
    Serial.println(Result_pre);              // シリアルモニタにも出力

    /* 2曲目のデータを送り続け、曲が終わったら関数を終える */
    while(1){
      err = theAudio->writeFrames(id, myFile);      // SDカードからデータを読み込んでファイルに入れる
      theAudio->stopPlayer(id);                     // オーディオをストップ 
      myFile.close();                               // ファイルを閉じて終了
      theAudio->setReadyMode();
      theAudio->end();                              // オーディオを終わらせて音漏れを防ぐ
      delay(5000);
      Serial.println("finshed");
      tft.fillRect(0,0, 320, 240, ILI9341_BLACK);
      exit(1);
    }
  }
}
