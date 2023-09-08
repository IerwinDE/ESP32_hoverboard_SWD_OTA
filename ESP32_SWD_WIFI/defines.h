/*
   Copyright (c) 2021 Aaron Christophel ATCnetz.de
   SPDX-License-Identifier: GPL-3.0-or-later
*/
#pragma once
#define DNS_NAME "driftKart" //if you change this, remember to update OTA Settings in platformio.ini
#define FLASH_SR 0x4002200C
#define FLASH_CR 0x40022010
#define FLASH_OBR 0x4002201C
#define FLASH_KEYR 0x40022004
#define FLASH_OPTKEYR 0x40022008
#define FLASH_OBP_RDP   0x1ffff800

#define FLASH_KEY1 0x45670123
#define FLASH_KEY2 0xCDEF89AB

#define FLASH_CR_STRT       (1U << 6U)
#define FLASH_CR_OPTER      (1U << 5U)
#define FLASH_CR_OPTPG      (1U << 4U)
#define FLASH_CR_OPTWRE     (1U << 9U)

#define FLASH_OFFSET  0x08000000

#define MAX_RETRIES 10 //maximum number of retries for a single write operation


#define swd_clock_pin_A 21
#define swd_data_pin_A 19

#define swd_clock_pin_B 23 
#define swd_data_pin_B 22

#define CORTEXM_AIRCR (CORTEXM_SCS_BASE + 0xd0cU)
#define CORTEXM_PPB_BASE 0xe0000000U
#define CORTEXM_SCS_BASE (CORTEXM_PPB_BASE + 0xe000U)
#define CORTEXM_AIRCR_VECTKEY (0x05faU << 16U)
#define CORTEXM_AIRCR_SYSRESETREQ   (1U << 2U)
