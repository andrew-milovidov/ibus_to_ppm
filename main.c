#include "iostm8s103f3.h"
#include "stdint.h"
//#include "intrinsics.h"

//-------------
//option byte, for the future
typedef struct
{
  unsigned char AFR0        : 1;
  unsigned char AFR1        : 1;
  unsigned char AFR2        : 1;
  unsigned char AFR3        : 1;
  unsigned char AFR4        : 1;
  unsigned char AFR5        : 1;
  unsigned char AFR6        : 1;
  unsigned char AFR7        : 1;
} __BITS_AFR_OPT2;
__IO_REG8_BIT(AFR_OPT2,      0x4803, __READ_WRITE, __BITS_AFR_OPT2);
//-------------

#define BUFFSIZE 32
#define MAX_CHANNEL 10
#define SYNCBYTE 0x20
#define UART_DIVIDER 70//8000000 / 115200

uint8_t data[BUFFSIZE];
uint8_t position = 0;
uint8_t receivedData = 0;
uint8_t temp = 0, i = 0;
uint16_t chksum = 0xFFFF;
uint16_t rxsum = 0;
  
uint8_t do_rx_byte()
{
  while (!UART1_SR_RXNE)
  {
    //asm("nop");
    //asm("nop");
  }
  uint8_t data1 = UART1_DR;
  return data1;
}

void outChannel1(uint8_t pwrH, uint8_t pwrL)
{
  TIM1_CCR1H = pwrH;
  TIM1_CCR1L = pwrL;
}

void outChannel2(uint8_t pwrH, uint8_t pwrL)
{
  TIM1_CCR2H = pwrH;
  TIM1_CCR2L = pwrL;
}


void outChannel3(uint8_t pwrH, uint8_t pwrL)
{
  TIM1_CCR3H = pwrH;
  TIM1_CCR3L = pwrL;
}

void outChannel4(uint8_t pwrH, uint8_t pwrL)
{
  TIM1_CCR4H = pwrH;
  TIM1_CCR4L = pwrL;
}

void processReceivedByte(uint8_t v)
{
  if (position == 0 && v != SYNCBYTE)
    return;
  data[position] = v;
  if (position == BUFFSIZE - 1)
  {
    chksum = 0xFFFF;
    for (i = 0; i < BUFFSIZE - 2; i++)
    {
      chksum -= data[i];
    }
    rxsum = data[30] + (data[31] << 8);
    if (rxsum == chksum)
    {
      outChannel1(data[15], data[14]);
      outChannel2(data[17], data[16]);
      outChannel3(data[19], data[18]);
      outChannel4(data[21], data[20]);
    }
    position = 0;
  }
  else
  {
    position++;
  }
}

int main( void )
{
  //setup clock
  CLK_CKDIVR_bit.HSIDIV = 1; //div by 2 (8 MHz)
  CLK_CKDIVR_bit.CPUDIV = 0; //still 8 MHz
  //setup GPIO
  PD_DDR = 0;//all input
  PD_CR1 = 0x00;
  PD_CR2_bit.C26 = 0;
  //PD_CR1_C16 = 0;
  PC_DDR = 1;//all output
  PC_CR1 = 1;//push-pull
  PC_ODR = 0;//set output to 0 to all
  //timer setup
  //CLK_PCKENR1 |= (1 << 5);//enable usart clock gate (not need, on stm8s all gates are open)
  //we need 50 Hz with 1..2 ms pwm
  TIM1_PSCRH = 7 >> 8;
  TIM1_PSCRL = 7 & 0xFF;
  TIM1_ARRH = 20000 >> 8;
  TIM1_ARRL = 20000 && 8;
  TIM1_CR1_ARPE = 0;//autoreload enable
  TIM1_CCER1_CC1P = 0;// Active high
  TIM1_CCER1_CC2P = 0;
  TIM1_CCER2_CC3P = 0;
  TIM1_CCER2_CC4P = 0;
  TIM1_CCER1_CC1E = 1;// enable compare mode
  TIM1_CCER1_CC2E = 1;
  TIM1_CCER2_CC3E = 1;
  TIM1_CCER2_CC4E = 1;
  TIM1_CCMR1_OC1M = 6;//PWM1 mode
  TIM1_CCMR2_OC2M = 6;
  TIM1_CCMR3_OC3M = 6;
  TIM1_CCMR4_OC4M = 6;
  TIM1_CCMR1_OC1PE = 1;//pwm enable
  TIM1_CCMR2_OC2PE = 1;
  TIM1_CCMR3_OC3PE = 1;
  TIM1_CCMR4_OC4PE = 1;
  outChannel1(3, 0xE8);//set default values - 1000
  outChannel2(3, 0xE8);
  outChannel3(3, 0xE8);
  outChannel4(3, 0xE8);
  TIM1_CR1_CEN = 1;//start timer
  TIM1_BKR_MOE = 1;//main output enable (this was a reason of one broken day)))  
  //setup uart
  //our clock div is 69,44 ~70
  UART1_BRR2 = UART_DIVIDER & 0x000F;
  UART1_BRR2 |= UART_DIVIDER >> 12; 
  UART1_BRR1 = (UART_DIVIDER >> 4) & 0x00FF;
  UART1_CR1 = 0;//uart control registers
  UART1_CR2 = 0;
  UART1_CR2_REN = 1;//start receiver;
  UART1_CR3 = 0;
  //main cycle
  while (1) 
  {
    processReceivedByte(do_rx_byte());
  }
}
