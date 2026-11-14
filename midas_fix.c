#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>

/* 可通过 insmod skip_all=0 关闭全局跳过，仅做日志观察 */
static bool skip_all = true;
module_param(skip_all, bool, 0644);
MODULE_PARM_DESC(skip_all, "Skip update_or_create_entry_locked entirely (default: true)");

static struct kprobe kp = {
    .symbol_name = "update_or_create_entry_locked",
};

static int skip_pre(struct kprobe *p, struct pt_regs *regs)
{
    if (!skip_all)
        return 0;

    /* 限频打印，避免时钟中断路径刷屏 */
    pr_info_ratelimited("midas_fix: 跳过 update_or_create_entry_locked (lr=0x%lx)\n",
                        regs->regs[30]);

    /* 直接返回到调用者 */
    regs->pc = regs->regs[30];
    return 1;  /* 非 0 告知 kprobe 框架已修改 PC */
}

static int __init midas_fix_init(void)
{
    int ret;

    kp.pre_handler = skip_pre;

    ret = register_kprobe(&kp);
    if (ret < 0) {
        pr_err("midas_fix: register_kprobe(%s) 失败: %d\n", kp.symbol_name, ret);
        return ret;
    }

    pr_info("midas_fix: kprobe 已注册在 %s (skip_all=%d)\n",
            kp.symbol_name, skip_all);
    return 0;
}

static void __exit midas_fix_exit(void)
{
    unregister_kprobe(&kp);
    pr_info("midas_fix: 卸载\n");
}

module_init(midas_fix_init);
module_exit(midas_fix_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Skip update_or_create_entry_locked to prevent oplus_bsp_midas panic");
