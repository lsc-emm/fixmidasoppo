#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/pid.h>

static struct kretprobe midas_krp;
static void *init_task_addr = NULL;

static int midas_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct task_struct *ret = (struct task_struct *)regs_return_value(regs);
    if (ret == NULL && init_task_addr != NULL)
        regs_set_return_value(regs, (unsigned long)init_task_addr);
    return 0;
}

static int __init midas_fix_init(void)
{
    struct kprobe kp = { .symbol_name = "init_task" };
    int ret = register_kprobe(&kp);
    if (ret < 0) {
        pr_err("midas_fix: resolve init_task failed: %d\n", ret);
        return ret;
    }
    init_task_addr = (void *)kp.addr;
    unregister_kprobe(&kp);

    midas_krp.kp.symbol_name = "find_task_by_vpid";
    midas_krp.handler = midas_ret_handler;
    midas_krp.maxactive = 20;

    ret = register_kretprobe(&midas_krp);
    if (ret < 0) {
        pr_err("midas_fix: register_kretprobe failed: %d\n", ret);
        return ret;
    }
    pr_info("midas_fix: hooked, init_task=0x%lx\n", (unsigned long)init_task_addr);
    return 0;
}

static void __exit midas_fix_exit(void)
{
    unregister_kretprobe(&midas_krp);
}

module_init(midas_fix_init);
module_exit(midas_fix_exit);
MODULE_LICENSE("GPL");
