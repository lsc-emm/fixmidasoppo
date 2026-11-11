#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>

static struct kprobe midas_kp;

static int midas_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    /*
     * ARM64: bl 把返回地址放在 x30(LR)。
     * kprobe 在函数入口断下时 regs->pc 是入口地址，
     * regs->regs[30] 是调用者的返回地址。
     * 把 PC 改成 LR，return 1 表示不再执行原指令，
     * 恢复后直接回到调用者，函数体被完全跳过。
     */
    regs->pc = regs->regs[30];
    return 1;
}

static int __init midas_fix_init(void)
{
    midas_kp.symbol_name = "midas_record_task_times";
    midas_kp.pre_handler = midas_pre_handler;

    if (register_kprobe(&midas_kp) < 0) {
        pr_err("midas_fix: register_kprobe failed, symbol not found\n");
        return -EINVAL;
    }
    pr_info("midas_fix: midas_record_task_times skipped at entry\n");
    return 0;
}

static void __exit midas_fix_exit(void)
{
    unregister_kprobe(&midas_kp);
}

module_init(midas_fix_init);
module_exit(midas_fix_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Skip midas_record_task_times entirely to avoid NULL deref");
