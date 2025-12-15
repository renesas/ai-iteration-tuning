/***********************************************************************************************************************
* Copyright (c) 2023 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
/**********************************************************************************************************************
 * File Name    : common_utils.h
 * Description  : Contains macros, data structures and functions used  common to the EP
 *********************************************************************************************************************/

/* generic headers */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hal_data.h"

#ifndef COMMON_UTILS_H_
#define COMMON_UTILS_H_

/* SEGGER RTT and error related headers */

#define BIT_SHIFT_8  (8u)
#define SIZE_64      (64u)

#define LVL_ERR      (1u)       /* error conditions   */

#define RESET_VALUE             (0x00)

#define BUFFER_LINE_LENGTH (1024)


//#define EP_SCREEN_MODE d2_mode_argb8888
#define EP_SCREEN_MODE d2_mode_rgb565

#endif /* COMMON_UTILS_H_ */
