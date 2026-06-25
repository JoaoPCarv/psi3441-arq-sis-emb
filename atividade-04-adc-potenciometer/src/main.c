#include <stdint.h>

/*
 * Atividade 04 - ADC na placa KL25Z
 *
 * Entrada analógica:
 * A0 da placa FRDM-KL25Z | PTB0 | ADC0_SE8
 *
 * Saídas:
 * LED verde -> PTB19
 * LED azul  -> PTD1
 *
 * LEDs da placa são ativo-baixo:
 * escrever 0 acende (clear)
 * escrever 1 apaga (set)
 */

/* ---------------- Registradores SIM ---------------- */

// Clock-gating para PTB e PTD
#define SIM_SCGC5   (*((volatile uint32_t *)0x40048038))
// Clock-gating para o módulo ADC0
#define SIM_SCGC6   (*((volatile uint32_t *)0x4004803C))

#define SIM_SCGC5_PORTB   (1 << 10)
#define SIM_SCGC5_PORTD   (1 << 12)
#define SIM_SCGC6_ADC0    (1 << 27)

/* ---------------- Registradores PORT ---------------- */

// PTB0: entrada analógica
#define PORTB_PCR0    (*((volatile uint32_t *)0x4004A000))  // PTB0 / ADC0_SE8
// PTB19: LED verde
#define PORTB_PCR19   (*((volatile uint32_t *)0x4004A04C))  // LED verde
// PTD1: LED azul
#define PORTD_PCR1    (*((volatile uint32_t *)0x4004C004))  // LED azul

// Máscaras para configurar o MUX dos pinos
#define MUX_ANALOG    (0 << 8)
#define MUX_GPIO      (1 << 8)

/* ---------------- Registradores GPIO ---------------- */

#define GPIOB_PSOR    (*((volatile uint32_t *)0x400FF044))
#define GPIOB_PCOR    (*((volatile uint32_t *)0x400FF048))
#define GPIOB_PDDR    (*((volatile uint32_t *)0x400FF054))

#define GPIOD_PSOR    (*((volatile uint32_t *)0x400FF0C4))
#define GPIOD_PCOR    (*((volatile uint32_t *)0x400FF0C8))
#define GPIOD_PDDR    (*((volatile uint32_t *)0x400FF0D4))

#define LED_GREEN_PIN   (1 << 19)
#define LED_BLUE_PIN    (1 << 1)

/* ---------------- Registradores ADC0 ---------------- */

// ADC0 Status and Control Register 1 - para selecionar canal e iniciar conversão
#define ADC0_SC1A    (*((volatile uint32_t *)0x4003B000))
// ADC0 Configuration Register 1 - para configurar clock e resolução
#define ADC0_CFG1    (*((volatile uint32_t *)0x4003B008))
// ADC0 Data Result Register - para ler o resultado da conversão
#define ADC0_RA      (*((volatile uint32_t *)0x4003B010))
// ADC0 Status and Control Register 2 - para escolher trigger por software
#define ADC0_SC2     (*((volatile uint32_t *)0x4003B020))

/* Máscaras do ADC */
#define ADC_SC1_COCO      (1 << 7)
// Com 'Single-ended' (DIFF = 0) setado
#define ADC_CHANNEL_SE8   8

/*
 * Resolução de 12 bits:
 * valor mínimo = 0
 * valor máximo = 4095
 *
 * Faixas escolhidas:
 * próximo de 0 V   -> abaixo de aproximadamente 10%
 * próximo de 3.3 V -> acima de aproximadamente 90%
 */
#define ADC_LOW_LIMIT     400
#define ADC_HIGH_LIMIT    3700

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

void leds_off(void)
{
    GPIOB_PSOR = LED_GREEN_PIN;  // apaga verde
    GPIOD_PSOR = LED_BLUE_PIN;   // apaga azul
}

void green_on(void)
{
    GPIOB_PCOR = LED_GREEN_PIN;  // acende verde
}

void blue_on(void)
{
    GPIOD_PCOR = LED_BLUE_PIN;   // acende azul
}

uint16_t adc_read_se8(void)
{
    /*
     * Escrever no ADC0_SC1A seleciona o canal e inicia a conversão.
     * AIEN = 0: sem interrupção
     * DIFF = 0: single-ended
     * ADCH = 8: canal ADC0_SE8
     */
    ADC0_SC1A = ADC_CHANNEL_SE8;

    /*
     * Espera o fim da conversão.
     * COCO = 1 indica conversão completa.
     */
    while ((ADC0_SC1A & ADC_SC1_COCO) == 0) {
        /* espera a conversão (Conversion not completed)*/
    }

    /*
     * Ler ADC0_RA retorna o resultado e limpa o flag COCO.
     */
    return (uint16_t)ADC0_RA;
}

int main(void)
{
    uint16_t adc_value;

    /*
     * 1) Habilitar clock das portas usadas.
     * PORTB: PTB0 entrada ADC e PTB19 LED verde.
     * PORTD: PTD1 LED azul.
     */
    SIM_SCGC5 |= SIM_SCGC5_PORTB;
    SIM_SCGC5 |= SIM_SCGC5_PORTD;

    /*
     * 2) Configurar PTB0 como entrada analógica.
     * Para função analógica, o MUX fica em 000.
     */
    PORTB_PCR0 = MUX_ANALOG;

    /*
     * 3) Configurar os pinos dos LEDs como GPIO.
     */
    PORTB_PCR19 = MUX_GPIO;
    PORTD_PCR1 = MUX_GPIO;

    /*
     * 4) Configurar LEDs como saída.
     */
    GPIOB_PDDR |= LED_GREEN_PIN;
    GPIOD_PDDR |= LED_BLUE_PIN;

    /*
     * 5) Inicialmente, apagar os LEDs.
     */
    leds_off();

    /*
     * 6) Habilitar clock do módulo ADC0.
     */
    SIM_SCGC6 |= SIM_SCGC6_ADC0;

    /*
     * 7) Configurar ADC0.
     *
     * ADC0_SC2:
     * ADTRG = 0 -> trigger por software.
     *
     * ADC0_CFG1: 0000100
     * ADICLK = 00 -> bus clock
     * MODE   = 01 -> conversão single-ended de 12 bits
     * ADLSMP = 0  -> sample time curto
     * ADIV   = 00 -> divisor 1
     */
    ADC0_SC2 = 0x00;
    ADC0_CFG1 = (1 << 2);

    while (1) {
        adc_value = adc_read_se8();

        leds_off();

        if (adc_value >= ADC_HIGH_LIMIT) {
            /*
             * Tensão próxima de 3.3 V.
             */
            blue_on();
        }
        else if (adc_value <= ADC_LOW_LIMIT) {
            /*
             * Tensão próxima de 0 V.
             */
            green_on();
        }

        delayMs(50);
    }

    return 0;
}