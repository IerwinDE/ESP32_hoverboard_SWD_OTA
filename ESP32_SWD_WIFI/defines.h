/*
   Copyright (c) 2021 Aaron Christophel ATCnetz.de
   SPDX-License-Identifier: GPL-3.0-or-later
*/
#pragma once
#define DNS_NAME "driftKart" //if you change this, remember to update OTA Settings in platformio.ini
#define FLASH_SR 0x4002200C
#define FLASH_CR 0x40022010

#define FLASH_OFFSET  0x08000000

#define MAX_RETRIES 10 //maximum number of retries for a single write operation


#define DUAL_BOARD //comment this line if you only have one connected STM

#define swd_clock_pin_A 21
#define swd_data_pin_A 19

#define swd_clock_pin_B 22 
#define swd_data_pin_B 23
