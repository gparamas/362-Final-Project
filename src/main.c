#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ff.h"
#include "diskio.h"
#include "sdcard.h"
#include <stdio.h>
#include <string.h>
#include "vga_graphics.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "ctf.h"

/*******************************************************************/

#define SD_MISO 16
#define SD_CS 17
#define SD_SCK 18
#define SD_MOSI 19

/*******************************************************************/

void init_spi_sdcard() {
    gpio_set_function(SD_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SD_MOSI, GPIO_FUNC_SPI);
    gpio_init(SD_CS);
    gpio_set_dir(SD_CS, true);

    spi_init(spi0, 400000);
    gpio_put(SD_CS, 1);
}

void disable_sdcard() {
    // fill in.
    
    //uint8_t end = 0xFF;
    //uint8_t received = sdcard_write(0xFF);
    ///printf("disable received: 0x%02x\n", received);
    gpio_put(SD_CS, 1);
    gpio_set_function(SD_MOSI, GPIO_FUNC_SIO);
    gpio_set_dir(SD_MOSI, 1);
    gpio_put(SD_MOSI, 1);
}

void enable_sdcard() {
    // fill in.
    gpio_set_function(SD_MOSI, GPIO_FUNC_SPI);
    gpio_put(SD_CS, 0);
}

void sdcard_io_high_speed() {
    // fill in.
    spi_set_baudrate(spi0, 12000000);
}

void init_sdcard_io() {
    // fill in.
    init_spi_sdcard();
    disable_sdcard();
}

/*******************************************************************/

void init_uart();
void init_uart_irq();
void date(int argc, char *argv[]);
void command_shell();

Player* player1;
Player* player2; 
Flag* flag1;
Flag* flag2;


int main(int argc, char** argv) {
    // Initialize the standard input/output library
    stdio_init_all();
    initVGA();
    init_uart();
    init_uart_irq();
    init_sdcard_io();

    //command_shell();

    mount();
    
    gpio_init(21);
    gpio_init(26);

    //initBackground();
    
    initCTF();
    char buf[768];
    readMap(buf, "map1.vga");
    for(int i = 0; i < 32; i++) {
        for(int j = 0; j < 24; j++) {
            printf("%d ", buf[i * 24 + j]);
        }
        printf("\n");
    }
    dispMap(buf);
    
    unmount();
    for(;;) {
        if(gpio_get(21)) {
            moveRight(player1, 5);
            sleep_ms(50);
        }
        else if(gpio_get(26)) {
            moveLeft(player1, 5);
            sleep_ms(50);
        }
        if(touchingFlag(player1, flag2) && !player1->hasFlag) {
           // printf("a");
            hasFlag(player1, 1);
            showEnd(player1);
        }
    }
}