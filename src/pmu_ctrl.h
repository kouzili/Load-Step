#ifndef Pmu_ctrl_H__
#define Pmu_ctrl_H__

#include <linux/kernel.h>    

/*
    configured performance counters:
    L1I_RF  pmevcntr0_el0
    1D_RF   pmevcntr1_el0
    L2_RF   pmevcntr2_el0
    L2_WB   pmevcntr3_el0
    L1D_WB->pmevcntr4_el0
*/
void set_pmu_el1(void* data);

void set_pmcr_el1(void* data);
#endif