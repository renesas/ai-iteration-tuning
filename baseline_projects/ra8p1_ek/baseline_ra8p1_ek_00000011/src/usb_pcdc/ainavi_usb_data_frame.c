/*
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/
/**********************************************************************************************************************
 * File Name    : ainavi_usb_data_frame.c
 * Description  : This file defines usb communication data frame.
 **********************************************************************************************************************/
#include "hal_data.h"
#include "common_data.h"
#include <stdio.h>
#include <string.h>
#include <ainavi_usb_data_frame.h>
#include <usb_thread.h>
#include "common_util.h"

/***************************************************************************************************************************
 * Typedef definitions
 ***************************************************************************************************************************/
/***************************************************************************************************************************
 * Imported global variables and functions (from other files)
 ***************************************************************************************************************************/
/***************************************************************************************************************************
 * Exported global variables and functions (to be accessed by other files)
 ***************************************************************************************************************************/
char sprintf_buffer[BUFFER_LINE_LENGTH] = {};
/***************************************************************************************************************************
 * Private global variables and functions
 ***************************************************************************************************************************/

static volatile uint32_t g_transfer_complete;
static volatile uint32_t g_receive_complete;
static volatile uint32_t g_operation_complete;

/*********************************************************************************************************************
 *  Console console callback
 *  @param[IN]   uart_callback_args_t *p_args: callback information
 *  @retval      None
***********************************************************************************************************************/
void console_output_usb_callback(rm_comms_callback_args_t *p_args)
{
    /* Handle the UART event */
    int event = (int)p_args->event;
    switch (event)
    {
        case RM_COMMS_EVENT_OPERATION_COMPLETE:
            g_operation_complete = 1;
            break;
        /* Receive complete */
        case RM_COMMS_EVENT_RX_OPERATION_COMPLETE:
        {
            g_receive_complete = 1;
            break;
        }
        /* Transmit complete */
        case RM_COMMS_EVENT_TX_OPERATION_COMPLETE:
        {
            g_transfer_complete = 1;
            break;
        }
        default:
        {
            /* Do nothing */
        }
    }
}

fsp_err_t usb_init (void)
{
    fsp_err_t fsp_err = FSP_SUCCESS;

    fsp_err = RM_COMMS_USB_PCDC_Open(&g_comms_usb_pcdc0_ctrl, &g_comms_usb_pcdc0_cfg);
    fsp_err = RM_COMMS_USB_PCDC_CallbackSet(&g_comms_usb_pcdc0_ctrl, console_output_usb_callback, NULL);
    return fsp_err;
}

fsp_err_t usb_read_data (uint8_t* buf, uint32_t len)
{
    fsp_err_t fsp_err = FSP_SUCCESS;
    g_receive_complete = 0;
    g_operation_complete = 0;
    fsp_err = RM_COMMS_USB_PCDC_Read(&g_comms_usb_pcdc0_ctrl, buf, len);
    if (FSP_SUCCESS != fsp_err)
    {
        APP_ERROR_TRAP(VISION_AI_APP_ERR_CONSOLE_READ);
    }
    while (!g_receive_complete && !g_operation_complete)
    {
#if (BSP_CFG_RTOS == 2) // FreeRTOS
        vTaskDelay(1);
#endif
    }
    return fsp_err;
}

fsp_err_t usb_write_data (uint8_t* buf, uint32_t len)
{
    fsp_err_t fsp_err = FSP_SUCCESS;
    g_transfer_complete = 0;
    g_operation_complete = 0;
    fsp_err = RM_COMMS_USB_PCDC_Write(&g_comms_usb_pcdc0_ctrl, buf, len);
    if (FSP_SUCCESS != fsp_err)
    {
        APP_ERROR_TRAP(VISION_AI_APP_ERR_CONSOLE_WRITE);
    }
    while (!g_transfer_complete && !g_operation_complete)
    {
#if (BSP_CFG_RTOS == 2) // FreeRTOS
        vTaskDelay(1);
#endif
    }
    return fsp_err;
}

