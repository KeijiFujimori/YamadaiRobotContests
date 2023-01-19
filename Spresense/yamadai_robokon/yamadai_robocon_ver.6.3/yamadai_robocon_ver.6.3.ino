/* 以前の数値と現在の数値を比較し、同じであれば数字のみ出力する */
/* pin4を押すとストリーミングになる */
/* 音楽の実装完了 */
/* リセットを追加した */
/* WiFiステーションモードのサーバになる */
/* coldsleepを実装した */
/* 前回のデータと比較できるように新たなファイルを生成するプログラムを実装した */
/* それをディスプレイで見れるようにした*/

#include <Camera.h>               // カメラ使う時に必要
#include <SPI.h>                  // SPI通信するときに必要
#include <EEPROM.h>               // SPI通信するのに必要
#include <DNNRT.h>                // AIを組み込む時に必要
#include <LowPower.h>             // sleepモードにするのに必要
#include "Adafruit_ILI9341.h"     // ディスプレイ使う時に必要

#include <SDHCI.h>                // SDカードを使うときに必要
#include <Audio.h>                // オーディオ機能を使うときに必要
SDClass theSD;                    // クラスをインスタンスにしてオブジェクト指向

AudioClass *theAudio;             // クラスをデータを格納するポインターとしてインスタンスにした
int tpre = 0;                     // millis関数を使うので基準値に0を代入しておく

bool bButtonPressed = false;     // bool変数は偽であると定義
int intPin = 4;                  // ストリーミングにするpin
int reset_Pin = 3;               // リセットpinを定義
int flag = 0;                    // camcbの信号をloopに送る変数

err_t err = 0;
String Result_pre = ("0");
String pre_value;

/* WiFi */
#define SSID ""   //your wifi ssid here
#define PASS ""    //your wifi key here
#define HOST "1"
#define SENDDATA "SEND"
#define PORT  "333"

#define REPLYBUFFSIZ 0xFFFF
String mozi1 = "",  Receivedata = "", mozi2 = "", connectionID = "";
char replybuffer[REPLYBUFFSIZ];
uint8_t getReply(char *send, uint16_t timeout = 500, boolean echo = true);
uint8_t espreadline(uint16_t timeout = 500, boolean multiline = false);
boolean sendCheckReply(char *send, char *reply, uint16_t timeout = 500);

const uint8_t button5 = PIN_D05;    // sleepを起こすpin
const uint8_t button4 = PIN_D04;

enum {WIFI_ERROR_NONE = 0, WIFI_ERROR_AT, WIFI_ERROR_RST, WIFI_ERROR_SSIDPWD, WIFI_ERROR_SERVER, WIFI_ERROR_UNKNOWN};

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

//***************************************************************************//
//                  changeState関数：bButtonPressedを真にする                   //
//***************************************************************************//
void changeState () {
  if(bButtonPressed == false){
    bButtonPressed = true;
    puts("true");
    }else{
      bButtonPressed = false;
      puts("false");
      tft.fillRect(0,0, 320, 240, ILI9341_BLACK);
      theCamera.startStreaming(false, CamCB);  // ストリーミングを終了
      software_reset();
      }
}


//****************************************************************************//
//                        クリップサイズと推論実行を担う関数                        //
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
//                                                   文字の下の黒いところを処理する関数                                                                              //
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


//*****************************************************************************************************************************//
//                                            ビデオ画像と学習済みデータを習得するためのコールバック関数                                  //
//******************************************************************************************************************************//
void CamCB(CamImage img) {
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
  String gStrResult = String(label[index_1]) + String(label[index_2]) + String(".") + String(label[index_3]);
  
  img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);    // ディスプレイに出力するためにRGB565に変換
  uint16_t* imgBuf = (uint16_t*)img.getImgBuff();    // 毎回枠を書かないようにimgBufをつくる　getImgBuff()で画像データのバッファアドレスを取得
 
  /* drawBox関数で座標や色を指定、赤い枠を書く */
  drawBox(imgBuf, CAM_CLIP_X,   CAM_CLIP_Y,   CAM_CLIP_W,   CAM_CLIP_H,   5, ILI9341_RED); 
  drawBox(imgBuf, CAM_CLIP_X_1, CAM_CLIP_Y_1, CAM_CLIP_W_1, CAM_CLIP_H_1, 5, ILI9341_RED); 
  drawBox(imgBuf, CAM_CLIP_X_2, CAM_CLIP_Y_2, CAM_CLIP_W_2, CAM_CLIP_H_2, 5, ILI9341_RED); 
  drawBox(imgBuf, CAM_CLIP_X_3, CAM_CLIP_Y_3, CAM_CLIP_W_3, CAM_CLIP_H_3, 5, ILI9341_RED);

  /* 4ピンを押すとストリーミングが始まる */
  if(bButtonPressed == false){
    puts("?");

    int t = millis();      // 秒数を取っていく

    /* 秒数の差を出し続ける、条件を越えたら次の条件式に移る */
    if((t-tpre)>500 && t>=5000 && gStrResult != "  0.0" ){

      /* 前のデータと後のデータが同じになれば変数を変えてloopに渡し、カメラ終了 */
      if(Result_pre == gStrResult){ 
        flag = 1;                                // loop関数に渡す
        Serial.println(t);
        theCamera.startStreaming(false, CamCB);  // ストリーミングを終了
        
      }else{
        Serial.println("one more time");         // 達成しなかったらもう一回と表示
        }
        tpre = t;                                // 基準値を更新
        Result_pre = gStrResult;                 // Result_preにgStrResultの文字列を格納
        
  }
 }else{
  /* bButtonPressedがtrueならストリーミングとしての処理 */
  flag = 2;     // 曲を停止するためにflagに2を返す
  tft.drawRGBBitmap(0, 0, (uint16_t *)img.getImgBuff(), 320, 224);           // 書いた画像をそのまま転送
  putStringOnLcd(gStrResult, ILI9341_YELLOW);                                // 結果を出力する
 }
}

//******************************************************************//
//                        リセット関数                                //
//******************************************************************//
void software_reset(){
  digitalWrite(reset_Pin, LOW);
}

//******************************************************************//
//                     引数を与えてサーバーになる関数                    //
//******************************************************************//
boolean ESP_GETpage(char *host, uint16_t port, String sdata) {
  String cmd = "AT+CIPSERVER=";   // サーバーになる
  cmd += host;
  cmd += ",";
  cmd += port;
  cmd.toCharArray(replybuffer, REPLYBUFFSIZ);

  getReply(replybuffer);

  if (strcmp(replybuffer, "OK") != 0) {
    while (true) {
      espreadline(500);  // this is the 'echo' from the data
      Serial.print("<--- "); Serial.println(replybuffer);
      //split for Receivedata
      mozi1 = String(strtok(replybuffer, ","));
      connectionID = String(strtok(NULL, ","));
      mozi1 = String(strtok(NULL, ":"));
      Receivedata = String(strtok(NULL, ":"));
      mozi2 = String(strtok(NULL, ":"));
      //Serial.println(mozi1); Serial.println(mozi2);

      if (mozi1 != "") {
        Serial.print("Connection ID = "); Serial.println(connectionID);
        Serial.print("Receivedata = "); Serial.println(Receivedata);
      }

      if (strstr(replybuffer, "+IPD"))
        break;
    }
  }

  delay(500);
  String request = "";
  request += sdata;
  cmd = "AT+CIPSEND=";   // データの送信
  cmd += connectionID;
  cmd += ",";
  cmd += 2 + request.length();                      // 必要な情報量に達するために2足した
  cmd.toCharArray(replybuffer, REPLYBUFFSIZ);
  sendCheckReply(replybuffer, ">");

  Serial.print("Sending: "); Serial.println(request.length());
  Serial.println(F("*********SENDING*********"));
  Serial.println(request);
  Serial.println(F("*************************"));
  request.toCharArray(replybuffer, REPLYBUFFSIZ);
  Serial2.println(request);

  while (true) {
    espreadline(3000);  // this is the 'echo' from the data
    Serial.print(">"); Serial.println(replybuffer); // probably the 'busy s...'

    if (strstr(replybuffer, "wrong syntax"))
      continue;
    else if (strstr(replybuffer, "ERROR"))
      continue;
    else if (strstr(replybuffer, "busy s..."))
      continue;
    else if (strstr(replybuffer, "OK"))
      break;
    else break;
  }

  if (! strstr(replybuffer, "SEND OK") ) return false;

}

//*************************************************************//
//                   WiFiをリセットさせる関数                      //
//*************************************************************//
boolean espReset() {
  getReply("AT+RST", 1000, true);  // リセット
  if (! strstr(replybuffer, "OK")) return false;
  delay(500);
  // turn off echo
  getReply("ATE0", 250, true);
  return true;
}

//*******************************************************************//
//                      WiFiにつなげる関数                             //
//*******************************************************************//
boolean ESPconnectAP(char *s, char *p) {

  getReply("AT+CWMODE=1", 500, true);  // ステーションモードに設定
  if (! (strstr(replybuffer, "OK") || strstr(replybuffer, "no change")) )
    return false;

  String connectStr = "AT+CWJAP=\"";   // ネットに接続
  connectStr += SSID;
  connectStr += "\",\"";
  connectStr += PASS;
  connectStr += "\"";
  connectStr.toCharArray(replybuffer, REPLYBUFFSIZ);
  getReply(replybuffer, 200, true);

  while (true) {
    espreadline(200);  // this is the 'echo' from the data
    if ((String)replybuffer == "") {
      Serial.print(".");
    } else {
      Serial.println("");
      Serial.print("<--- "); Serial.println(replybuffer);
    }
    if (strstr(replybuffer, "OK"))
      break;
  }
  return true;
}

//**********************************************************************//
//                    WiFiをリセットする関数                               //
//**********************************************************************//
byte setupWiFi() {
  // reset WiFi module
  Serial.println(F("Soft resetting..."));
  if (!espReset())
    return WIFI_ERROR_RST;

  delay(500);

  if (!ESPconnectAP(SSID, PASS))
    return WIFI_ERROR_SSIDPWD;

  Serial.println(F("Multiple Mode"));

  if (!sendCheckReply("AT+CIPMUX=1", "OK"))   // マルチプルコネクションモード
    return WIFI_ERROR_SERVER;

  return WIFI_ERROR_NONE;
}

//************************************************************************//
//                      IPアドレスを取得する関数                              //
//************************************************************************//
boolean getIP() {
  getReply("AT+CIFSR", 100, true);            // IPアドレスの取得
  while (true) {
    espreadline(50);  // this is the 'echo' from the data
    Serial.print("<--- "); Serial.println(replybuffer);
    if (strstr(replybuffer, "OK"))
      break;
  }

  delay(100);

  return true;
}

//*************************************************************************//
//                      ポータから読んだ行数を返す                              //
//*************************************************************************//
uint8_t espreadline(uint16_t timeout, boolean multiline) {
  uint16_t replyidx = 0;

  while (timeout--) {
    if (replyidx > REPLYBUFFSIZ - 1) break;

    while (Serial2.available()) {
      char c =  Serial2.read();
      if (c == '\r') continue;
      if (c == 0xA) {
        if (replyidx == 0)   // the first 0x0A is ignored
          continue;

        if (!multiline) {
          timeout = 0;       // the second 0x0A is the end of the line
          break;
        }
      }
      replybuffer[replyidx] = c;
      replyidx++;
    }

    if (timeout == 0) break;
    delay(1);
  }
  replybuffer[replyidx] = 0;  // null term
  return replyidx;
}

//************************************************************************//
//                           読んだ行の文字数を返す関数                        //
//************************************************************************//
uint8_t getReply(char *send, uint16_t timeout, boolean echo) {
  // フラッシュ入力
  while (Serial2.available()) {
    Serial2.read();
    delay(1);
  }

  if (echo) {
    Serial.print("---> "); 
    Serial.println(send);
  }
  Serial2.println(send);

  // 最初の返信文を食べる (エコー)
  uint8_t readlen = espreadline(timeout);

  if (strncmp(send, replybuffer, readlen) == 0) {
    // そのエコー、別の行を読んでください!
    readlen = espreadline();
  }

  if (echo) {
    Serial.print ("<--- "); Serial.println(replybuffer);
  }
  return readlen;
}

boolean sendCheckReply(char *send, char *reply, uint16_t timeout) {

  getReply(send, timeout, true);

  return (strcmp(replybuffer, reply) == 0);
}

//**********************************************************************************//
//                           エラー内容を表示する関数                                   //
//**********************************************************************************//
void debugLoop() {
  Serial.println("========================");
  //diag のシリアル ループ モード
  while (1) {
    if (Serial.available()) {
      Serial2.write(Serial.read());
    }
    if (Serial2.available()) {
      Serial.write(Serial2.read());
    }
  }
}
//***********************************************************************************//
//                             何番pinで押したかわかる関数                               //
//***********************************************************************************//
void printBootCause(bootcause_e bc)
{
  Serial.println("--------------------------------------------------");
  Serial.print("Boot Cause: ");
  if ((COLD_GPIO_IRQ36 <= bc) && (bc <= COLD_GPIO_IRQ47)) {
    // Wakeup by GPIO
    int pin = LowPower.getWakeupPin(bc);
    Serial.print(" <- pin ");
    Serial.print(pin);
  }
  Serial.println();
  Serial.println("--------------------------------------------------");
}

void pushed5()
{
  Serial.println("Pushed D05!");
}

//*****************************************************************************************************************************************************************//
//                                                             メイン関数                                                                                           //
//*****************************************************************************************************************************************************************//
void setup() { 
  digitalWrite(reset_Pin, HIGH);       // 予めpinに電圧を入れる
  Serial.begin(115200);                // シリアル通信を初期化し、通信速度を設定
  Serial2.begin(115200);

  
  /* オーディオ処理 */
  theAudio = AudioClass::getInstance(); 
  theAudio->begin();                                                     // オーディオを初期化してaudio_attention_cbを呼び出す()の中は空欄でもいいらしい
  theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);                    // クロックモードの設定、AS_CLKMODE_NORMAL:49Hz以下とAS_CLKMODE_HIRES:90Hz異常の2つの設定がある
  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_LINEOUT);           // 出力先：スピーカ出力に設定、出力サイズ:スピーカ出力をラインアウトレベル設定(ジャックを使うならこれ)
  theAudio->setVolume(110);
  err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO); 
  /* 1曲目 */
  myFile = theSD.open("Sound0.mp3");         // ファイルを開く
  puts("file open");                         // ファイルのオープンを知らせる
  err = theAudio->writeFrames(AudioClass::Player0, myFile);   // とりあえずなんかFIFOに入れておく
  theAudio->startPlayer(AudioClass::Player0);                 // オーディオスタート
  
  pinMode(reset_Pin, OUTPUT);          // pinを定義
  pinMode(intPin,INPUT_PULLUP);        // pullupによって5v流れてる ボタン押されたら0v
  
  
  /* 割り込み：(割り込みのトリガーとなるpin、割り込み発生時に実行する関数、mode:トリガー) */
  attachInterrupt(digitalPinToInterrupt(intPin) ,changeState ,FALLING);
  
  /* LowPowerライブラリを初期化*/
  LowPower.begin();
  bootcause_e bc = LowPower.bootCause();       // boot causeを取得
  printBootCause(bc);                          // 引数をbcとして関数にとぶ
  pinMode(button5, INPUT_PULLUP);              // sleepを起こすpin
  attachInterrupt(button5, pushed5, FALLING);  // sleepを起こすには割り込み関数が必要
  LowPower.enableBootCause(button5);           // 起こすのに５ピンは有効
  
  /* ディスプレイ設定 */
  tft.begin();                                     // ディスプレイを初期化
  tft.setRotation(3);                              // ディスプレイの向きを3番にする、1番と検討
  tft.fillRect(0,0, 320, 240, ILI9341_BLACK);      // 背景の色
  tft.setTextSize(10);                             // テキストのサイズ
  tft.setCursor(120, 80);                          // setCursor(col.row) col:列 row:行
  tft.setTextColor(ILI9341_YELLOW);                // テキストの色
  tft.println("?");                                // ディスプレイに文字型を出力
  
  while (!theSD.begin()) { putStringOnLcd("Insert SD card", ILI9341_RED); } // エラーの処理、putStringOnLcd関数に引数Insert SD cardを赤文字で返して出力
  myFile = theSD.open("test.txt");
  if (myFile) {
    Serial.println("test.txt:");

    /* ファイルを読み取る */
    while (myFile.available()) {              // 1文字ずつ読み取る
      pre_value += char(myFile.read());       // 読み取る関数:前回起動時の値を読み取る
    }
    Serial.println(pre_value);                // この時点で文字型
    myFile.close();                           // いったんファイル閉じる
  } else {
    Serial.println("error opening test.txt"); // エラー処理
  }
    
  File nnbfile = theSD.open("model.nnb");        // 学習済みデータの読み込み

  /* AIの処理 */
  int ret = dnnrt.begin(nnbfile);                // DNNRT(組み込みAIライブラリ)を学習済みモデルで初期化（ランタイムライブラリ）
  if (ret < 0) {
    putStringOnLcd("dnnrt.begin failed" + String(ret), ILI9341_RED);     // エラーの処理、もしmodel.nnbファイルがなかったら初期化失敗と表示
    return;
  }
  
  /* カメラ処理 */
  theCamera.begin();                         // 液晶ディスプレイの開始 
  theCamera.startStreaming(true, CamCB);     // カメラのストリーミング開始CamCBに返す認識処理もCamCBで行っている
}

void loop() {
  /* 1曲目のデータを送り続ける */
  err = theAudio->writeFrames(AudioClass::Player0, myFile);  // FIFOに書き込み続ける
  /* CamCBのフラグを取得したら曲をストップして2曲目へ */
  if(flag == 1){
    theAudio->stopPlayer(AudioClass::Player0, AS_STOPPLAYER_NORMAL);     // オーディオをストップ 
    myFile.close();                                                      // ファイルを閉じて終了
    puts("Spound1.mp is closed");
    
    /* ディスプレイに数値を出力 */
    tft.fillRect(0,0, 320, 240, ILI9341_BLACK);
    tft.setTextSize(6);                      // テキストのサイズ
    tft.setCursor(20, 90);                   // setCursor(col.row) col:列 row:行
    tft.setTextColor(ILI9341_YELLOW);        // テキストカラー
    tft.println(Result_pre + String(" ") + String("kg"));              // ディスプレイに数値を出力
    Serial.println(Result_pre + String(" ") + String("kg"));           // シリアルモニタにも出力
    float Result_pre_flo = Result_pre.toFloat();

    float pre_value_flo = pre_value.toFloat();                         // prevalueをfloat型にした
    float difference = Result_pre_flo - pre_value_flo;                 // 今回と前回の差
    Serial.println(difference);                                        // 差を出力

    String difference_str =  String(difference,1);                     // floatからStringへ変換 

    if(difference <= 0){
      /* ディスプレイにも出力 */
      int len = difference_str.length();                                 // 整数lenを引数strの文字数として代入
      tft.setTextSize(3);                                                // テキストの文字の大きさ
      int sx = 160 - len/2*12;                                           // 文字の始まる列を計算によってもとめている
      if (sx < 0) sx = 0;                                                // もしマイナスを取ることがあったらそれは0と見ていい
      tft.setCursor(sx, 16);                                             // setCursor(col.row) col:列 row:行
      tft.setTextColor(ILI9341_YELLOW);                                  // テキストの色を引数であるcolorにする
      tft.println(difference_str+"kg");                                       // ディスプレイに文字型を出力
      // errはグローバル変数になってるから改めて定義する必要がある
      err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);
      myFile = theSD.open("Sound1.mp3");
      err = theAudio->writeFrames(AudioClass::Player0, myFile);   // SDカードからデータを読み込んでファイルに入れる
      theAudio->setVolume(50);
      theAudio->startPlayer(AudioClass::Player0);                 // SpresenseのFIFOの中のデータを引っ張って再生
      puts("start");
      while(1){
        err = theAudio->writeFrames(AudioClass::Player0, myFile);      // SDカードからデータを読み込んでファイルに入れる
        if (err){
          printf("Main player error code: %d\n", err);
          theAudio->stopPlayer(AudioClass::Player0);    // オーディオをストップ
          myFile.close();                               // ファイルを閉じて終了
          theAudio->setReadyMode();
          theAudio->end();                              // オーディオを終わらせて音漏れを防ぐ
          break;                                        // プログラムの流れをラベルのつけたところに移す
        }
      }
    }else{
      /* 2曲目 */
      /* ディスプレイにも出力 */
      int len = difference_str.length();                                 // 整数lenを引数strの文字数として代入
      tft.setTextSize(3);                                                // テキストの文字の大きさ
      int sx = 160 - len/2*12;                                           // 文字の始まる列を計算によってもとめている
      if (sx < 0) sx = 0;                                                // もしマイナスを取ることがあったらそれは0と見ていい
      tft.setCursor(sx, 16);                                             // setCursor(col.row) col:列 row:行
      tft.setTextColor(ILI9341_YELLOW);                                  // テキストの色を引数であるcolorにする
      tft.println("+"+difference_str+"kg");                                       // ディスプレイに文字型を出力
      // errはグローバル変数になってるから改めて定義する必要がある
      err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);
      myFile = theSD.open("Sound2.mp3"); 
      err = theAudio->writeFrames(AudioClass::Player0, myFile);   // SDカードからデータを読み込んでファイルに入れる
      theAudio->setVolume(50);
      theAudio->startPlayer(AudioClass::Player0);                 // SpresenseのFIFOの中のデータを引っ張って再生
      
      while(1){
        err = theAudio->writeFrames(AudioClass::Player0, myFile);      // SDカードからデータを読み込んでファイルに入れる
        if (err){
          printf("Main player error code: %d\n", err);
          theAudio->stopPlayer(AudioClass::Player0);                     // オーディオをストップ
          myFile.close();                               // ファイルを閉じて終了
          puts("close file");
          theAudio->setReadyMode();
          theAudio->end();                              // オーディオを終わらせて音漏れを防ぐ
          break;                                        // プログラムの流れをラベルのつけたところに移す
          }
        }
    }

    /* ファイルがあるならそれは消す */
    if (theSD.exists("test.txt")) {
      theSD.remove("test.txt");
    }

    /* ファイルを書く */
    myFile = theSD.open("test.txt", FILE_WRITE);

    if (myFile) {
      Serial.print("Writing to test.txt...");
      myFile.println(Result_pre_flo);
      /* Close the file */
      myFile.close();
      Serial.println("done.");
      }else{
      /* If the file didn't open, print an error */
      Serial.println("error opening test.txt");
      }
  
      /* WiFiをリセットする関数に飛ぶ */
      while (true) {
        byte err = setupWiFi();            // WiFiエラーなしが戻り値

        /* WiFiエラーの条件式 */
       if (err) {
          Serial.print("setup error:");  
          Serial.println((int)err);
          debugLoop();                     // エラーを表示する関数にとぶ
        }
        else if ((int)err == 0)            // エラーがなければループから抜けられる
          break;
      }
      getIP();                             // IPアドレスを取得する関数にとぶ

      String s = Result_pre;
      String SendData = s;
      int count = 0;

      ESP_GETpage(HOST, 8080, SendData);

      Serial.println(F("**********REPLY***********"));
      Serial.println(replybuffer);
      Serial.println(F("**************************"));

      while (true) {
        espreadline(3000);  // this is the 'echo' from the data
        Serial.print(">"); Serial.println(replybuffer); // probably the 'busy s...'

        count++;

        int i = 0;
        char buf[20];
        int ch[5];

        int time_left = 3 - count;

        String time_left_tx = String(time_left);                       // int を Stringに変換

        putStringOnLcd("connecting:"+time_left_tx, ILI9341_YELLOW);    // カウントダウンが見れる

        if (strstr(replybuffer, "busy s..."))
          continue;
        else if (strstr(replybuffer, "Recv"))
          break;
        else if (strstr(replybuffer, "CONNECT"))
          break;
        else if (count >= 3)                           // カウントする数
          break;
      }
      delay(200);
      Serial.println("finshed");
      tft.fillRect(0,0, 320, 240, ILI9341_BLACK);
      LowPower.coldSleep();                             // sleepする
  }

  /* ストリーミング時の処理お音楽を消す */
  if(flag == 2){
    Serial.println(flag);
    theAudio->stopPlayer(AudioClass::Player0, AS_STOPPLAYER_NORMAL);     // オーディオをストップ 
    myFile.close();                                                      // ファイルを閉じて終了
    theAudio->setReadyMode();
    theAudio->end();                                                     // オーディオを終わらせて音漏れを防ぐ
    while(1);
    }
}
