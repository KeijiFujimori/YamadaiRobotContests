/* 一曲がループしている中でスイッチを押すと2曲目に変わり音楽の終わりと共に終了するプログラム */

#include <SDHCI.h>         // SDカードを使うときに必要
#include <Audio.h>         // オーディオ機能を使うときに必要

SDClass theSD;             // クラスをインスタンスに変えた：SDClassはSDカードにアクセスしファイルとディレクトリを操作
AudioClass *theAudio;      // クラスをデータを格納するポインターとしてインスタンスにした

File myFile;               // FileインスタスをmyFileクラスとした

bool bButtonPressed = false;    // bool型(真か偽の2種類しか値を取らないこと)：ErrEndは偽として定義した
int intPin = 4;

//******************************************************//
//            changeState関数：bButtonPressedを真にする    //
//******************************************************//
void changeState () {
  bButtonPressed = true;
}

void setup()
{

  Serial.begin(115200);                                                  // 通信速度115200

  attachInterrupt(digitalPinToInterrupt(intPin) ,changeState ,FALLING);  // 割り込み設定
    
  theAudio = AudioClass::getInstance();                                  // オーディオをスタートする

  theAudio->begin();                                                     // オーディオを初期化してaudio_attention_cbを呼び出す()の中は空欄でもいいらしい


  theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);                    // クロックモードの設定、AS_CLKMODE_NORMAL:49Hz以下とAS_CLKMODE_HIRES:90Hz異常の2つの設定がある
                                                                                           

  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_LINEOUT);           // 出力先：スピーカ出力に設定、出力サイズ:スピーカ出力をラインアウトレベル設定(ジャックを使うならこれ)
 
  theAudio->setVolume(-60);                                                                 // ボリュームの設定：-6.0dB

  // Player0を初期化,デコーダをMP3似設定,MP3のデコーダのありかを示す,サンプリングレートを自動生成,ステレオ(2ch)に設定
  err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);


   myFile = theSD.open("Sound0.mp3");   // ファイルを開く
   err = theAudio->writeFrames(AudioClass::Player0, myFile);  // とりあえずなんかFIFOに入れておく
   puts("FIFo1");
   
  while(1){

      puts("file open");

      if(bButtonPressed == true){
      theAudio->stopPlayer(AudioClass::Player0);                // trueならbreak
      myFile.close();
      Serial.println("Button is pushed");
      goto second_song;
      }
    


      if(bButtonPressed == true){
      theAudio->stopPlayer(AudioClass::Player0);                // trueならbreak
      myFile.close();
      Serial.println("Button is pushed");
      goto second_song;
      }
    
      theAudio->startPlayer(AudioClass::Player0); // SpresenseのFIFOの中のデータを引っ張って再生

      puts("プレイヤーをスタート");
      
      err = theAudio->writeFrames(AudioClass::Player0, myFile);  // FIFOに書き込み続ける
      puts("FIFO2");

      if(bButtonPressed == true){
      theAudio->stopPlayer(AudioClass::Player0);                // trueならbreak
      myFile.close();
      Serial.println("Button is pushed");
      goto second_song;
      }

      
      
      if (err == AUDIOLIB_ECODE_FILEEND){
          //theAudio->stopPlayer(AudioClass::Player0);              // 曲をストップ
          //myFile.close();                                         // ファイルを閉じる

          if(bButtonPressed == true){

      Serial.println("Button is pushed");

      goto second_song;
      }

          puts("曲終わりもう一回");
          }
       }

       /****************2曲目に入る*********************************************************/

     second_song:
     
       myFile = theSD.open("Sound1.mp3"); 

       err = theAudio->writeFrames(AudioClass::Player0, myFile);                             // SDカードからデータを読み込んでファイルに入れる
       
       theAudio->startPlayer(AudioClass::Player0);                                            // SpresenseのFIFOの中のデータを引っ張って再生
       while(1){
        err = theAudio->writeFrames(AudioClass::Player0, myFile);                             // SDカードからデータを読み込んでファイルに入れる
        if (err == AUDIOLIB_ECODE_FILEEND){
          break;                                                                              // ループをbreak
          }
        }
       theAudio->stopPlayer(AudioClass::Player0);                                              // オーディオをストップ 
       myFile.close();                                                                         // ファイルを閉じて終了
       theAudio->setReadyMode();
       theAudio->end();
       Serial.println("finshed");
}

void loop()
{
       
}
