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
    UINT send_len = strlen(msg);
    puts_com(DID_USART3, msg, &send_len, TMO_FEVR);
}

static void set_led(int state){
    if (state){
        GPIOA->BSRR = (1 << (8 + 16)); // R19 (PA8) ON (Set low)
        debug_print("LED set to ON\r\n");
    }else{
        GPIOA->BSRR = (1 << 8); // R19 (PA8) OFF (Set high)
        debug_print("LED set to OFF\r\n");
    }
    led_state = state;
}

void com_tsk1(VP_INT exinf){
    static VB mojimoji[MOJI];
    UINT len;   // len �Ƃ��� unsigned int �^�̕ϐ���錾

    // debug_print("Initializing...\r\n"); // �������v���Z�X�̊J�n�������f�o�b�O���b�Z�[�W

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;        // RCC���W�X�^�𑀍삵��GPIOA�̃N���b�N��L���ɂ���
    debug_print("GPIOA clock enabled\r\n");     // GPIOA�̃N���b�N���L���ɂȂ������Ƃ������f�o�b�O���b�Z�[�W

    // GPIOA��MODER��ݒ�BPA8 �s�����o�̓��[�h�ɐݒ�
    GPIOA->MODER &= ~(3U << 16);
    GPIOA->MODER |= (1U << 16);

    GPIOA->OTYPER &= ~(1U << 8);        // GPIOA �� OTYPE��ݒ肵�APA8 ���v�b�V���v���o�̓��[�h�ɐݒ�
    GPIOA->OSPEEDR |= (3U << 16);       // GPIOA��OSPEEDER��ݒ肵�APA8 �̏o�͑��x���ō����x�ɐݒ�
    GPIOA->PUPDR &= ~(3U << 16);
    debug_print("PA8 configured as output\r\n");

    set_led(0);

    T_COM_SMOD debugCommSetting = {38400, BLEN8, PAR_NONE, SBIT1, FLW_NONE};
    ini_com(DID_USART3, &debugCommSetting);
    ctr_com(DID_USART3, STA_COM, 0);
    debug_print("USART3 initialized\r\n");

    debug_print("Initialization complete\r\n");

    for (int i = 0; i < 5; i++) {
        set_led(1);
        dly_tsk(500);
        set_led(0);
        dly_tsk(500);
    }
    debug_print("Initial LED test complete\r\n");
    debug_print("Waiting for commands...\r\n");

    for(;;){
        len = MOJI;     // 256��len�ɑ��
        memset(mojimoji, 0, MOJI);      // mojimoji�z���0�ŃN���A
        
        // �^�C���A�E�g��ݒ肵�� gets_com ���Ăяo��
        ER ercd = gets_com(DID_USART3, mojimoji, 0, CR_CODE, &len, TMO_FEVR);
          // DID_USART3 : �g�p����USART�̃f�o�C�XID
          // mojimoji   : ��M�����f�[�^���i�[����z��
          // 0          : �J�n�ʒu�i�o�b�t�@�̐擪����j
          // CR_CODE    : �I���R�[�h�i�����炭�L�����b�W���^�[���j
          // &len       : ���ۂɎ�M�����f�[�^�̒������i�[����|�C���^
          // TMO_FEVR   : �^�C���A�E�g�ݒ�i�����ł͉i�v�ɑҋ@�j
        
        if (ercd == E_OK && len > 0){  // �R�}���h�̎�M����������M�����f�[�^�̒�����0���傫���ꍇ
            debug_print("Received command: ");
            debug_print(mojimoji);
            debug_print("\r\n");        

            if (strncmp(mojimoji, "#$0XC01", 7) == 0){         // ��M�����R�}���h�� "#$0XC01"���m�F
                if (led_state){        // ����LED��ON�̏ꍇ
                    debug_print("LED is already ON\r\n");
                }else{          // LED��OFF�̏ꍇ
                    set_led(1);         // LED����������LED set to OFF�ƕ\��
                }
            }else if (strncmp(mojimoji, "#$0XC00", 7) == 0){    // ��M�����R�}���h�� "#$0XC00"���m�F
                if (!led_state){       // ����LED��OFF�̏ꍇ
                    debug_print("LED is already OFF\r\n");
                }else{          // LED��ON�̏ꍇ
                    set_led(0);         // LED����������LED set to OFF�ƕ\��
                }
            }else{      // ��M�����R�}���h�� "#$0XC01"��"#$0XC00"�ȊO�̏ꍇ
                debug_print("Error: Invalid command\r\n");
            }
            debug_print("Waiting for next command...\r\n");
        }
    }
}