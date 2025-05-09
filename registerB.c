#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include "registerB.h"

// Protótipo correto da função
void toggle(void); void analog(volatile uint8_t, volatile int); void status(void);
void uart_init(void); void uart_transmit(uint8_t); void uart_print(const char*);
volatile uint8_t pwm = 0; volatile int posineg = 1;

// Inicializa a tarefa
operatingSystem tasks[NUM_TASKS] = {
    { .funcSp = toggle, .interval_ms = 0x1F4, .counter = 0x00, .ok = false, .padding = 
        {
            0b00000000, 0b00000000, 0b00000000
        } 
    }
    ,
    { .funcCp = analog, .interval_ms = 0x1F4, .counter = 0x00, .ok = false, .padding =
        {
            0b00000000, 0b00000000, 0b00000000
        }
    }
    ,
    { .funcSp = status, .interval_ms = 0x1F4, .counter = 0x00, .ok = false, .padding =
        {
            0b00000000, 0b00000000, 0b00000000 
        }
    }
};

void setup(void) {
    // Configura PB5 como saída
    DDRB |=   ((0x01 << PB5) | (0x01 << PB0)); PORTB_REG.pb5 = 0b0; PORTB_REG.pb0 = 0b0;
    uart_init();
    
    // Configura Timer1 em modo CTC com prescaler 64
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (0 << CS12) |(1 << CS11) | (1 << CS10); // 64x prescaler, modo CTC
    OCR1A = 250; // 16MHz / 64 = 250kHz → 250 ciclos = 1 ms
    TIMSK1 |= (1 << OCIE1A); // Habilita interrupção por comparação A

    // --- Configura Timer2 para Fast PWM no pino OC2B ---
    TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20); // Fast PWM, não-invertido
    TCCR2B = (1 << CS21); /* Prescaler = 8 */ OCR2B = pwm;

}

void loop(void) {
    for (int i = 0; i < NUM_TASKS; ++i) {
        if (tasks[i].ok) {
            if      (i == 0x00) tasks[i].funcSp();
            else if (i == 0x01) tasks[i].funcCp(pwm, posineg);
            else                tasks[i].funcSp();
            tasks[i].ok = false;
        }
    }
}

// Interrupção de Timer1 a cada 1ms
ISR(TIMER1_COMPA_vect) {
    for (int i = 0; i < NUM_TASKS; ++i) {
        
        tasks[i].counter++;
        
        if (tasks[i].counter >= tasks[i].interval_ms) { tasks[i].counter = 0x00; tasks[i].ok = true; }
    }
}

void uart_init(void) {       /* Parte alta do baud rate | Parte baixa do baud rate  */
    uint16_t ubrr = 103; UBRR0H = (ubrr >> 0x08); UBRR0L = ubrr; 
    /* Fórmula: UBRR = (F_CPU / (16 * BAUD)) - 1 */
    UCSR0A = 0; // (opcional) não usar modo duplo
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8 bits de dados, 1 stop bit, sem paridade
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);   // Habilita recepção e transmissão
}

void uart_transmit(uint8_t data) {
  while (!(UCSR0A & (1 << UDRE0))); // Espera até buffer estar vazio
  UDR0 = data;                      // Escreve o dado
}

void uart_print(const char* str) { while (*str) { uart_transmit(*str++); } }

// Alterna o estado de PB5 (LED)
void toggle(void) { PORTB_REG.pb5 ^= 0x01; }
void analog(volatile uint8_t i, volatile int y) { i += y; if (i == 0xff || i == 0x00) { y = -y; } OCR2B = i; }
void status(void) { uart_print("Sistema operacional Cooperativo atuando a cada 1s.\n"); }
