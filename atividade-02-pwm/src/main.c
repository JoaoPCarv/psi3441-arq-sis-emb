#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <stdlib.h>
#include <pwm_z42.h>
// Define o valor do registrador MOD do TPM para configurar o período do PWM
#define TPM_MODULE 1000
// Tamanho do buffer para a leitura UART. 
#define BUFFER_SIZE 8
static const struct device *uart_dev =
    DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
static uint16_t set_pwm_bright(int bright)
{
    if (bright < 0) {
        bright = 0;
    }
    if (bright > 100) {
        bright = 100;
    }
    /*
    Ativo baixo:
     * brilho 0%   -> PWM = 1000 -> apagado
     * brilho 100% -> PWM = 0    -> máximo
     */
    return TPM_MODULE - ((bright * TPM_MODULE) / 100);
}
static void set_orange(int intensity)
{
    int red;
    int green;
    /*
     * Laranja = 100% vermelho + 35% verde + 0% azul.
     * A intensidade digitada controla o brilho geral.
     */
    red = intensity;
    green = (intensity * 35) / 100;
    pwm_tpm_CnV(TPM2, 0, set_pwm_bright(red));
    pwm_tpm_CnV(TPM2, 1, set_pwm_bright(green));
}

int main(void)
{

    if (!device_is_ready(uart_dev)) {
        printk("UART device is not ready.\r\n");
        return 0;
    }

    // inicialização do TPM2 para gerar PWM
    pwm_tpm_Init(TPM2, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);

    /*
     * Vermelho: PTB18 -> TPM2 canal 0
     * Verde:    PTB19 -> TPM2 canal 1
     */
    pwm_tpm_Ch_Init(TPM2, 0, TPM_PWM_H, GPIOB, 18);
    pwm_tpm_Ch_Init(TPM2, 1, TPM_PWM_H, GPIOB, 19);
    set_orange(0);

    // Buffer de dados digitados (UART)
    char buffer[BUFFER_SIZE];
    // Índice de posição no buffer
    int i = 0;
    // Caractere lido da UART
    unsigned char c;
    // Intensidade do LED laranja (0 a 100)
    int intensity;
    // Inicializa o buffer com zeros
    memset(buffer, 0, sizeof(buffer));

    printk("\r\nControle do LED laranja por PWM\r\n");
    printk("Digite a intensidade de 0 a 100: ");

    while (1) {
        k_msleep(100);
        if (uart_poll_in(uart_dev, &c) == 0) {

            /*
             * Se recebeu um número, guarda no buffer.
             */
            if (c >= '0' && c <= '9') {
                if (i < BUFFER_SIZE - 1) {
                    buffer[i] = c;
                    i++;
                    printk("%c", c);
                }
            }

            /*
             * Se recebeu Enter, processa o número.
             */
            else if (c == '\r' || c == '\n') {

                /*
                 * Se i == 0, significa Enter vazio.
                 * Isso também ignora o segundo caractere de um Enter tipo \r\n.
                 */
                if (i > 0) {
                    buffer[i] = '\0';

                    intensity = atoi(buffer);

                    if (intensity < 0) {
                        intensity = 0;
                    }

                    if (intensity > 100) {
                        intensity = 100;
                    }

                    set_orange(intensity);
                    printk("\r\nIntensidade ajustada para %d%%\r\n", intensity);

                    /*
                    * Limpa o buffer e reinicia o índice.
                     */
                    memset(buffer, 0, sizeof(buffer));
                    i = 0;
                    printk("Digite a intensidade de 0 a 100: ");
                }
            }

            /*
             * Backspace
             */
            else if (c == 8 || c == 127) {
                if (i > 0) {
                    i--;
                    buffer[i] = '\0';
                    printk("\b \b");
                }
            }
        }

        k_msleep(50);
    }

    return 0;
}