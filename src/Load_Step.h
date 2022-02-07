#ifndef Load_Step_H__
#define Load_Step_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include "memory.h"
#include "debugfs_ctrl.h"
#include "kern_hijack.h"

// choose timing sources, comment for hardware timing source
#define SOFT_TIMER 2 // 1->pmccntr_el0  2->count-down looping

// #define ATTACK_BIG_CORE 

#ifndef ATTACK_BIG_CORE 
//attack a53
#define AUX_CORE 4
#define VIM_CORE 2
#define L1D_NUM_SETS 128
#define L1D_ASSOCIATIVITY 4
#define L1I_NUM_SETS 256
#define L1I_ASSOCIATIVITY 2
#define L2_NUM_SETS 512
#define L2_ASSOCIATIVITY 16
#define SIZE_CACHELINE 64
#else   
//attack a73
#define AUX_CORE 2
#define VIM_CORE 4
#define L1D_NUM_SETS 64
#define L1D_ASSOCIATIVITY 16
#define L1I_NUM_SETS 256
#define L1I_ASSOCIATIVITY 4
#define L2_NUM_SETS 2048
#define L2_ASSOCIATIVITY 16
#define SIZE_CACHELINE 64
#endif

#define BUF_SIZE (1<<30)

#define L2_PP 0
// other side-channel attacks... 

#define IRQ_LIMIT 100000

// variables in kern_hijack
extern unsigned long load_step_irq_interval;
extern void (*load_step_detect) (unsigned char *);
extern void (*load_step_fill) (unsigned char *);
extern unsigned char* load_step_detect_buffer;
extern unsigned int load_step_side_channel;

// varibales in source file
extern unsigned char* detect_buffer;
extern char* data_pool;
#endif