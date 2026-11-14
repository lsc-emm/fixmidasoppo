#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/sched.h>
#include <linux/nsproxy.h>
#include <linux/pid_namespace.h>
#include <asm/memory.h>   /* PAGE_OFFSET */

static int mode = 1;
module_param(mode, int, 0644);
MODULE_PARM_DESC(mode, "0=skip all (safe), 1=detect ghost (default)");

static struct kprobe kp_skip = {
    .symbol_name = "update_or_create_entry_locked",
};
static struct kprobe kp_detect = {
    .symbol_name = "midas_record_task_times",
};

/* ===== mode 0：跳过整个函数 ===== */
static int skip_pre(struct kprobe *p, struct pt_regs *regs)
{
    if (mode != 0)
        return 0;
    pr_info_ratelimited("midas_fix: [mode0] skip update_or_create_entry_locked\n");
    regs->pc = regs->regs[30];
    return 1;
}

/* ===== mode 1：检测 ghost task ===== */
static bool task_is_ghost(unsigned long task_addr)
{
    struct task_struct *task = (struct task_struct *)task_addr;
    struct nsproxy *nsp;
    struct pid_namespace *pns;

    /* 1. 空指针或用户空间地址 */
    if (task_addr < PAGE_OFFSET)
        return true;

    /* 2. 不是 current：可疑，保守跳过 */
    /*    正常 tick 路径下 x0 一定是 current */
    if (task != current)
        return true;

    /* 3. current：看 namespace level */
    nsp = task->nsproxy;
    if (!nsp)
        return false;
    pns = nsp->pid_ns_for_children;
    if (!pns)
        return false;

    /* level != 0 表示容器 namespace */
    return pns->level != 0;
}

static int detect_pre(struct kprobe *p, struct pt_regs *regs)
{
    unsigned long task_addr;

    if (mode != 1)
        return 0;

    task_addr = regs->regs[0];
    if (task_is_ghost(task_addr)) {
        const char *reason;
        if (task_addr < PAGE_OFFSET)
            reason = "null/user";
        else if ((void *)task_addr != current)
            reason = "non-current";
        else
            reason = "container";
        pr_info_ratelimited("midas_fix: [mode1] skip task=0x%lx (%s)\n",
                            task_addr, reason);
        regs->pc = regs->regs[30];
        return 1;
    }
    return 0;
}

static int __init midas_fix_init(void)
{
    int ret;

    if (mode == 0) {
        kp_skip.pre_handler = skip_pre;
        ret = register_kprobe(&kp_skip);
        if (ret < 0) {
            pr_err("midas_fix: register %s failed: %d\n",
                   kp_skip.symbol_name, ret);
            return ret;
        }
        pr_info("midas_fix: mode0 registered on %s\n", kp_skip.symbol_name);
    } else if (mode == 1) {
        kp_detect.pre_handler = detect_pre;
        ret = register_kprobe(&kp_detect);
        if (ret < 0) {
            pr_err("midas_fix: register %s failed: %d\n",
                   kp_detect.symbol_name, ret);
            return ret;
        }
        pr_info("midas_fix: mode1 registered on %s\n", kp_detect.symbol_name);
    } else {
        pr_err("midas_fix: invalid mode %d\n", mode);
        return -EINVAL;
    }
    return 0;
}

static void __exit midas_fix_exit(void)
{
    if (mode == 0)
        unregister_kprobe(&kp_skip);
    else if (mode == 1)
        unregister_kprobe(&kp_detect);
    pr_info("midas_fix: unloaded\n");
}

module_init(midas_fix_init);
module_exit(midas_fix_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Detect ghost tasks in oplus_bsp_midas to prevent panic");
