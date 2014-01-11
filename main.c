#include <msp430.h>

#define TXLED  (1U<<0)
#define ERRLED (1U<<6)
#define TXD    (1U<<2)
#define CAPT   (1U<<3)

enum trans_source {FROM_TIMER=1, FROM_EDGE=0};

static unsigned int current; /* Zero or non-zero */

#define BUFSIZE 256
#if (BUFSIZE & (BUFSIZE-1))
#error BUFSIZE should be a power of 2
#endif
static volatile char tx_buf [BUFSIZE];
static volatile unsigned char wri=0, rdi=0;
/* Empty when equal */
static volatile int isTransmitting = 0;
__attribute__((interrupt(USCIAB0TX_VECTOR))) void USCI0TX_ISR(void)
{
   __bic_SR_register((unsigned)GIE);
   UC0IFG &= ~UCA0TXIFG;
   if(rdi != wri) {
      rdi = (rdi + 1) & (BUFSIZE-1);
      UCA0TXBUF = (unsigned char) tx_buf[rdi];
   }
   else {
      isTransmitting = 0;
   }
   __bis_SR_register((unsigned)GIE);
}

static void uart_putchar(char c) {
   /* Only use with disabled interrupts */
   if(!isTransmitting) {
      isTransmitting = 1;
      UCA0TXBUF = (unsigned char) c;
   }
   else {
      wri = (wri + 1) & (BUFSIZE-1);
      if(wri == rdi) {
         /* ERROR, buffer is full */
         P1OUT |= ERRLED;
         for(;;)
            ;
      }
      tx_buf[wri] = c;
   }
}

static void uart_puts(char *s) {
   /* Only use with disabled interrupts */
   while(*s) {
      uart_putchar(*s);
      s++;
   }
}

struct raw_data {
   unsigned int len;
   unsigned char flags;
};
union data_packet {
   char buf[sizeof(struct raw_data)];
   struct raw_data data;
} ;

static void transmit(enum trans_source ts) {
   unsigned int tmpTar = TAR;
   unsigned char c,d,flags;

   flags = 0xa0U;
   flags |= (current ? 1 : 0);
   if (ts == FROM_TIMER) {
      flags |= 2;
   }

   __bic_SR_register((unsigned)(GIE));
   c = ((1+tmpTar) & 0xff);
   d = ((tmpTar>>8) & 0xff);
   /* Avoid ascii 17 and 19, flow control */
   if((c & ~2) == (unsigned char) 17 ) {
      flags |= (1U<<3);
      c &= ~1;
   }
   if((d & ~2) == (unsigned char) 17 ) {
      flags |= (1U<<4);
      d &= ~1;
   }
   uart_putchar((char) flags);
   uart_putchar((char) c);
   uart_putchar((char) d);
   __bis_SR_register((unsigned)(GIE));

}

__attribute__((interrupt(TIMER0_A0_VECTOR))) void TIMERA_CCR0_ISR(void) {
   transmit(FROM_TIMER);
}


int main(void)
{
   /* Clock init */

#if 0
   WDTCTL = (unsigned int) WDT_MDLY_0_064;
   IE1 |= WDTIE;
#else
   WDTCTL = (unsigned int) (WDTHOLD + WDTPW);
#endif

   DCOCTL = 0;
#define SMCLK_FREQ 16000000U
   BCSCTL1 = CALBC1_16MHZ;
   DCOCTL = CALDCO_16MHZ;

   /* Pin init */
   P1SEL = TXD ;
   P1SEL2 = TXD ;
   P1DIR = TXLED + TXD + ERRLED; /* XXX: CAPT is set to zero */

   /* Uart init */
#define BAUD_RATE  115200U
/* Bit rate divisor */
#define BRD ((SMCLK_FREQ + (BAUD_RATE >> 1)) / BAUD_RATE)
   UCA0CTL1 = (unsigned) UCSWRST; /* Hold USCI in reset to allow configuration*/
   UCA0CTL0 = 0; /* No parity, LSB first, 8 bits, one stop bit, UART (async)*/
   UCA0BR1 = (BRD >> 12) & 0xFF; /* High byte of whole divisor*/
   UCA0BR0 = (BRD>> 4) & 0xFF; /* Low byte of whole divisor*/
   UCA0MCTL = ((BRD << 4) & 0xF0) | UCOS16; /*Frac divisor, oversampling mode*/
   UCA0CTL1 = (unsigned) UCSSEL_2; /* Use SMCLK, release reset*/
   UC0IE |= UCA0TXIE;

   /* Capture init */

   current = 0; /* Assume low */
#if 0
   P1IES = 0; /* 0: interrupt on 0->1, 1: on 1->0 */
   P1IE |= CAPT;
   P1IFG &= ~CAPT;
#endif

#if 0
   /* Set pullup on capture pin */
   P1REN = CAPT;
   P1OUT &= CAPT;
#endif

   /* Timer init */
   TACCR0 =  0xffffU; /* Have some margin to fire transmit. */
   TACCTL0 = (unsigned int) CCIE; /* CCR0 interrupt enabled */
   TACTL = (unsigned int) (TACLR+TASSEL_2+MC_1+ID_0+ID_1);/*clear,SMCLK/8,up*/

   uart_puts("<STARTING>");

   for(;;) {
      unsigned char tmp;
      __bis_SR_register((unsigned int) (GIE));
      tmp = P1IN & CAPT;
      if(tmp != current) {
         P1OUT ^= ERRLED;
         current = tmp;
         transmit(FROM_EDGE);
      }
   }
}

