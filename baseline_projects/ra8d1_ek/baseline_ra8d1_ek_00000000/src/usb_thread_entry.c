/*
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**********************************************************************************************************************
 * File Name    : usb_thread_entry.c
 * Version      : .
 * Description  : The USB thread operations.
 *********************************************************************************************************************/
/***************************************************************************************************************************
 * Includes   <System Includes> , "Project Includes"
 ***************************************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <ainavi_usb_data_frame.h>
#include <usb_thread.h>
#include "application_config.h"
#include "common_util.h"
#include "time_counter.h"
#if BACKEND == CPU
#include "ai_application/mera/compute_sub_0000.h"
#elif BACKEND == ETHOS
#include "ai_application/mera/model.h"
#endif
/***************************************************************************************************************************
 * Macro definitions
 ***************************************************************************************************************************/
#define DISPLAY_THREAD_YIELD    (20U)

/***************************************************************************************************************************
 * Typedef definitions
 ***************************************************************************************************************************/
/***************************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***************************************************************************************************************************/
//===== This area will be modified by Python ====
// Extern Variables
extern int8 input_0[36864];
extern int8 output_0[648];
extern int8 output_1[2592];
//===============================================
extern uint8_t g_apl_device[];
extern uint8_t g_apl_configuration[];
extern uint8_t g_apl_hs_configuration[];
extern uint8_t g_apl_qualifier_descriptor[];
extern uint8_t *g_apl_string_table[];

#define NUM_STRING_DESCRIPTOR               (7U)
const usb_descriptor_t g_usb_descriptor =
{ g_apl_device, /* Pointer to the device descriptor */
  g_apl_configuration, /* Pointer to the configuration descriptor for Full-speed */
  g_apl_hs_configuration, /* Pointer to the configuration descriptor for Hi-speed */
  g_apl_qualifier_descriptor, /* Pointer to the qualifier descriptor */
  g_apl_string_table, /* Pointer to the string descriptor table */
  NUM_STRING_DESCRIPTOR };

/***************************************************************************************************************************
 * Private global variables and functions
 ***************************************************************************************************************************/
header_frame_t header;
volatile uint8_t first_time_flag = 1;
volatile uint8_t g_inference_flag = 0;
volatile uint8_t g_results_flag = 0;
volatile uint64_t debug_id_pc = 0;
uint64_t debug_id = 1704067200123; // AI Navi defined: DEBUG_ID
/*********************************************************************************************************************
 *  display thread entry function
 *               This thread initializes all the hardware and display the camera input to the mipi lcd.
 *  @param[IN]   void *pvParameters: not used
 *  @retval      None
 ***********************************************************************************************************************/
void usb_thread_entry(void *pvParameters)
{
    fsp_err_t fsp_status = FSP_SUCCESS;
    FSP_PARAMETER_NOT_USED(pvParameters);

    // Initialize modules for console output
    fsp_status = usb_init ();
    vTaskDelay (4000 / portTICK_PERIOD_MS);
    if (FSP_SUCCESS != fsp_status)
    {
        APP_ERROR_TRAP (VISION_AI_APP_ERR_CONSOLE_OPEN);
    }

    // Set the USB initialize done flag
    xEventGroupSetBits(g_ai_app_event, SOFTWARE_USB_INIT_DONE);

    // Initialize timers for measuring each processing time
    TimeCounter_Init ();
    TimeCounter_CountReset ();

    // Wait for all required hardware and software initialization complete
    xEventGroupWaitBits (g_ai_app_event, (HARDWARE_ETHOSU_INIT_DONE | SOFTWARE_AI_INFERENCE_INIT_DONE), pdFALSE, pdTRUE,
                         portMAX_DELAY);
    while (true)
    {
        usb_read_data ((uint8_t*) &header, sizeof(header));
        if (0 == memcmp (header.magic_word, "AIIT", 4))
        {
            switch (header.event)
            {
                case PING:
                    header.event = ACK;
                    usb_write_data ((uint8_t*) &header, sizeof(header));
                break;
                case ACK:
                    if(g_results_flag == 1 && first_time_flag == 1)
                    {
                        g_results_flag = 2;
                        first_time_flag = 0;
                        header.event = LENGHT;
                        usb_write_data ((uint8_t*) &header, sizeof(header));
                        //===== This area will be modified by Python ====
                        // Send number of output
                        uint8_t num_out = 2;
                        uint8_t size = sizeof(int8);
                        uint32_t len[] = { 648, 2592 };
                        usb_write_data((uint8_t*) &num_out, sizeof(num_out));
                        usb_write_data((uint8_t*) &size, sizeof(size));
                        usb_write_data((uint8_t*) len, sizeof(len));
                        //===============================================
                        break;
                    }
                    if (g_results_flag == 2)
                    {
                        g_results_flag = 3;
                        header.event = START_DATA_FRAME;
                        usb_write_data ((uint8_t*) &header, sizeof(header));

                        // Send inference time
                        usb_write_data ((uint8_t*) &application_processing_time.ai_inference_time_ms, sizeof(float));

                        //===== This area will be modified by Python ====
                        // Send model outputs
                        usb_write_data((uint8_t*) output_0, 648);
                        usb_write_data((uint8_t*) output_1, 2592);
                        //===============================================
                        break;
                    }
                    if(g_results_flag == 3){
                        header.event = END_DATA_FRAME;
                        usb_write_data ((uint8_t*) &header, sizeof(header));
                        g_results_flag = 0;
                    }
                break;
                case START_DATA_FRAME:
                    //===== This area will be modified by Python ====
                    // Receive an image for AI inference
                    usb_read_data((uint8_t*) input_0, 36864);
                    //===============================================
                break;
                case END_DATA_FRAME:
                    header.event = ACK;
                    usb_write_data ((uint8_t*) &header, sizeof(header));
                    g_inference_flag = 1;
                break;
                case DEBUG_ID:
                    usb_read_data ((uint8_t*) &debug_id_pc, sizeof(debug_id_pc));
                    if (debug_id_pc == debug_id) {
                        header.event = ACK;
                    } else {
                        header.event = DEBUG_ID; // TODO NACK
                    }
                    usb_write_data ((uint8_t*) &header, sizeof(header));
                break;
                default:
                    break;
            }
            // Clear header buffer
            memset ((uint8_t*) &header, 0, sizeof(header));
        }
        if (g_inference_flag)
        {
#if (BSP_CFG_DCACHE_ENABLED == 1)
            // Clean cache data because this buffer will be accessed by NPU hardware in subsequent process
            //SCB_CleanDCache_by_Addr ((uint8_t*) &model_buffer_int8[0], (int32_t) model_buffer_int8_size);
#endif
            //Set AI inference input image ready flag. AI inference thread may waiting this flag set.
            xEventGroupSetBits (g_ai_app_event, AI_INFERENCE_INPUT_IMAGE_READY);

            // Make a change for immediate task switch
            vTaskDelay (1);

            // Wait for inference
            xEventGroupWaitBits(g_ai_app_event, AI_INFERENCE_RESULT_UPDATED, pdTRUE, pdTRUE, portMAX_DELAY);

            g_inference_flag = 0;

            if (first_time_flag){
                g_results_flag = 1;
            }else{
                g_results_flag = 2;
            }

            // Ping to the PC to prepare for receive model output
            memcpy (header.magic_word, "AIIT", 4);
            header.event = PING;
            usb_write_data ((uint8_t*) &header, sizeof(header));
        }
        vTaskDelay (DISPLAY_THREAD_YIELD);
    }
}
