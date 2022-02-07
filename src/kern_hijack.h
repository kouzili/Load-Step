#ifndef Kern_hijack_H__
#define Kern_hijack_H__

/*
For the best performance and the lowest noise, we actually modify and replace the irq handler fucntions of the Linux kernel and the OPTEE.
*/
#include <linux/kallsyms.h>
#include <linux/interrupt.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/clockchips.h>
#include <linux/irqdesc.h>
#include <linux/tee_drv.h>
#include <linux/arm-smccc.h>

//Needed by arch_timer functions------------------------------------//
enum tick_device_mode {
	TICKDEV_MODE_PERIODIC,
	TICKDEV_MODE_ONESHOT,
};

struct tick_device {
	struct clock_event_device *evtdev;
	enum tick_device_mode mode;
};
//-----------------------------------------------------------------//

//Needed by optee functions----------------------------------------//
#define OPTEE_SMC_RETURN_RPC_PREFIX_MASK	0xFFFF0000
#define OPTEE_SMC_RETURN_RPC_PREFIX		0xFFFF0000
#define OPTEE_SMC_RETURN_UNKNOWN_FUNCTION 0xFFFFFFFF
#define OPTEE_SMC_RETURN_ETHREAD_LIMIT	0x1
#define OPTEE_SMC_RETURN_RPC_FUNC_MASK		0x0000FFFF
#define OPTEE_SMC_RPC_FUNC_FOREIGN_INTR		4
#define OPTEE_SMC_RETURN_IS_RPC(ret)	__optee_smc_return_is_rpc((ret))
#define OPTEE_SMC_RETURN_GET_RPC_FUNC(ret) \
	((ret) & OPTEE_SMC_RETURN_RPC_FUNC_MASK)

static inline bool __optee_smc_return_is_rpc(u32 ret)
{
	return ret != OPTEE_SMC_RETURN_UNKNOWN_FUNCTION &&
	       (ret & OPTEE_SMC_RETURN_RPC_PREFIX_MASK) ==
			OPTEE_SMC_RETURN_RPC_PREFIX;
}

typedef void (optee_invoke_fn)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, struct arm_smccc_res *);

struct optee_call_queue {
	/* Serializes access to this struct */
	struct mutex mutex;
	struct list_head waiters;
};

struct optee_supp {
	/* Serializes access to this struct */
	struct mutex mutex;
	struct tee_context *ctx;

	int req_id;
	struct list_head reqs;
	struct idr idr;
	struct completion reqs_c;
};

struct optee_wait_queue {
	/* Serializes access to this struct */
	struct mutex mu;
	struct list_head db;
};

struct optee {
	struct tee_device *supp_teedev;
	struct tee_device *teedev;
	optee_invoke_fn *invoke_fn;
	struct optee_call_queue call_queue;
	struct optee_wait_queue wait_queue;
	struct optee_supp supp;
	struct tee_shm_pool *pool;
	void *memremaped_shm;
	u32 sec_caps;
};
//-----------------------------------------------------------------//

// functions for init and deinit
void IRQ_Handler_Hijack_Init(void);

void IRQ_Handler_Hijack_Free(void);

void Arch_Timer_Hijack_Init(void);

void Arch_Timer_Hijack_Free(void);

void TEE_Invoke_Hijack_Init(void);

void TEE_Invoke_Hijack_Free(void);

#endif