/*
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/**********************************************************************************************************************
 * File Name    : application_config.h
 * Description  : This file defines the macro of the application configuration.
 **********************************************************************************************************************/
#ifndef APPLICATION_CONFIG_H__
#define APPLICATION_CONFIG_H__

// ###################### MEMORY ALLOCATION ######################
/* Defines for memory allocation options */
#define float32 float
#define int8 int8_t

#define ETHOS                               0
#define CPU                                 1

//===== This area will be modified by Python ====
// Define Backend
#define BACKEND CPU
//===============================================
#define ALLOCATE_TO_ONCHIP_ROM              0
#define ALLOCATE_TO_ONCHIP_RAM              1
#define ALLOCATE_TO_SDRAM                   2 // Buffer will be located in ".sdram"
#define ALLOCATE_TO_SDRAM_INITIAL_IN_OSPI   3 // Buffer will be located in ".sdram_ospi_data.data"
#define ALLOCATE_TO_OSPI                    4 // Buffer will be located in ".ospi_device_1.data"

#define AI_INPUT_IMAGE_ALLOCATION           ALLOCATE_TO_ONCHIP_RAM  /* Option: OnchipRAM or SDRAM */
#define AI_MODEL_ALLOCATION                 ALLOCATE_TO_ONCHIP_ROM  /* Option: OnchipROM, OnchipRAM, SDRAM(IntialOSPI) or OSPI */
#define TENSOR_ARENA_ALLOCATION             ALLOCATE_TO_ONCHIP_RAM  /* Option: OnchipRAM or SDRAM */

// ------------------ Internal auto config ------------------
#if ((AI_MODEL_ALLOCATION) == (ALLOCATE_TO_OSPI))
#define REQUIRE_OSPI_OPEN
#elif ((AI_MODEL_ALLOCATION) == (ALLOCATE_TO_SDRAM_INITIAL_IN_OSPI))
#define REQUIRE_OSPI_OPEN
#define REQUIRE_OSPI_MEMORY_COPY_TO_SDRAM
#endif

#endif /* APPLICATION_CONFIG_H__ */
