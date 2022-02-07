#include "pmu_ctrl.h"

void set_pmu_el1(void* data)
{
    unsigned int val32;
    // pmcr_el0.E = 1 & pmcr_el0.P = 1 to reset counter once
    asm volatile("mrs %0, pmcr_el0" : "=r" (val32));
    val32 |= (BIT(0) | BIT(1) | BIT(2));
	isb();
	asm volatile("msr pmcr_el0, %0" : : "r" (val32));

    // enable pmevcntr0_el0 pmevcntr1_el0 pmevcntr2_el0 pmevcntr3_el0 pmevcntr4_el0
    asm volatile("mrs %0, pmcntenset_el0" : "=r" (val32));
	val32 |= (BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(31)); // BIT(31) is to enable cycle counter
	isb();
	asm volatile("msr pmcntenset_el0, %0" : : "r" (val32));

    /*pmxevtyper_el0.u=1 to only count el1   
    L1I_RF->pmevcntr0_el0  
    L1D_RF->pmevcntr1_el0   
    L2_RF->pmevcntr2_el0   
    L2_WB->pmevcntr3_el0
    L1D_WB->pmevcntr4_el0
    */
    asm volatile("msr pmevtyper0_el0, %0" : : "r" ((unsigned int)(0x40000001)));
    asm volatile("msr pmevtyper1_el0, %0" : : "r" ((unsigned int)(0x40000003)));
    asm volatile("msr pmevtyper2_el0, %0" : : "r" ((unsigned int)(0x40000017)));
    asm volatile("msr pmevtyper3_el0, %0" : : "r" ((unsigned int)(0x40000018)));
    asm volatile("msr pmevtyper4_el0, %0" : : "r" ((unsigned int)(0x40000015)));

	// disable user-mode access to event counters.
	asm volatile("msr pmuserenr_el0, %0" : : "r" ((unsigned int)0));

    // pmccfiltr_el0.u = 1 for not counting EL0
    asm volatile("mrs %0, pmccfiltr_el0" : "=r" (val32));
	val32 = BIT(30);
	isb();
	asm volatile("msr pmccfiltr_el0, %0" : : "r" (val32));

    // disable cycle/event counter overflow interrupt
	asm volatile("msr pmintenset_el1, %0" : : "r" ((unsigned long)(0 << 31)));
}

void set_pmcr_el1(void* data)
{
    unsigned int val32;
    // pmcr_el0.E = 1 & pmcr_el0.P = 1 to reset counter once
    asm volatile("mrs %0, pmcr_el0" : "=r" (val32));
    val32 |= (BIT(0) | BIT(2));
	isb();
	asm volatile("msr pmcr_el0, %0" : : "r" (val32));

    // enable cycle counter
    asm volatile("mrs %0, pmcntenset_el0" : "=r" (val32));
	val32 |= BIT(31);
	isb();
	asm volatile("msr pmcntenset_el0, %0" : : "r" (val32));

	// disable user-mode access to event counters.
	asm volatile("msr pmuserenr_el0, %0" : : "r" ((unsigned int)0));

    // pmccfiltr_el0.u = 1 for not counting EL0
    asm volatile("mrs %0, pmccfiltr_el0" : "=r" (val32));
	val32 = BIT(30);
	isb();
	asm volatile("msr pmccfiltr_el0, %0" : : "r" (val32));

    // disable cycle/event counter overflow interrupt
	asm volatile("msr pmintenset_el1, %0" : : "r" ((unsigned long)(0 << 31)));
}