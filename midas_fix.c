#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/mm.h>

#define TARGET_SYMBOL "strncpy"

/* 安全空字符串缓冲区，16字节全零，对齐到8字节 */
static const char safe_buf[16] __aligned(8) = {0};

static struct kprobe kp = {
    .symbol_name = TARGET_SYMBOL,
};

/*
 * ARM64 strncpy 调用约定：
 *   x0 - dest
 *   x1 - src
 *   x2 - count
 *
 * 我们检查 x1（源地址）。如果它落在无效的低地址区域（< PAGE_SIZE），
 * 说明是 NULL + offset 的结果，直接替换为 safe_buf。
 */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    unsigned long src = regs->regs[1];  /* x1: 源地址 */
    unsigned long dst = regs->regs[0];  /* x0: 目标地址 */
    unsigned long len = regs->regs[2];  /* x2: 长度 */

    if (src < PAGE_SIZE) {
        pr_info("strncpy_guard: 拦截非法源地址 0x%lx (dst=0x%lx len=%lu)，替换为安全缓冲区\n",
                src, dst, len);
        regs->regs[1] = (unsigned long)safe_buf;
    }

    return 0;
}

static int __init strncpy_guard_init(void)
{
    int ret;

    kp.pre_handler = handler_pre;

    ret = register_kprobe(&kp);
    if (ret < 0) {
        pr_err("strncpy_guard: register_kprobe 失败, ret=%d\n", ret);
        return ret;
    }

    pr_info("strncpy_guard: 已在 %s 上注册 kprobe\n", TARGET_SYMBOL);
    return 0;
}

static void __exit strncpy_guard_exit(void)
{
    unregister_kprobe(&kp);
    pr_info("strncpy_guard: kprobe 已注销\n");
}

module_init(strncpy_guard_init);
module_exit(strncpy_guard_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("拦截 strncpy 的非法源地址，防止 oplus_bsp_midas 空指针 panic");
