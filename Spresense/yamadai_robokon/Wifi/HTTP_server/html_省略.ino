//は説明文　　/*は必要ないコード
#include <GS2200Hal.h>
#include <GS2200AtCmd.h>
#include <TelitWiFi.h>

#ifndef _CONFIG_H_
#define _CONFIG_H_
#define  AP_SSID        "GS2200_LIMITED_AP"
#define  PASSPHRASE     "123456789"
#define  AP_CHANNEL     6
#define  TCPSRVR_PORT   "80"　　// http:192.168.1.99　←これを検索すればアクセスできる
#endif /*_CONFIG_H_*/
#define  CONSOLE_BAUDRATE  115200

extern uint8_t ESCBuffer[];
extern uint32_t ESCBufferCnt;

TelitWiFi gs2200;
TWIFI_Params gsparams;

// webページの構築（21~34行まで）,HTTP GETリクエストに応答するデータの生成
String header         = " HTTP/1.1 200 OK \r\n";
String content        = "<!DOCTYPE html><html> Go!Go!</html>\r\n";//htmlでGO!の文字列を表示
String content_type   = "Content-type: text/html \r\n";
String content_length = "Content-Length: " + String(content.length()) + "\r\n\r\n";

int send_contents(char* ptr, int size)
{  
  String str = header + content_type + content_length + content;
  size = (str.length() > size)? size : str.length();
  str.getBytes(ptr, size);
  
  return str.length();
}

void setup() {
 
  Serial.begin( CONSOLE_BAUDRATE ); 

  //今度の他のライブラリとの接続を踏まえて本家と少しAPIを変える。
  Init_GS2200_SPI_type(iS110B_TypeC);//GS2200が使用するSPIの初期化

  //ATコマンドを使いAPモードを指定
  gsparams.mode = ATCMD_MODE_LIMITED_AP;
  gsparams.psave = ATCMD_PSAVE_DEFAULT;
  if( gs2200.begin( gsparams ) ){
   /* ConsoleLog( "GS2200 Initilization Fails" ); */
    while(1);
  }

  //APモードとして働くために必要な情報指定
  if( gs2200.activate_ap( AP_SSID, PASSPHRASE, AP_CHANNEL ) ){
    /*ConsoleLog( "WiFi Network Fails" ); */
    while(1);
  }
}

void loop() {

  ATCMD_RESP_E resp;
  char server_cid = 0, remote_cid=0;
  ATCMD_NetworkStatus networkStatus;
  uint32_t timer=0;

  resp = ATCMD_RESP_UNMATCH;
  /*ConsoleLog( "Start TCP Server");*/

  //TCPサーバー起動
  resp = AtCmd_NSTCP( TCPSRVR_PORT, &server_cid);
  /*if (resp != ATCMD_RESP_OK) {
    ConsoleLog( "No Connect!" );
    delay(2000);
    return;
  }*/

  while( 1 ){
     /*ConsoleLog( "Waiting for TCP Client"); */

    //クライアントの接続を待つ
    if( ATCMD_RESP_TCP_SERVER_CONNECT != WaitForTCPConnection( &remote_cid, 5000 ) ){
      continue;
    }

   /*ConsoleLog( "TCP Client Connected");*/
    // 受信データの次のchunckに備える
    WiFi_InitESCBuffer();
    sleep(1);

    // Start the echo server
    while( Get_GPIO37Status() ){ //Wi－Fiからの要求がある間ループ
      resp = AtCmd_RecvResponse();　//Wi－Fiからの要求の読み出し

      if( ATCMD_RESP_BULK_DATA_RX == resp ){　//読み出すデータがある場合
        if( Check_CID( remote_cid ) ){
          ConsolePrintf( "Received : %s\r\n", ESCBuffer+1 );

          String message = ESCBuffer+1;
          if (message.substring(0, message.indexOf(' ')) == "GET") {　//GETリクエストであるか確認
            int length = send_contents(ESCBuffer+1,MAX_RECEIVED_DATA);        
            ConsolePrintf( "Will send : %s\r\n", ESCBuffer+1 );
            if( ATCMD_RESP_OK != AtCmd_SendBulkData( remote_cid, ESCBuffer+1, length ) ){　//コンテンツを作成し送信
              delay(10);
            }
          }
        }          
        WiFi_InitESCBuffer();
      }
    }
  }
}