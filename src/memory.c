#include "memory.h"
#include "Load_Step.h"

int Data_Pool_Init(void)
{    
	data_pool = NULL;
    data_pool = kmalloc((4<<20), GFP_ATOMIC);
	if (!data_pool)
	{
		printk("[!] Error: data_pool Allocation\n");
		return 0;
	}

	detect_buffer = NULL;
    detect_buffer = vmalloc(BUF_SIZE);
	if (!detect_buffer)
	{
		printk("[!] Error: detect_buffer Allocation\n");
		return 0;
	}

    return 1;
}

void Data_Pool_Free(void)
{
	vfree(detect_buffer);
    kfree(data_pool);
}