// SPDX-License-Identifier: GPL-2.0
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

static char *message = "hello from a kernel work item";
module_param(message, charp, 0444);
MODULE_PARM_DESC(message, "Message emitted by the delayed work item (read-only)");

static unsigned int repeat = 3;
static int repeat_set(const char *val, const struct kernel_param *kp)
{
	unsigned int parsed;
	int ret = kstrtouint(val, 0, &parsed);

	if (ret)
		return ret;
	if (parsed < 1 || parsed > 20)
		return -ERANGE;

	WRITE_ONCE(*(unsigned int *)kp->arg, parsed);
	return 0;
}

static const struct kernel_param_ops repeat_ops = {
	.set = repeat_set,
	.get = param_get_uint,
};
module_param_cb(repeat, &repeat_ops, &repeat, 0644);
MODULE_PARM_DESC(repeat, "Number of work-item messages (1..20; writable)");

static bool fail_after_alloc;
module_param(fail_after_alloc, bool, 0400);
MODULE_PARM_DESC(fail_after_alloc, "Inject an init failure after allocations (load-time only)");

struct lesson_ctx {
	struct delayed_work work;
	char *message;
	unsigned int iteration;
	bool stopping;
};

static struct lesson_ctx *ctx;

static void lesson_workfn(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct lesson_ctx *local = container_of(dwork, struct lesson_ctx, work);
	unsigned int limit = READ_ONCE(repeat);

	if (READ_ONCE(local->stopping))
		return;

	local->iteration++;
	pr_debug("work item %u/%u: %s\n", local->iteration, limit,
		 local->message);

	if (local->iteration < limit && !READ_ONCE(local->stopping))
		schedule_delayed_work(&local->work, msecs_to_jiffies(1000));
}

static int __init lesson_init(void)
{
	int ret = -ENOMEM;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->message = kstrdup(message, GFP_KERNEL);
	if (!ctx->message)
		goto err_free_ctx;

	INIT_DELAYED_WORK(&ctx->work, lesson_workfn);

	if (fail_after_alloc) {
		pr_err("injected failure after allocating context and message\n");
		ret = -EIO;
		goto err_free_message;
	}

	pr_info("loaded; repeat=%u; work starts after 1 second\n",
		READ_ONCE(repeat));
	schedule_delayed_work(&ctx->work, msecs_to_jiffies(1000));
	return 0;

err_free_message:
	kfree(ctx->message);
err_free_ctx:
	kfree(ctx);
	ctx = NULL;
	return ret;
}

static void __exit lesson_exit(void)
{
	if (!ctx)
		return;

	WRITE_ONCE(ctx->stopping, true);
	cancel_delayed_work_sync(&ctx->work);
	pr_info("unloaded after %u work item(s)\n", ctx->iteration);
	kfree(ctx->message);
	kfree(ctx);
	ctx = NULL;
}

module_init(lesson_init);
module_exit(lesson_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Masterclass");
MODULE_DESCRIPTION("Lesson 05: module parameters, logging, delayed work and safe teardown");
MODULE_VERSION("1.0");
