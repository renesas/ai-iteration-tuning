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
#include <wrapper.h>
#include "common_util.h"
#include "common_data.h"
#include "application_config.h"
#include "time_counter.h"

/***************************************************************************************************************************
 * Macro definitions
 ***************************************************************************************************************************/
#define AI_THREAD_YIELD                     (25)
/***************************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***************************************************************************************************************************/
extern int8_t model_buffer_int8[IMAGE_DATA_SIZE];

/* Inference engine input buffer */
#if (AI_INPUT_IMAGE_ALLOCATION == ALLOCATE_TO_ONCHIP_RAM)
int8_t model_buffer_int8[IMAGE_DATA_SIZE] BSP_ALIGN_VARIABLE(8);
#elif (AI_INPUT_IMAGE_ALLOCATION == ALLOCATE_TO_SDRAM)
int8_t model_buffer_int8[IMAGE_DATA_SIZE] BSP_PLACE_IN_SECTION(".sdram")  BSP_ALIGN_VARIABLE(8);
#else
#error "Add your preferred buffer definition"
#endif
uint32_t model_buffer_int8_size = sizeof(model_buffer_int8);

/***************************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)

 ***************************************************************************************************************************/
processinf_time_info_t application_processing_time;

//===== This area will be modified by Python ====
// Define output tensors
//===============================================

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

    err = RM_ETHOSU_Open(&g_rm_ethosu0_ctrl, &g_rm_ethosu0_cfg);

    if(FSP_SUCCESS != err )
    {
        APP_ERROR_TRAP(VISION_AI_APP_ERR_AI_INIT);
    }

    xEventGroupSetBits(g_ai_app_event, HARDWARE_ETHOSU_INIT_DONE);
    xEventGroupSetBits(g_ai_app_event, SOFTWARE_AI_INFERENCE_INIT_DONE);
    while (true)
    {
        xEventGroupWaitBits(g_ai_app_event, AI_INFERENCE_INPUT_IMAGE_READY, pdTRUE, pdTRUE, portMAX_DELAY);

        memcpy(mera_input_ptr(), model_buffer_int8, IMAGE_DATA_SIZE);

        /* Run inference over this image. */
        volatile uint32_t old_counter =  TimeCounter_CurrentCountGet();
        mera_invoke();
        volatile uint32_t new_counter = TimeCounter_CurrentCountGet();
        application_processing_time.ai_inference_time_ms = TimeCounter_CountValueConvertToMs(old_counter, new_counter);

        //===== This area will be modified by Python ====
        // Get output tensors
        //===============================================

        xEventGroupSetBits(g_ai_app_event, AI_INFERENCE_RESULT_UPDATED);

        vTaskDelay(AI_THREAD_YIELD);
    }
}
