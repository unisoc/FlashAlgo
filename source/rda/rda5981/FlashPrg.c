/* Flash OS Routines
 * Copyright (c) 2009-2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** @file FlashPrg.c */

#include "FlashOS.h"
#include "FlashPrg.h"
#include "string.h"

#define FLASH_SIZE 0x100000
#define spi_wip_reset_addr_4 0x1dbb
#define spi_write_reset_addr_4 0x1dcf
#define wait_busy_down_addr_4 0x1db1
#define spi_flash_erase_4KB_sector_addr_4 0x23d3
#define FLASH_WRITE_FUN_ADDR_4 0x2271
#define SPI_FLASH_READ_DATA_FOR_MBED_ADDR_4 0x2037
#define spi_flash_flush_cache_addr_4 0x1efd
//FLASH REG
#define FLASH_CTL_REG_BASE 0x17fff000
#define FLASH_CTL_TX_CMD_ADDR_REG (FLASH_CTL_REG_BASE + 0x00)
#define FLASH_CTL_TX_BLOCK_SIZE_REG (FLASH_CTL_REG_BASE + 0x04)
#define FLAHS_CTL_TX_FIFO_DATA_REG (FLASH_CTL_REG_BASE + 0x08)
#define FLAHS_CTL_RX_FIFO_DATA_REG (FLASH_CTL_REG_BASE + 0x10)
#define FLASH_CTL_FLASH_CONFIG_REG (FLASH_CTL_REG_BASE + 0x14)
#define FLASH_CTL_NAND_CFG1_REG (FLASH_CTL_REG_BASE + 0x24)
#define RDA_APB_GPIO_BASE (0x40001000UL)
#define RDA_APB_GPIO_DATAOUTPUT_REG (RDA_APB_GPIO_BASE + 0x8)
#define RDA_APB_GPIO_WIFI_IOMUX_CFG0_REG (RDA_APB_GPIO_BASE + 0x44)
#define CACHE_REG_BASE (0x40014000UL)
#define CACHE_CFG_REG (CACHE_REG_BASE + 0x00)
#define CACHE_FLUSH_REG (CACHE_REG_BASE + 0x04)
#define CACHE_ADDR_REG (CACHE_REG_BASE + 0x08)
#define FLASH_CTL (0x14000000UL)
#define SCU_BASE (0x40000000UL)
#define HCLK1_FLASH_EN	(1ul<<27)
#define HCLK1_ICACHE_EN (1ul<<28)
#define CMD_64KB_CHIP_ERASE (0x00000060UL)
#define WRITE_REG32(REG, VAL)    ((*(volatile unsigned int*)(REG)) = (unsigned int)(VAL))
#define MREAD_WORD(addr) *((volatile unsigned int *)(addr))
#define MIN(a,b) ((a) < (b) ? (a) : (b))	

static inline void wait_busy_down_4(void)
{
    ((void(*)(void))wait_busy_down_addr_4)();
}

static inline void spi_write_reset_4(void)
{
    ((void(*)(void))spi_write_reset_addr_4)();
}

static inline void spi_wip_reset_4(void)
{
    ((void(*)(void))spi_wip_reset_addr_4)();
}

void spi_init(void)
{
    WRITE_REG32(FLASH_CTL_NAND_CFG1_REG, 0x030f1300);
    WRITE_REG32(RDA_APB_GPIO_WIFI_IOMUX_CFG0_REG, 0x006DB6C0);
}

void spi_flash_cfg_cache(void)
{
    uint32_t tmp;
	
    WRITE_REG32(CACHE_ADDR_REG, FLASH_CTL);
    tmp = MREAD_WORD(CACHE_CFG_REG);
    WRITE_REG32(CACHE_CFG_REG, tmp|0xc0);
    tmp = MREAD_WORD(CACHE_FLUSH_REG);
	
    while((tmp&0x1) == 0x1)
    {
        tmp = MREAD_WORD(CACHE_FLUSH_REG);
    }	
}

void spi_controller_init(void)
{
    uint32_t val = 0;
    volatile uint32_t *addr;
    uint32_t status2, status3, status4, status5;
    //4wire
    *(volatile uint32_t *)FLASH_CTL_FLASH_CONFIG_REG |= 0x01;
    val = ((*(volatile uint32_t *)RDA_APB_GPIO_WIFI_IOMUX_CFG0_REG)&(0xFF03FFFF));
    (*(volatile uint32_t *)RDA_APB_GPIO_WIFI_IOMUX_CFG0_REG) = (val | (0x9<<18));
    (*(volatile uint32_t *)RDA_APB_GPIO_DATAOUTPUT_REG) = ((0x1 << 18) | (0x1 << 19));

    WRITE_REG32(FLASH_CTL_TX_BLOCK_SIZE_REG, 3<<8);
    status3 = MREAD_WORD(FLAHS_CTL_RX_FIFO_DATA_REG);
    status4 = MREAD_WORD(FLAHS_CTL_RX_FIFO_DATA_REG);
    status5 = MREAD_WORD(FLAHS_CTL_RX_FIFO_DATA_REG);
    spi_wip_reset_4();
    wait_busy_down_4();
    WRITE_REG32(FLASH_CTL_TX_CMD_ADDR_REG, 0x9F);
    wait_busy_down_4();
    WRITE_REG32(FLASH_CTL_TX_BLOCK_SIZE_REG, 3<<8);
    wait_busy_down_4();
    status3 = MREAD_WORD(FLAHS_CTL_RX_FIFO_DATA_REG);
    status4 = MREAD_WORD(FLAHS_CTL_RX_FIFO_DATA_REG);
    status5 = MREAD_WORD(FLAHS_CTL_RX_FIFO_DATA_REG);
	
    if((status4&0xFF)==0x40)
    {
        if((status5&0xFF)<=0x15)
        {
            spi_wip_reset_4();
            addr = (volatile uint32_t *)FLASH_CTL_TX_BLOCK_SIZE_REG;
            *addr = 2<<8;
            spi_wip_reset_4();
            wait_busy_down_4();
            addr = (volatile uint32_t *)FLAHS_CTL_TX_FIFO_DATA_REG;
            *addr = (0x00);
            wait_busy_down_4();
            addr = (volatile uint32_t *)FLAHS_CTL_TX_FIFO_DATA_REG;
            *addr = (0x02);
            spi_wip_reset_4();
            spi_write_reset_4();
            wait_busy_down_4();
            WRITE_REG32(FLASH_CTL_TX_CMD_ADDR_REG, 0x01);
            wait_busy_down_4();
        }else{
            spi_wip_reset_4();
            addr = (volatile uint32_t *)FLASH_CTL_TX_BLOCK_SIZE_REG;
            *addr = 1<<8;
            spi_wip_reset_4();
            wait_busy_down_4();
            addr = (volatile uint32_t *)FLAHS_CTL_TX_FIFO_DATA_REG;
            *addr = (0x02);
            spi_wip_reset_4();
            spi_write_reset_4();
            wait_busy_down_4();
            WRITE_REG32(FLASH_CTL_TX_CMD_ADDR_REG, 0x31);
            wait_busy_down_4();
        }
    }
    spi_wip_reset_4();
    wait_busy_down_4();
    WRITE_REG32(FLASH_CTL_TX_CMD_ADDR_REG, 0x35);
    wait_busy_down_4();
    status2=MREAD_WORD(FLAHS_CTL_RX_FIFO_DATA_REG);
    while((status2&(0x02))==0)
    {
        spi_wip_reset_4();
        WRITE_REG32(FLASH_CTL_TX_CMD_ADDR_REG, 0x35);
        wait_busy_down_4();
        status2=MREAD_WORD(FLAHS_CTL_RX_FIFO_DATA_REG);
    }
	
    val = ((*(volatile uint32_t *)RDA_APB_GPIO_WIFI_IOMUX_CFG0_REG)&(0xFF03FFFF));
    (*(volatile uint32_t *)RDA_APB_GPIO_WIFI_IOMUX_CFG0_REG) = (val | (0x1B<<18));
}	

uint32_t Init(uint32_t adr, uint32_t clk, uint32_t fnc)
{
    // Called to configure the SoC. Should enable clocks
    //  watchdogs, peripherals and anything else needed to
    //  access or program memory. Fnc parameter has meaning
    //  but currently isnt used in MSC programming routines

    /* open flash ctl enable */
    uint32_t val = 0;
    *(volatile uint32_t *)SCU_BASE = ((*(volatile uint32_t *)SCU_BASE)|HCLK1_FLASH_EN);
    spi_init();
    *(volatile uint32_t *)SCU_BASE = ((*(volatile uint32_t *)SCU_BASE)|HCLK1_ICACHE_EN);
    spi_flash_cfg_cache();	
    spi_controller_init();
	
    //divider
    val = (*(volatile uint32_t *)FLASH_CTL_FLASH_CONFIG_REG) & (~(0x00FFUL << 8));
    (*(volatile uint32_t *)FLASH_CTL_FLASH_CONFIG_REG) = val | (0x0004UL << 8);
    return 0;
}

uint32_t UnInit(uint32_t fnc)
{
    // When a session is complete this is called to powerdown
    //  communication channels and clocks that were enabled
    //  Fnc parameter has meaning but isnt used in MSC program
    //  routines
    return 0;
}

uint32_t BlankCheck(uint32_t adr, uint32_t sz, uint8_t pat)
{
    // Check that the memory at address adr for length sz is 
    //  empty or the same as pat
    return 0;
}

uint32_t EraseChip(void)
{
    // Execute a sequence that erases the entire of flash memory region
    spi_wip_reset_4();
    spi_write_reset_4();
    WRITE_REG32(FLASH_CTL_TX_CMD_ADDR_REG, CMD_64KB_CHIP_ERASE);
    wait_busy_down_4();
    spi_wip_reset_4();
    return 0;
}

uint32_t EraseSector(uint32_t adr)
{
    // Execute a sequence that erases the sector that adr resides in
    adr &= (FLASH_SIZE -1);
    ((void(*)(uint32_t))spi_flash_erase_4KB_sector_addr_4)(adr);
    return 0;
}

uint32_t ProgramPage(uint32_t adr, uint32_t sz, uint32_t *buf)
{
    // Program the contents of buf starting at adr for length of sz
    adr &= (FLASH_SIZE -1);
    ((void(*)(uint32_t, uint8_t *, uint32_t))FLASH_WRITE_FUN_ADDR_4)(adr, (uint8_t *)buf, sz);
    return 0;
}

uint32_t Verify(uint32_t adr, uint32_t sz, uint32_t *buf)
{
    // Given an adr and sz compare this against the content of buf
    uint32_t func = 0;
    uint8_t verify_buf[16] = {0};
    uint32_t verify_size = MIN(sz, sizeof(verify_buf));
    adr &= (FLASH_SIZE-1);
    func = spi_flash_flush_cache_addr_4;
    ((void(*)(void))func)();
    func = SPI_FLASH_READ_DATA_FOR_MBED_ADDR_4;
    ((void(*)(void *, void *, uint32_t))func)((void *)verify_buf, (void *)adr, verify_size);
		
    if (memcmp(buf, verify_buf, verify_size) != 0) 
    {
        return 1;
    }
    return 0;
}
