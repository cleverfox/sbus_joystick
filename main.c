#include "includes.h"

#define _nop_() __asm__("nop")
void x_delay(unsigned int i){ 
      while (--i) _nop_();
} 
#define delay_480us() x_delay(1360)

//#define TXTDEBUG

void cout(const char c) {
  //parity calculator as workaround for broken parity generator in stm8s103k3
  uint8_t bits=0;
  uint8_t c1=c;
  while(c1){
    if(c1&1) bits++;
    c1>>=1;
  };
  if(bits % 2){
    ((UART1_TypeDef *) 0x5230)->CR1 |= 0x40;
  }else{
    ((UART1_TypeDef *) 0x5230)->CR1 &= ~0x40;
  }
  
  while(!(UART1->SR & UART1_SR_TXE)) {}
  //UART1->CR2 &= (uint8_t)(~UART1_CR2_REN);  
  UART1_SendData8(c);
  while(!(UART1->SR & UART1_SR_TXE)) {}
  //delay_480us();
  //UART1->CR2 |= (uint8_t)UART1_CR2_REN;  
}

#ifdef TXTDEBUG
void UART_Send(const char *s) {
    char c;
    while ( (c = *s++) ) {
        cout(c);
    }
}

void xcout(uint16_t i){
  int c;
  for(c=4;c>0;c--){
    uint16_t q=(i&0xF000)>>12;
    if(q<10){
      cout('0'+q);
    }else{
      cout('A'+q-10);
    }
    i<<=4;
  }
}
#endif


void UART1_RX_vector(void) __interrupt(18) {
    while(UART1_GetITStatus(UART1_IT_RXNE)){
        char c=UART1_ReceiveData8();
        if(c=='R'){
            cout('O');
            cout('K');
            cout('\r');
            cout('\n');
        }
    }
}

void TIM2OVF(void) __interrupt(13) { //overflow
    if(TIM2_GetITStatus(TIM2_IT_UPDATE)==SET){
        TIM2_ClearITPendingBit(TIM2_IT_UPDATE);
    }
    //just wakeup
}

void PORTC_EXTI(void) __interrupt(5) {
  //do nothing, just wake up
}



int main(void) {
  uint8_t port;
  uint8_t btn=0;
  uint16_t chans[5]={837,978,978,978,978};
  uint8_t b[7];
  CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV1);

  UART1_DeInit();
  //parity generator in UART broken in atm8s103, so, I calculate parity
  //by software
  UART1_Init(9, 15, UART1_WORDLENGTH_9D, UART1_STOPBITS_2, UART1_PARITY_NO,
      UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);

  /*
  ((UART1_TypeDef *) 0x5230)->CR1 = 0x04;
  ((UART1_TypeDef *) 0x5230)->CR2 = 0x08;
  ((UART1_TypeDef *) 0x5230)->CR3 = 0x20;
  */
  UART1_Cmd(ENABLE);

  UART1_ITConfig(UART1_IT_RXNE_OR,ENABLE);
  //GPIO_Init(GPIOD, GPIO_PIN_4, GPIO_MODE_IN_PU_NO_IT); //Master/Slave pin

  EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOC,EXTI_SENSITIVITY_RISE_FALL);

  GPIO_Init(GPIOC, 
      GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5, 
      GPIO_MODE_IN_PU_IT); //BUTTON

  CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, DISABLE);
  CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE);
  CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER4, DISABLE);
  CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, DISABLE);
  CLK_PeripheralClockConfig(CLK_PERIPHERAL_SPI, DISABLE);
  CLK_PeripheralClockConfig(CLK_PERIPHERAL_ADC, DISABLE);

#ifdef TXTDEBUG
  TIM2_TimeBaseInit(TIM2_PRESCALER_256, 4096);
#else
  TIM2_TimeBaseInit(TIM2_PRESCALER_256, 1024);
#endif
  TIM2_Cmd(ENABLE);
  TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);

  //enableInterrupts();

#ifdef TXTDEBUG
  UART_Send("Preved\r\n");
#endif

  while(1){
    port=(~GPIO_ReadInputData(GPIOC)) & 0x3e;
    //U8 R2 L20 D10 B4
    if(port & 2) chans[0]=0x70f;
    else if(port & 0x20) chans[0]=0x0f0;
    else chans[0]=0x345;

    if(port & 8) chans[1]=0x70f;
    else if(port & 0x10) chans[1]=0x0f0;
    else chans[1]=978;

    if(port&4){
      if(!btn){
        if(chans[4] > 1024)
          chans[4]=0x0f0;
        else
          chans[4]=0x70f;
        btn=1;
      }
    }else{
      btn=0;
    }

    b[0] = chans[0];
    b[1] = (chans[0] >> 8) | (chans[1] << 3)  ;
    b[2] = (chans[1] >> 5) | (chans[2] << 6);
    b[3] = chans[2] >> 2;
    b[4] = chans[2] >> 10 | chans[3]<<1;
    b[5] = chans[2] >> 7 | chans[4]<<4;
    b[6] = chans[4]>>4;

#ifdef TXTDEBUG
    xcout(port);
    xcout(chans[0]);
    xcout(chans[1]);
    xcout(chans[2]);
    UART_Send(".");

    cout('\r');
    cout('\n');

#else
    cout(0x0f); //SOF
    cout(b[0]);
    cout(b[1]);
    cout(b[2]);
    cout(b[3]);
    cout(b[4]);
    cout(b[5]);
    cout(b[6]);

    cout(0xE8);
    cout(0xC0);

    cout(0x03);
    cout(0x1E);
    cout(0xF0);
    cout(0x80);
    cout(0x07);
    cout(0x00);
    cout(0x01);
    cout(0x08);
    cout(0x40);
    cout(0x00);

    cout(0x02);
    cout(0x10);
    cout(0x80);
    cout(0x00);
    cout(0x00);
#endif

delay_480us();
delay_480us();
delay_480us();
delay_480us();
delay_480us();
delay_480us();
delay_480us();
delay_480us();
delay_480us();
//    __asm__("wfi");
  }
}



