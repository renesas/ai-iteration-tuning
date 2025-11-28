/*
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/**********************************************************************************************************************
 * File Name    : common_utils.h
 * Version      : .
 * Description  : .
 *********************************************************************************************************************/
#ifndef COMMON_UTIL_H__
#define COMMON_UTIL_H__

#include "hal_data.h"
#include "application_config.h"

/* sync events */
#define HARDWARE_ETHOSU_INIT_DONE       (1 << 0)
#define SOFTWARE_AI_INFERENCE_INIT_DONE (1 << 1)
#define SOFTWARE_USB_INIT_DONE          (1 << 2)
#define AI_INFERENCE_INPUT_IMAGE_READY  (1 << 3)
#define AI_INFERENCE_RESULT_UPDATED     (1 << 4)

#define APP_ERROR_TRAP(err)        if(err) { __asm("BKPT #0\n");} /* system execution breaks  */

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/** Common error codes */
typedef enum e_vision_ai_app_err
{
    VISION_AI_APP_SUCCESS                = 0,
    VISION_AI_APP_ERR_AI_INIT            ,  ///< AI init failed
    VISION_AI_APP_ERR_CONSOLE_OPEN       ,
    VISION_AI_APP_ERR_CONSOLE_WRITE      ,
    VISION_AI_APP_ERR_CONSOLE_READ       ,
} vision_ai_app_err_t;

/** process_time report */
typedef struct st_processing_time_info_t
{
    uint32_t ai_inference_pre_processing_time_ms;   ///< Pre processing time for AI inference
    float ai_inference_time_ms;                  ///< AI inference processing time
} processinf_time_info_t;

/**********************************************************************************************************************
 * Exported global variables
 **********************************************************************************************************************/

extern char sprintf_buffer[];
extern processinf_time_info_t application_processing_time;

#endif /* COMMON_UTIL_H__ */
