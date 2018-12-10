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

/** @file FlashDev.c */

#include "FlashOS.H"

#define FLASH_DRV_VERS (0x0100+VERS)   // Driver Version, do not modify!
#define DEVICE_NAME    "RDA5981 1MB Flash"

struct FlashDevice const FlashDevice = {
    FLASH_DRV_VERS,             // Driver Version, do not modify!
    DEVICE_NAME,                // Device Name (128 chars max)
    ONCHIP,                     // Device Type
    0x18001000,                 // Device Start Address
    0x000FF000,                 // Device Size
    0x00001000,                 // Programming Page Size
    0x00000000,                 // Reserved, must be 0
    0xFF,                       // Initial Content of Erased Memory
    0x00000064,                 // Program Page Timeout 100 mSec
    0x00007530,                 // Erase Sector Timeout 30000 mSec
    {{0x00001000, 0x00000000},  // Sector Size {4kB, starting at address 0}
    {SECTOR_END}}
};
