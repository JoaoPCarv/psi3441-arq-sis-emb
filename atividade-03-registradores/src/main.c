#include <stdint.h>

// PTB19 é o pino conectado ao LED verde (catodo comum) na placa FRDM-KL25Z 

/* Registradores */
// System Clock Gating Control Register 5 (SCGC5) - habilita o clock para os periféricos
#define SIM_SCGC5     (*((volatile unsigned int *)0x40048038))
// Registrador de configuração (Pin Control Register) do pino PTB19
#define PORTB_PCR19   (*((volatile unsigned int *)0x4004A04C))
// Registrador de saída do GPIOB
#define GPIOB_PDOR    (*((volatile unsigned int *)0x400FF040))
// Registrador de set do GPIOB (coloca em 1)
#define GPIOB_PSOR    (*((volatile unsigned int *)0x400FF044))
// Registrador de clear do GPIOB (coloca em 0)
#define GPIOB_PCOR    (*((volatile unsigned int *)0x400FF048))
// Pino de direção do GPIOB
#define GPIOB_PDDR    (*((volatile unsigned int *)0x400FF054))

/* Máscaras de bits */
// No SCGC5, o bit 10 habilita o clock da porta B
#define SIM_SCGC5_PORTB   (1 << 10)
// Selecionador do pino para o LED verde (PTB19 ou pino 19) no registrador PCR
#define LED_GREEN_PIN     (1 << 19)
// Mux 001: GPIO
#define MUX_GPIO          (1 << 8)

/* Função de espera simples */
void delayMs(int n)
{
    volatile int i;
    volatile int j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < 7000; j++) {
            __asm volatile ("nop");
        }
    }
}

int main(void)
{
    /* 1) Habilitar clock da porta B */
    SIM_SCGC5 |= SIM_SCGC5_PORTB;

    /* 2) Configurar PTB19 como GPIO */
    // Mux 001: GPIO
    PORTB_PCR19 = MUX_GPIO;

    /* 3) Setar direção do pino como saída */
    GPIOB_PDDR |= LED_GREEN_PIN;

    while (1) {
        /* 4) Habilitar saída: acende LED verde (catodo comum) */
        GPIOB_PCOR = LED_GREEN_PIN;

        /* 5) Espera 1 segundo */
        delayMs(1000);

        /* 6) Desabilitar saída: apaga LED verde (catodo comum) */
        GPIOB_PSOR = LED_GREEN_PIN;

        /* 7) Espera 1 segundo */
        delayMs(1000);
    }

    return 0;
}