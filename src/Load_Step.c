#include "Load_Step.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zili KOU");
MODULE_DESCRIPTION("A precise TrustZone execution control framework");

// gloabal variables
char* data_pool;
unsigned char* detect_buffer;
unsigned char* load_step_detect_buffer;
unsigned long load_step_irq_interval;
unsigned int load_step_side_channel;
void (*load_step_detect) (unsigned char *);
void (*load_step_fill) (unsigned char *);

int __init Load_Step_init(void)
{
    if (!Data_Pool_Init())
    {
        printk("Data_Pool_Init Failed!\n");
    }
	
    if (!DebugFS_Init())
    {
        printk("Pool_Allocate_Initiate Failed!\n");
    }

    // replace the default irq handler function, arch_timer counter, and the optee's irq handler
    IRQ_Handler_Hijack_Init();
#ifndef SOFT_TIMER
    Arch_Timer_Hijack_Init();
#endif
    TEE_Invoke_Hijack_Init();

    printk("Load_Step installed!\n");
    return 0;  
}

void __exit Load_Step_cleanup(void)
{
    TEE_Invoke_Hijack_Free();
#ifndef SOFT_TIMER
    Arch_Timer_Hijack_Free();
#endif
    IRQ_Handler_Hijack_Free();

    DebugFS_Free();
    Data_Pool_Free();
    printk("Load_Step removed!\n");
}

module_init(Load_Step_init);
module_exit(Load_Step_cleanup);
