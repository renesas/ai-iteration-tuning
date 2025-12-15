/* generated common source file - do not edit */
#include "common_data.h"
ioport_instance_ctrl_t g_ioport_ctrl;
const ioport_instance_t g_ioport =
{ .p_api = &g_ioport_on_ioport, .p_ctrl = &g_ioport_ctrl, .p_cfg = &g_bsp_pin_cfg, };
EventGroupHandle_t g_ai_app_event;
#if 1
StaticEventGroup_t g_ai_app_event_memory;
#endif
void rtos_startup_err_callback(void *p_instance, void *p_data);
void g_common_init(void)
{
    g_ai_app_event =
#if 1
            xEventGroupCreateStatic (&g_ai_app_event_memory);
#else
                xEventGroupCreate();
                #endif
    if (NULL == g_ai_app_event)
    {
        rtos_startup_err_callback (g_ai_app_event, 0);
    }
}
