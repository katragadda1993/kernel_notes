// SPDX-License-Identifier: GPL-2.0
/*
 * Lesson 07: object lifetime, kref ownership and asynchronous work.
 * Build only against the exact target kernel's build tree. This module is
 * intentionally not loaded on a kernel with a mismatched vermagic/config.
 */
#include <linux/atomic.h>
#include <linux/errno.h>
#include <linux/jiffies.h>
#include <linux/kref.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#define DRV_NAME "lesson07_lifetime"

static unsigned int delay_ms = 5000;
module_param(delay_ms, uint, 0444);
MODULE_PARM_DESC(delay_ms, "One-shot delayed work delay in milliseconds (1..60000)");

static bool fail_after_queue;
module_param(fail_after_queue, bool, 0400);
MODULE_PARM_DESC(fail_after_queue, "Return -EIO after queueing, exercising init-failure unwind");

struct lesson07_object {
	struct kref ref;
	struct delayed_work work;
	atomic_t stopping;
	unsigned int generation;
};

static struct lesson07_object *active_object;

static void lesson07_object_release(struct kref *ref)
{
	struct lesson07_object *obj = container_of(ref, struct lesson07_object, ref);

	pr_info(DRV_NAME ": final release generation=%u\n", obj->generation);
	kfree(obj);
}

static void lesson07_workfn(struct work_struct *work)
{
	struct lesson07_object *obj = container_of(to_delayed_work(work),
						   struct lesson07_object, work);

	if (!atomic_read(&obj->stopping))
		pr_info(DRV_NAME ": delayed callback generation=%u\n",
			obj->generation);

	/* Drop the reference owned by this queued work item. */
	kref_put(&obj->ref, lesson07_object_release);
}

static void lesson07_drop_pending_work_ref(struct lesson07_object *obj)
{
	/*
	 * cancel_delayed_work_sync() returns true when it canceled a pending
	 * timer/work item. In that case the callback will not run, so this
	 * path must drop the work item's reference. If it returns false,
	 * a running callback has completed (or already completed) and that
	 * callback owns the put.
	 */
	if (cancel_delayed_work_sync(&obj->work))
		kref_put(&obj->ref, lesson07_object_release);
}

static int __init lesson07_init(void)
{
	struct lesson07_object *obj;

	if (!delay_ms || delay_ms > 60000)
		return -EINVAL;

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj)
		return -ENOMEM;

	kref_init(&obj->ref); /* initial reference: module owner */
	atomic_set(&obj->stopping, 0);
	obj->generation = 1;
	INIT_DELAYED_WORK(&obj->work, lesson07_workfn);

	/* Acquire the callback's reference before publishing the work item. */
	kref_get(&obj->ref);
	if (!queue_delayed_work(system_wq, &obj->work,
				msecs_to_jiffies(delay_ms))) {
		/* Queue failed: callback will not consume its reference. */
		kref_put(&obj->ref, lesson07_object_release);
		kref_put(&obj->ref, lesson07_object_release);
		return -EBUSY;
	}

	if (fail_after_queue) {
		atomic_set(&obj->stopping, 1);
		lesson07_drop_pending_work_ref(obj);
		/* Failed module_init() is not followed by module_exit(). */
		kref_put(&obj->ref, lesson07_object_release);
		return -EIO;
	}

	active_object = obj;
	pr_info(DRV_NAME ": loaded; delay_ms=%u\n", delay_ms);
	return 0;
}

static void __exit lesson07_exit(void)
{
	struct lesson07_object *obj = active_object;

	if (!obj)
		return;

	/* Stop callback behavior before synchronizing with it. */
	atomic_set(&obj->stopping, 1);
	lesson07_drop_pending_work_ref(obj);

	/* Drop the module owner's reference after the callback is drained. */
	active_object = NULL;
	kref_put(&obj->ref, lesson07_object_release);
	pr_info(DRV_NAME ": unloaded\n");
}

module_init(lesson07_init);
module_exit(lesson07_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Masterclass");
MODULE_DESCRIPTION("Lesson 07 kref ownership and delayed-work lifetime lab");
MODULE_VERSION("1.0");
