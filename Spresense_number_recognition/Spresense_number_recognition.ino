// 画像をクリップしリサイズして入力画像を生成

#include <Camera.h>                                                                 // カメラを使う時に必要
#include <SPI.h>                                                                    // SPI通信するときに必要
#include <EEPROM.h>                                                                 // 
#include <DNNRT.h>                                                                  // AIを組み込む時に必要
#include "Adafruit_ILI9341.h"                                                       // ディスプレイ使う時に必要

#include <SDHCI.h>                                                                  // SDカードを使うときに必要
SDClass theSD;

/* LCD Settings */                                                                  // ディスプレイのピンを定義
#define TFT_RST 8                                                                   // リセットを8番ピン
#define TFT_DC  9                                                                   // ディジタルを9番ピン
#define TFT_CS  10                                                                  // SPI通信のスレーブは10番ピン

// 出力する画像サイズ
#define DNN_IMG_W 28
#define DNN_IMG_H 28

//入力する画像サイズ
#define CAM_IMG_W 320
#define CAM_IMG_H 240

// クリッピングを開始する開地点
#define CAM_CLIP_X 104
#define CAM_CLIP_Y 0

// 横と縦の領域をクリップする
#define CAM_CLIP_W 112
#define CAM_CLIP_H 224

#define LINE_THICKNESS 5

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);     // Adafruit_ILI9341クラスをtftインスタンスにした

uint8_t buf[DNN_IMG_W*DNN_IMG_H];

DNNRT dnnrt;                                                                // DNNRTクラスをdnnrtインスタンスと宣言
                                                                            // 推論させたいデータを渡す
DNNVariable input(DNN_IMG_W*DNN_IMG_H);                                     // 入力データ用のバッファ　DNNVariableは独自の変数の型 データ列のメモリを確保　変数を設定
  
static uint8_t const label[11] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};        // ラベルを定義

void putStringOnLcd(String str, int color) {
  int len = str.length();
  tft.fillRect(0,224, 320, 240, ILI9341_BLACK);
  tft.setTextSize(2);
  int sx = 160 - len/2*12;
  if (sx < 0) sx = 0;
  tft.setCursor(sx, 225);
  tft.setTextColor(color);
  tft.println(str);
}

void drawBox(uint16_t* imgBuf) {
  /* Draw target line */
  for (int x = CAM_CLIP_X; x < CAM_CLIP_X+CAM_CLIP_W; ++x) {
    for (int n = 0; n < LINE_THICKNESS; ++n) {
      *(imgBuf + CAM_IMG_W*(CAM_CLIP_Y+n) + x)              = ILI9341_RED;
      *(imgBuf + CAM_IMG_W*(CAM_CLIP_Y+CAM_CLIP_H-1-n) + x) = ILI9341_RED;
    }
  }
  for (int y = CAM_CLIP_Y; y < CAM_CLIP_Y+CAM_CLIP_H; ++y) {
    for (int n = 0; n < LINE_THICKNESS; ++n) {
      *(imgBuf + CAM_IMG_W*y + CAM_CLIP_X+n)                = ILI9341_RED;
      *(imgBuf + CAM_IMG_W*y + CAM_CLIP_X + CAM_CLIP_W-1-n) = ILI9341_RED;
    }
  }  
}


// ビデオ画像を習得するためのコールバック関数
void CamCB(CamImage img) {

  if (!img.isAvailable()) {                                                                // 利用可能な画像かをチェック
    Serial.println("Image is not available. Try again");                                   // だめなら知らせる
    return;
  }

  CamImage small;                                                                          // CamImageインスタンスのsmallに縮小した画像が格納される
                                                                                           // 画像をクリップサイズして縮小化する
  CamErr err = img.clipAndResizeImageByHW(small                                            // clipAndResizeImageByHWはspresenseのアクセサレータ 
                     , CAM_CLIP_X, CAM_CLIP_Y                                              // このアクセサレータには制約がある
                     , CAM_CLIP_X + CAM_CLIP_W -1                                          // 開始地点が偶数でなくてはいけない
                     , CAM_CLIP_Y + CAM_CLIP_H -1                                          // WHが偶数でなくてはいけない
                     , DNN_IMG_W, DNN_IMG_H);                                              // ハードウェアの制約
  if (!small.isAvailable()){
    putStringOnLcd("Clip and Reize Error:" + String(err), ILI9341_RED);
    return;
  }

  small.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);                                        // ニューラルコンソールの画像がモノラルだったのでここでモノラル化する 便宜的にRGB565に変換 yuvでもいいらしい
  uint16_t* tmp = (uint16_t*)small.getImgBuff();

  float *dnnbuf = input.data();                                                            // input.data()で中身のポインタを取得
  float f_max = 0.0;                                                                       // 最大値を定義
                                                                                           // 入力用画像データを認識用入力データにコピー  
  for (int n = 0; n < DNN_IMG_H*DNN_IMG_W; ++n) {                                          // RGB→5bit,6bit,5bitとなっている 今回はGの6bitでビットマスクをかけてGだけの値を出すようにした0～64の値
    dnnbuf[n] = (float)((tmp[n] & 0x07E0) >> 5);
    if (dnnbuf[n] > f_max) f_max = dnnbuf[n];                                              // 前処理、画像のなかの最大値を定義
  }
  
  /* normalization */
  for (int n = 0; n < DNN_IMG_W*DNN_IMG_H; ++n) {
    dnnbuf[n] /= f_max;                                                                    // 最大値で割って正規化
  }
  
  String gStrResult = "?";

  // DNNRTにデータを設定し推論を実行。推論結果(最大の確立をもつインデックス値)を取得
  
  dnnrt.inputVariable(input, 0);                                                // inputは1つなので0番目の領域を指定
  dnnrt.forward();                                                              // 推論を実行
  DNNVariable output = dnnrt.outputVariable(0);                                 // outputを得る
  int index = output.maxIndex();                                                // 確からしいラベルを取得
  
  if (index < 10) {
    gStrResult = String(label[index]) + String(":") + String(output[index]);    // 得られた結果をラベルで文字列にした+確からしさも
  } else {
    gStrResult = String("?:") + String(output[index]);
  }
  Serial.println(gStrResult);

  img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);                               // ディスプレイに出力するためにRGB565に変換
  uint16_t* imgBuf = (uint16_t*)img.getImgBuff();                               // 毎回枠を書かないようにimgBufをつくる

  drawBox(imgBuf);                                                              // 赤い枠を書く drawBox関数が呼び出される
  tft.drawRGBBitmap(0, 0, (uint16_t *)img.getImgBuff(), 320, 224);              // 書いた画像をそのまま転送
  // 結果を出力する
  
  putStringOnLcd(gStrResult, ILI9341_YELLOW);                                   // 結果をlcdに黄色い文字で出力
}


void setup() {   
  Serial.begin(115200);
 
  tft.begin();
  tft.setRotation(3);

  while (!theSD.begin()) { putStringOnLcd("Insert SD card", ILI9341_RED); }
  
  File nnbfile = theSD.open("model.nnb");                                                             // 学習済みデータの読み込み
  int ret = dnnrt.begin(nnbfile);                                                                     // DNNRT(組み込みAIライブラリ)を学習済みモデルで初期化（ランタイムライブラリ）
  if (ret < 0) {
    putStringOnLcd("dnnrt.begin failed" + String(ret), ILI9341_RED);
    return;
  }
  // カメラ
  theCamera.begin();                                                                                  // 液晶ディスプレイの開始 
  theCamera.startStreaming(true, CamCB);                                                              // カメラのストリーミング開始CamCBに返す　認識処理もCamCBで行っている
}

void loop() { }
