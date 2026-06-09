#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

static void uart_init(void) {
  UBRR0H = 0;
  UBRR0L = 103;                 // 16 MHz, 9600 bps, U2X0 = 0
  UCSR0A = 0;
  UCSR0B = _BV(TXEN0);          // TX only on PD1/TXD
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
}

static void uart_putc(char c) {
  while (!(UCSR0A & _BV(UDRE0))) {
  }
  UDR0 = c;
}

static void uart_puts(const char *text) {
  while (*text) {
    uart_putc(*text++);
  }
}

int main(void) {
  uart_init();
  uart_puts("BOOT\n");

  while (1) {
    uart_puts("HB\n");
    _delay_ms(100);
  }
}
