// 画像をクリップしリサイズして入力画像を生成

#include <Camera.h>                                                                 // カメラ使う時に必要
#include <SPI.h>                                                                    // SPI通信するときに必要
#include <EEPROM.h>                                                                 // SPI通信するのに必要
#include <DNNRT.h>                                                                  // AIを組み込む時に必要
#include "Adafruit_ILI9341.h"                                                       // ディスプレイ使う時に必要

#include <SDHCI.h>                                                                  // SDカードを使うときに必要
SDClass theSD;                                                                      // クラスをインスタンスにしてオブジェクト指向

/* LCD Settings */                                                                  // ディスプレイのピンを定義
#define TFT_RST 8                                                                   // リセットを8番ピン
#define TFT_DC  9                                                                   // ディジタルを9番ピン
#define TFT_CS  10                                                                  // SPI通信のスレーブは10番ピン

// 出力する画像サイズ
#define DNN_IMG_W (28)
#define DNN_IMG_H (28)

//入力する画像サイズ
#define CAM_IMG_W (320)
#define CAM_IMG_H (240)

//************************************今ココをいじった***********************************//
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

// #define LINE_THICKNESS 5                                                 // 赤線の太さに影響する

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);     // Adafruit_ILI9341クラスをtftインスタンスにした

uint8_t buf[DNN_IMG_W*DNN_IMG_H];                                           // unit8_tは変数の型：8ビットの非負整数を格納

DNNRT dnnrt;                                                                // DNNRTクラスをdnnrtインスタンスと宣言
                                                                            // 推論させたいデータを渡す
DNNVariable input(DNN_IMG_W*DNN_IMG_H);                                     // 入力データ用のバッファ　DNNVariableは独自の変数の型 データ列のメモリを確保　変数を設定
  
// static uint8_t const label[11] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};        // ラベルを定義
const char label[11] = {'0','1','2','3','4','5','6','7','8','9',' '};


//******************************************************************************//
//                        クリップサイズと推論実行を担う関数                           //
//******************************************************************************//
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
  small.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);                                      // ニューラルコンソールの画像がモノラルだったのでここでモノラル化する 便宜的にRGB565に変換 yuvでもいいらしい
  uint16_t* tmp = (uint16_t*)small.getImgBuff();

  float *dnnbuf = input.data();                                                            // input.data()で中身のポインタを取得
  float f_max = 0.0;                                                                       // 最大値を定義
                                                                                           // 入力用画像データを認識用入力データにコピー  
  for (int n = 0; n < DNN_IMG_H*DNN_IMG_W; ++n) {                                          // RGB→5bit,6bit,5bitとなっている 今回はGの6bitでビットマスクをかけてGだけの値を出すようにした0～64の値
    dnnbuf[n] = (float)((tmp[n] & 0x07E0) >> 5);
    if (dnnbuf[n] > f_max) f_max = dnnbuf[n];                                              // 前処理、画像のなかの最大値を定義
  }
  for (int n = 0; n < DNN_IMG_W*DNN_IMG_H; ++n) {
    dnnbuf[n] /= f_max;                                                                    // 最大値で割って正規化
  }
  
  // 推論の実行
  dnnrt.inputVariable(input, 0);
  dnnrt.forward();
  DNNVariable output = dnnrt.outputVariable(0);
  int index = output.maxIndex();

  return index;
}

//**************************************************************************************************************************************************************//
//                                                   文字の下の黒いところを処理する関数                                                                                //
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

//*************************************************************************ココもいじってる*************************************************************//
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

  if (!img.isAvailable()) {                                                                               // 利用可能な画像かをチェック
    Serial.println("Image is not available. Try again");                                                  // だめなら知らせる
    return;
  }
  /****************** ここもいじってる*********************/

  // カメラ画像の切り抜きと縮小
  index_0 = Number_Re(img, CAM_CLIP_X, CAM_CLIP_Y);
  index_1 = Number_Re(img, CAM_CLIP_X_1, CAM_CLIP_Y_1);
  index_2 = Number_Re(img, CAM_CLIP_X_2, CAM_CLIP_Y_2);
  index_3 = Number_Re(img, CAM_CLIP_X_3, CAM_CLIP_Y_3);

  String gStrResult;                                                                                  // この時点でgStrResultは文字列？

  if (index_0 < 11) {
    gStrResult = String(label[index_0]) + String(label[index_1]) + String(label[index_2]) + String(".") + String(label[index_3]) + String("kg");    // 変数index番目のラベルを取得、output[index]は数字がindexである確率を表す
  } else {
    gStrResult = String("?");                                                                               // indexが10より小さいわけじゃなかったら?を代入
  }
  Serial.println(gStrResult);                                                                               // 得られた文字列を表示

  img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);                                                           // ディスプレイに出力するためにRGB565に変換
  uint16_t* imgBuf = (uint16_t*)img.getImgBuff();                                                           // 毎回枠を書かないようにimgBufをつくる　getImgBuff()で画像データのバッファアドレスを取得
                                                                                                            // drawBox関数で座標や色を指定
  drawBox(imgBuf, CAM_CLIP_X,   CAM_CLIP_Y,   CAM_CLIP_W,   CAM_CLIP_H,   5, ILI9341_RED); 
  drawBox(imgBuf, CAM_CLIP_X_1, CAM_CLIP_Y_1, CAM_CLIP_W_1, CAM_CLIP_H_1, 5, ILI9341_RED); 
  drawBox(imgBuf, CAM_CLIP_X_2, CAM_CLIP_Y_2, CAM_CLIP_W_2, CAM_CLIP_H_2, 5, ILI9341_RED); 
  drawBox(imgBuf, CAM_CLIP_X_3, CAM_CLIP_Y_3, CAM_CLIP_W_3, CAM_CLIP_H_3, 5, ILI9341_RED);                   // 赤い枠を書く drawBox関数が呼び出される
  
  tft.drawRGBBitmap(0, 0, (uint16_t *)img.getImgBuff(), 320, 224);                                           // 書いた画像をそのまま転送
  // 結果を出力する
  
  putStringOnLcd(gStrResult, ILI9341_YELLOW);                                                                 // 結果をlcdに黄色い文字で出力
}


//*****************************************************************************************************************************************************************//
//                                                    プログラム起動時に呼ばれる関数                                                                                     //
//*****************************************************************************************************************************************************************//
void setup() {   
  Serial.begin(115200);                                                                               // シリアル通信を初期化し、通信速度を設定
 
  tft.begin();                                                                                        // ディスプレイを初期化
  tft.setRotation(3);                                                                                 // ディスプレイの向きを3番にする、1番と検討

  while (!theSD.begin()) { putStringOnLcd("Insert SD card", ILI9341_RED); }                           // エラーの処理、putStringOnLcd関数に引数Insert SD cardを赤文字で返して出力
  
  File nnbfile = theSD.open("model.nnb");                                                             // 学習済みデータの読み込み
  int ret = dnnrt.begin(nnbfile);                                                                     // DNNRT(組み込みAIライブラリ)を学習済みモデルで初期化（ランタイムライブラリ）
  if (ret < 0) {
    putStringOnLcd("dnnrt.begin failed" + String(ret), ILI9341_RED);                                  // エラーの処理、もしmodel.nnbファイルがなかったら初期化失敗と表示
    return;
  }
  // カメラ
  theCamera.begin();                                                                                  // 液晶ディスプレイの開始 
  theCamera.startStreaming(true, CamCB);                                                              // カメラのストリーミング開始CamCBに返す　認識処理もCamCBで行っている
}

void loop() { }
