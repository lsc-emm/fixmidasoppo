#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>

static struct kprobe midas_kp;

static int midas_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    /*
     * ARM64 的 pt_regs 里，x30(LR) 位于 regs->regs[30]。
     * 把 PC 改成 LR，CPU 恢复执行后直接回到调用者，
     * 函数体（含 strncpy）完全不执行。
     */
    regs->pc = regs->regs[30];
    return 1;
}

static int __init midas_fix_init(void)
{
    midas_kp.symbol_name = "midas_record_task_times";
    midas_kp.pre_handler = midas_pre_handler;

    if (register_kprobe(&midas_kp) < 0) {
        pr_err("midas_fix: register_kprobe failed\n");
        return -EINVAL;
    }
    pr_info("midas_fix: hooked midas_record_task_times, function skipped\n");
    return 0;
}

static void __exit midas_fix_exit(void)
{
    unregister_kprobe(&midas_kp);
}

module_init(midas_fix_init);
module_exit(midas_fix_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Skip oplus_bsp_midas midas_record_task_times to avoid NULL deref");
