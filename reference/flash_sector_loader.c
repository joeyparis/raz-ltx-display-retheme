#include <stdint.h>

#define SPI2_SR (*(volatile uint32_t *)0x40014408u)
#define SPI2_DR (*(volatile uint16_t *)0x4001440cu)
#define GPIOA_BSRR (*(volatile uint32_t *)0x40010818u)
#define GPIOA_BRR (*(volatile uint32_t *)0x40010828u)
#define IWDG_KR (*(volatile uint32_t *)0x40003000u)

static void feed_watchdog(void) { IWDG_KR = 0xaaaau; }
static void select_flash(void) { GPIOA_BRR = 0x100u; }
static void release_flash(void) { GPIOA_BSRR = 0x100u; }

static int transfer(uint8_t output) {
    uint32_t timeout = 100000u;
    while ((SPI2_SR & 2u) == 0u) {
        if (--timeout == 0u) return -1;
    }
    SPI2_DR = output;
    timeout = 100000u;
    while ((SPI2_SR & 1u) == 0u) {
        if (--timeout == 0u) return -1;
    }
    feed_watchdog();
    return (int)(SPI2_DR & 0xffu);
}

static int command(uint8_t opcode, uint32_t address, const uint8_t *data, uint32_t count) {
    int result = 0;
    select_flash();
    if (transfer(opcode) < 0) result = -1;
    if (result == 0 && address != 0xffffffffu) {
        if (transfer((uint8_t)(address >> 16)) < 0 ||
            transfer((uint8_t)(address >> 8)) < 0 ||
            transfer((uint8_t)address) < 0) result = -1;
    }
    for (uint32_t i = 0; result == 0 && i < count; ++i) {
        if (transfer(data[i]) < 0) result = -1;
    }
    release_flash();
    return result;
}

static int status(void) {
    select_flash();
    int result = transfer(0x05u);
    if (result >= 0) result = transfer(0xffu);
    release_flash();
    return result;
}

static int ready(uint32_t limit) {
    for (uint32_t i = 0; i < limit; ++i) {
        int value = status();
        if (value < 0) return -1;
        if ((value & 1) == 0) return value;
    }
    return -1;
}

int flash_sector_loader(uint32_t sector, const uint8_t *data) {
    if ((sector & 0xfffu) != 0u || sector >= 0x400000u ||
        data != (const uint8_t *)0x20001000u) return 1;
    int initial = ready(100000u);
    if (initial != 0) return 2;
    if (command(0x06u, 0xffffffffu, data, 0u) < 0) return 3;
    int enabled = status();
    if (enabled < 0 || (enabled & 2) == 0) return 4;
    if (command(0x20u, sector, data, 0u) < 0) return 5;
    if (ready(100000u) < 0) return 6;
    for (uint32_t page = 0; page < 16u; ++page) {
        if (command(0x06u, 0xffffffffu, data, 0u) < 0) return 7;
        enabled = status();
        if (enabled < 0 || (enabled & 2) == 0) return 8;
        if (command(0x02u, sector + page * 256u, data + page * 256u, 256u) < 0) return 9;
        if (ready(100000u) < 0) return 10;
    }
    if (command(0x04u, 0xffffffffu, data, 0u) < 0) return 11;
    return 0;
}
