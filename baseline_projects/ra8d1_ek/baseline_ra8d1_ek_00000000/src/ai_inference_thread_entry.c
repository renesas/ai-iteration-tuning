/*
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/**********************************************************************************************************************
 * File Name    : ai_inference_thread_entry.c
 * Description  : This file defines the entry function of the ai thread and activates the inference. It also includes
 *                the image pre processing before inference.
 **********************************************************************************************************************/
#include "ai_inference_thread.h"
#include <stdio.h>
#include "common_util.h"
#include "common_data.h"
#include "application_config.h"
#include "time_counter.h"
#if BACKEND == CPU
#include "ai_application/mera/compute_sub_0000.h"
#elif BACKEND == ETHOS
#include "ai_application/mera/model.h"
#endif
/***************************************************************************************************************************
 * Macro definitions
 ***************************************************************************************************************************/
#define AI_THREAD_YIELD                     (25)
/***************************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***************************************************************************************************************************/
//===== This area will be modified by Python ====
// Variable definitions
uint8_t my_buffer[kBufferSize_sub_0000];
int8 input_0[36864];
int8 output_0[648];
int8 output_1[2592];
//===============================================
/***************************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***************************************************************************************************************************/
processinf_time_info_t application_processing_time;
/*********************************************************************************************************************
 *  AI thread entry function. The image will be processed prior to inference.
 *  The image processing and inference is repeatedly carried out in this thread.
 *  @param[IN]      void *pvParameters, contains TaskHandle_t, not used.
 *  @retval      None
***********************************************************************************************************************/
void ai_inference_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    xEventGroupWaitBits (g_ai_app_event, SOFTWARE_USB_INIT_DONE, pdFALSE, pdTRUE, portMAX_DELAY);

    fsp_err_t err = FSP_SUCCESS;

    if(FSP_SUCCESS != err )
    {
        APP_ERROR_TRAP(VISION_AI_APP_ERR_AI_INIT);
    }

    xEventGroupSetBits(g_ai_app_event, HARDWARE_ETHOSU_INIT_DONE);
    xEventGroupSetBits(g_ai_app_event, SOFTWARE_AI_INFERENCE_INIT_DONE);
    while (true)
    {
        xEventGroupWaitBits(g_ai_app_event, AI_INFERENCE_INPUT_IMAGE_READY, pdTRUE, pdTRUE, portMAX_DELAY);

        /* Run inference over this image. */
        volatile uint32_t old_counter =  TimeCounter_CurrentCountGet();
        //===== This area will be modified by Python ====
        // Run Model
        compute_sub_0000(my_buffer, input_0, output_0, output_1);
        //===============================================
        volatile uint32_t new_counter = TimeCounter_CurrentCountGet();
        application_processing_time.ai_inference_time_ms = TimeCounter_CountValueConvertToMs(old_counter, new_counter);

        xEventGroupSetBits(g_ai_app_event, AI_INFERENCE_RESULT_UPDATED);

        vTaskDelay(AI_THREAD_YIELD);
    }
}
