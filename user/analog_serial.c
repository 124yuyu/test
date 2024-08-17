#include "kernel.h"
#include "stm32f4xx.h"
#include "DDR_STM32F4_USART0_cfg.h"
#include "DDR_COM.h"
#include <string.h>
#include "device_id.h"

#define CR_CODE 0x0D
#define MOJI 256

static int led_state = 0;

static void debug_print(const char* msg){
    UINT send_len = strlen(msg);        // strlen(msg)で引数として渡された文字列の長さを計算しsend_lenに格納
    
    // puts_com 関数を呼び出して、メッセージを送信
    puts_com(DID_USART3, msg, &send_len, TMO_FEVR);
      // DID_USART3     : 使用するUSARTデバイスのIDを指定
      // msg            : 送信するメッセージ（文字列）
      // &send_len      : 文字列の長さへのポインタ
      // TMO_FEVR       : タイムアウト設定（ここでは永久に待機）
}

static void set_led(int state){
    // int state  : LEDの状態（オンかオフか）を指定する整数型の引数
  
    if (state){         // state が0以外（真）の場合、LEDオン
        GPIOA->BSRR = (1 << (8 + 16)); // R19 (PA8) ON (Set low)　24ビット目を1にセット
        debug_print("LED set to ON\r\n");
    }else{             // state が0（偽）の場合、LEDオフ
        GPIOA->BSRR = (1 << 8); // R19 (PA8) OFF (Set high)　8ビット目に1をセット
        debug_print("LED set to OFF\r\n");
    }
    led_state = state;  // グローバル変数 led_state に現在のLEDの状態を保存
}

void com_tsk1(VP_INT exinf){
    static VB mojimoji[MOJI];
    UINT len;   // len という unsigned int 型の変数を宣言

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;        // RCCレジスタを操作してGPIOAのクロックを有効にする
    debug_print("GPIOA clock enabled\r\n");     // GPIOAのクロックが有効になったことを示すデバッグメッセージ

    // GPIOAのMODERを設定。PA8 ピンを出力モードに設定
    GPIOA->MODER &= ~(3U << 16);
    GPIOA->MODER |= (1U << 16);

    GPIOA->OTYPER &= ~(1U << 8);        // GPIOAのOTYPEを設定し、PA8をプッシュプル出力モードに設定
    GPIOA->OSPEEDR |= (3U << 16);       // GPIOAのOSPEEDERを設定し、PA8の出力速度を最高速度に設定
    GPIOA->PUPDR &= ~(3U << 16);        // GPIOAのPUPDRを設定し、PA8のプルアップ/プルダウン抵抗を無効
    debug_print("PA8 configured as output\r\n");

    set_led(0); // 初期状態：消灯

    T_COM_SMOD debugCommSetting = {38400, BLEN8, PAR_NONE, SBIT1, FLW_NONE};    // USART3の通信設定を定義
    ini_com(DID_USART3, &debugCommSetting);     // 上記の設定を使用してUSART3を初期化
    ctr_com(DID_USART3, STA_COM, 0);    // USART3の通信を開始
    debug_print("USART3 initialized\r\n");      // USART3の初期化が完了したことを示すデバッグメッセージ

    for (int i = 0; i < 5; i++) {       // LED を5回点滅(テスト)
        set_led(1);
        dly_tsk(500);
        set_led(0);
        dly_tsk(500);
    }
    
    debug_print("Waiting for commands...\r\n"); // コマンド入力待ちの状態なったことを示すデバッグメッセージ

    for(;;){
        len = MOJI;     // 256をlenに代入
        memset(mojimoji, 0, MOJI);      // mojimoji配列を0でクリア
        
        // タイムアウトを設定して gets_com を呼び出す
        ER ercd = gets_com(DID_USART3, mojimoji, 0, CR_CODE, &len, TMO_FEVR);
          // DID_USART3 : 使用するUSARTのデバイスID
          // mojimoji   : 受信したデータを格納する配列
          // 0          : 開始位置（バッファの先頭から）
          // CR_CODE    : 終了コード（おそらくキャリッジリターン）
          // &len       : 実際に受信したデータの長さを格納するポインタ
          // TMO_FEVR   : タイムアウト設定（ここでは永久に待機）
        
        if (ercd == E_OK && len > 0){  // コマンドの受信が成功し受信したデータの長さが0より大きい場合
            debug_print("Received command: ");
            debug_print(mojimoji);
            debug_print("\r\n");        

            if (strncmp(mojimoji, "#$0XC01", 7) == 0){         // 受信したコマンドが "#$0XC01"か確認
                if (led_state){        // 既にLEDがONの場合
                    debug_print("LED is already ON\r\n");
                }else{          // LEDがOFFの場合
                    set_led(1);         // LEDを消灯させLED set to OFFと表示
                }
            }else if (strncmp(mojimoji, "#$0XC00", 7) == 0){    // 受信したコマンドが "#$0XC00"か確認
                if (!led_state){       // 既にLEDがOFFの場合
                    debug_print("LED is already OFF\r\n");
                }else{          // LEDがONの場合
                    set_led(0);         // LEDを消灯させLED set to OFFと表示
                    debug_print("LED set to OFF\r\n");
                }
            }else{      // 受信したコマンドが "#$0XC01"と"#$0XC00"以外の場合
                debug_print("Error: Invalid command\r\n");
            }
            debug_print("Waiting for next command...\r\n");     // コマンド入力待ちの状態なったことを示すデバッグメッセージ
        }
    }
}