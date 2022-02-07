#include "debugfs_ctrl.h"
#include "Load_Step.h"
#include "pmu_ctrl.h"
#include "side_channel_attacks.h"

struct dentry *dir = NULL;
struct debugfs_blob_wrapper blob;

int set_side_channel(void *data, u64 val)
{
    load_step_side_channel = val;
    printk("set side channel to %d.\n", load_step_side_channel);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(debugfs_set_side_channel, NULL, set_side_channel, "%llu\n");

int set_enable(void *data, u64 val)
{
    load_step_detect_buffer = detect_buffer;
    load_step_irq_interval = val;
    printk("set irq interval to %d.\n", load_step_irq_interval);

    switch(load_step_side_channel)
    {
        case L2_PP:
            load_step_detect=L2_prime_probe_detect;
            load_step_fill=L2_prime_probe_fill;
            break;
        // other side-channel attacks... 
        default:
            break;
    }

    // configure performance counters
    smp_call_function_single(VIM_CORE, set_pmu_el1, NULL, true);
    smp_call_function_single(AUX_CORE, set_pmcr_el1, NULL, true);

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(debugfs_set_enable, NULL, set_enable, "%llu\n");

int set_disable(void *data, u64 val)
{
    printk("set_disable\n");

    int i;

    for(i=(load_step_detect_buffer - detect_buffer); i<BUF_SIZE; i++)
	{
		detect_buffer[i] = 0;
	}

    load_step_irq_interval = 0;
    load_step_detect = NULL;
    load_step_fill = NULL;

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(debugfs_set_disable, NULL, set_disable, "%llu\n");

int DebugFS_Init(void)
{
    struct dentry *junk;
    dir = debugfs_create_dir("Load_Step", 0);
    if (!dir) {
        printk( "debugfs: failed to create /sys/kernel/debug/Load_Step\n");
        return 0;
    }

    blob.data = (void *) detect_buffer;
    blob.size = (unsigned long) (BUF_SIZE * sizeof(unsigned char));

    junk = debugfs_create_blob("data", 0644, dir, &blob);
    if (!junk) {
        // Abort module load.
        printk( "debugfs: failed to create /sys/kernel/debug/Load_Step/data\n");
        return 0;
    }

    junk = debugfs_create_file("set_side_channel",0666, dir, NULL, &debugfs_set_side_channel);
    if (!junk) {
        // Abort module load.
        printk( "debugfs: failed to create /sys/kernel/debug/Load_Step/debugfs_set_side_channel\n");
        return 0;
    }

    junk = debugfs_create_file("set_enable",0666, dir, NULL, &debugfs_set_enable);
    if (!junk) {
        // Abort module load.
        printk( "debugfs: failed to create /sys/kernel/debug/Load_Step/debugfs_set_enable\n");
        return 0;
    }

    junk = debugfs_create_file("set_disable",0666, dir, NULL, &debugfs_set_disable);
    if (!junk) {
        // Abort module load.
        printk( "debugfs: failed to create /sys/kernel/debug/Load_Step/debugfs_set_disable\n");
        return 0;
    }

    return 1;
}

void DebugFS_Free(void)
{
    debugfs_remove_recursive(dir);
}