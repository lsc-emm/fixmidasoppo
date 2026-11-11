#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/version.h>

static struct kretprobe midas_krp;

/* 返回处理：将 NULL 替换为 init_task */
static int midas_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct task_struct *ret = (struct task_struct *)regs_return_value(regs);
    
    /* GKI 5.10 的 init_task 是导出符号，模块可直接引用 */
    if (ret == NULL) {
        regs_set_return_value(regs, (unsigned long)&init_task);
    }
    return 0;
}

static int __init midas_fix_init(void)
{
    midas_krp.kp.symbol_name = "find_task_by_vpid";
    midas_krp.handler = midas_ret_handler;
    midas_krp.maxactive = 20;

    if (register_kretprobe(&midas_krp) < 0) {
        pr_err("midas_fix: register_kretprobe failed\n");
        return -EINVAL;
    }
    pr_info("midas_fix: kretprobe on find_task_by_vpid registered\n");
    return 0;
}

static void __exit midas_fix_exit(void)
{
    unregister_kretprobe(&midas_krp);
    pr_info("midas_fix: unloaded\n");
}

module_init(midas_fix_init);
module_exit(midas_fix_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Workaround oplus_bsp_midas NULL deref via kretprobe");
