/*
 * ainavi_usb_data_frame.h
 *
 *  Created on: Jun 21, 2025
 *      Author: rvc
 */

#ifndef AINAVI_USB_DATA_FRAME_H_
#define AINAVI_USB_DATA_FRAME_H_

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "hal_data.h"
#include "common_util.h"
#include "stdio.h"
#include "stdint.h"

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define BUFFER_LINE_LENGTH (1024)

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
typedef enum{
    PING = 0,
    ACK,
    START_DATA_FRAME,
    END_DATA_FRAME,
    DEBUG_ID,
    LENGHT
} event_t;

typedef struct __attribute__((packed)){
    uint8_t magic_word[4];
    event_t event;
}header_frame_t;

/**********************************************************************************************************************
 * Exported global variables
 **********************************************************************************************************************/
extern char sprintf_buffer[];

fsp_err_t usb_init (void);
fsp_err_t usb_read_data (uint8_t* buf, uint32_t len);
fsp_err_t usb_write_data (uint8_t* buf, uint32_t len);

#endif /* AINAVI_USB_DATA_FRAME_H_ */
