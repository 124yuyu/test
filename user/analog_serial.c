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
    UINT send_len = strlen(msg);        // strlen(msg)�ň����Ƃ��ēn���ꂽ������̒������v�Z��send_len�Ɋi�[
    
    // puts_com �֐����Ăяo���āA���b�Z�[�W�𑗐M
    puts_com(DID_USART3, msg, &send_len, TMO_FEVR);
      // DID_USART3     : �g�p����USART�f�o�C�X��ID���w��
      // msg            : ���M���郁�b�Z�[�W�i������j
      // &send_len      : ������̒����ւ̃|�C���^
      // TMO_FEVR       : �^�C���A�E�g�ݒ�i�����ł͉i�v�ɑҋ@�j
}

static void set_led(int state){
    // int state  : LED�̏�ԁi�I�����I�t���j���w�肷�鐮���^�̈���
  
    if (state){         // state ��0�ȊO�i�^�j�̏ꍇ�ALED�I��
        GPIOA->BSRR = (1 << (8 + 16)); // R19 (PA8) ON (Set low)�@24�r�b�g�ڂ�1�ɃZ�b�g
        debug_print("LED set to ON\r\n");
    }else{             // state ��0�i�U�j�̏ꍇ�ALED�I�t
        GPIOA->BSRR = (1 << 8); // R19 (PA8) OFF (Set high)�@8�r�b�g�ڂ�1���Z�b�g
        debug_print("LED set to OFF\r\n");
    }
    led_state = state;  // �O���[�o���ϐ� led_state �Ɍ��݂�LED�̏�Ԃ�ۑ�
}

void com_tsk1(VP_INT exinf){
    static VB mojimoji[MOJI];
    UINT len;   // len �Ƃ��� unsigned int �^�̕ϐ���錾

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;        // RCC���W�X�^�𑀍삵��GPIOA�̃N���b�N��L���ɂ���
    debug_print("GPIOA clock enabled\r\n");     // GPIOA�̃N���b�N���L���ɂȂ������Ƃ������f�o�b�O���b�Z�[�W

    // GPIOA��MODER��ݒ�BPA8 �s�����o�̓��[�h�ɐݒ�
    GPIOA->MODER &= ~(3U << 16);
    GPIOA->MODER |= (1U << 16);

    GPIOA->OTYPER &= ~(1U << 8);        // GPIOA��OTYPE��ݒ肵�APA8���v�b�V���v���o�̓��[�h�ɐݒ�
    GPIOA->OSPEEDR |= (3U << 16);       // GPIOA��OSPEEDER��ݒ肵�APA8�̏o�͑��x���ō����x�ɐݒ�
    GPIOA->PUPDR &= ~(3U << 16);        // GPIOA��PUPDR��ݒ肵�APA8�̃v���A�b�v/�v���_�E����R�𖳌�
    debug_print("PA8 configured as output\r\n");

    set_led(0); // ������ԁF����

    T_COM_SMOD debugCommSetting = {38400, BLEN8, PAR_NONE, SBIT1, FLW_NONE};    // USART3�̒ʐM�ݒ���`
    ini_com(DID_USART3, &debugCommSetting);     // ��L�̐ݒ���g�p����USART3��������
    ctr_com(DID_USART3, STA_COM, 0);    // USART3�̒ʐM���J�n
    debug_print("USART3 initialized\r\n");      // USART3�̏������������������Ƃ������f�o�b�O���b�Z�[�W

    for (int i = 0; i < 5; i++) {       // LED ��5��_��(�e�X�g)
        set_led(1);
        dly_tsk(500);
        set_led(0);
        dly_tsk(500);
    }
    
    debug_print("Waiting for commands...\r\n"); // �R�}���h���͑҂��̏�ԂȂ������Ƃ������f�o�b�O���b�Z�[�W

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
                    debug_print("LED set to OFF\r\n");
                }
            }else{      // ��M�����R�}���h�� "#$0XC01"��"#$0XC00"�ȊO�̏ꍇ
                debug_print("Error: Invalid command\r\n");
            }
            debug_print("Waiting for next command...\r\n");     // �R�}���h���͑҂��̏�ԂȂ������Ƃ������f�o�b�O���b�Z�[�W
        }
    }
}