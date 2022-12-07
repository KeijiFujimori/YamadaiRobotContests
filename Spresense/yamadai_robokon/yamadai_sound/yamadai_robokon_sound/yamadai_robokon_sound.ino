#include <SDHCI.h>                                                              // SDカードを使うときに必要
#include <Audio.h>                                                              // オーディオ機能を使うときに必要

SDClass theSD;                                                                  // クラスをインスタンスに変えた：SDClassはSDカードにアクセスしファイルとディレクトリを操作
AudioClass *theAudio;                                                           // クラスをデータを格納するポインターとしてインスタンスにした

File myFile;                                                                    // FileインスタスをmyFileクラスとした

bool ErrEnd = false;                                                            // bool型(真か偽の2種類しか値を取らないこと)：ErrEndは偽として定義した

//**************************************************************************************//
//                  オーディオ内部でエラーが発生したときに呼び出される関数                         //
//**************************************************************************************//
static void audio_attention_cb(const ErrorAttentionParam *atprm)                
{
  puts("Attention!");                                                           // 文字を表示
  
  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING)                           
    {
      ErrEnd = true;                                                            // 上で定義したErrEndをTrueにする
   }
}

void setup()
{
  /* Initialize SD */
  while (!theSD.begin())                                                                   // SDのインスタンスを初期化できないならループする
    {
      Serial.println("Insert SD card.");                                                   // SDカードがささっていないと表示
    }
    
  theAudio = AudioClass::getInstance();                                                    // オーディオをスタートする

  theAudio->begin(audio_attention_cb);                                                     // オーディオを初期化してaudio_attention_cbを呼び出す()の中は空欄でもいいらしい

  puts("initialization Audio Library");                                                    // ライブラリを初期化したと知らせる

  /* Set clock mode to normal */
  theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);                                      // クロックモードの設定、AS_CLKMODE_NORMAL:49Hz以下とAS_CLKMODE_HIRES:90Hz異常の2つの設定がある
                                                                                           

  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_LINEOUT);         // 出力先：スピーカ出力に設定、出力サイズ:スピーカ出力をラインアウトレベル設定(ジャックを使うならこれ)

  err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);
                                                                                            // Player0を初期化,デコーダをMP3似設定,MP3のデコーダのありかを示す,サンプリングレートを自動生成,ステレオ(2ch)に設定

  /* Verify player initialize */
  if (err != AUDIOLIB_ECODE_OK)
    {
      printf("Player0 initialize error\n");
      exit(1);
    }
    
  myFile = theSD.open("Sound1.mp3");                                                         // Sound.mp3を開く

  if (!myFile)
    {
      printf("File open error\n");
      exit(1);
    }
  printf("Open! 0x%08lx\n", (uint32_t)myFile);

  err = theAudio->writeFrames(AudioClass::Player0, myFile);                                 // FIFOにデータがないとplayerが止まるので予めwriteFramesという関数を呼ぶ

  if ((err != AUDIOLIB_ECODE_OK) && (err != AUDIOLIB_ECODE_FILEEND))
    {
      printf("File Read Error! =%d\n",err);
      myFile.close();
      exit(1);
    }

  puts("Play!");                                                                               // 再生を知らせる

  theAudio->setVolume(-160);                                                                   // ボリュームの設定：-16.0dB
  theAudio->startPlayer(AudioClass::Player0);                                                  // SpresenseのFIFOの中のデータを引っ張って再生
}

void loop()
{
  puts("loop!!");

  int err = theAudio->writeFrames(AudioClass::Player0, myFile);                                 // SDカードからデータを読み込んでファイルに入れる

  if (err == AUDIOLIB_ECODE_FILEEND)                                                            // 曲が終わったら知らせる
    {
      printf("Main player File End!\n");
    }

  /* Show error code from player and stop */
  if (err)                                                                                      // エラーしたら曲を止める
    {
      printf("Main player error code: %d\n", err);
      goto stop_player;                                                                         // プログラムの流れをラベルのつけたところに移す
    }

  if (ErrEnd)
    {
      printf("Error End\n");                                                           // もし偽であるならplayerを止める
      goto stop_player;
    }

  usleep(40000);                                                                          // 40秒FIFOの中の楽曲データの処理を待っている
  return;                                                                                 // 先に進まずプレイを実行

stop_player:                                                                              // label:　stop_playerというラベル
  theAudio->stopPlayer(AudioClass::Player0);                                              // オーディををストップ
  myFile.close();
  theAudio->setReadyMode();
  theAudio->end();
  exit(1);
}
