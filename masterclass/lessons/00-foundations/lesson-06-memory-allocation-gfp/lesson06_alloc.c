// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/mm.h>
#include <linux/overflow.h>

#define CACHE_NAME "lesson06_alloc_obj"
#define MAX_ARRAY_COUNT 4096

struct lesson06_obj {
	u64 sequence;
	u8 payload[48];
};

static unsigned int array_count = 16;
module_param(array_count, uint, 0444);
MODULE_PARM_DESC(array_count, "Number of u32 elements allocated with kcalloc (1..4096)");

static unsigned int delay_ms = 100;
module_param(delay_ms, uint, 0444);
MODULE_PARM_DESC(delay_ms, "One-shot timer delay in milliseconds");

static void *kmalloc_buf;
static u32 *array_buf;
static void *vmalloc_buf;
static struct kmem_cache *obj_cache;
static struct lesson06_obj *cache_obj;
static struct work_struct alloc_work;
static struct timer_list alloc_timer;

static void alloc_work_fn(struct work_struct *work)
{
	void *p;

	/* Workqueue context may sleep: GFP_KERNEL can enter direct reclaim. */
	p = kmalloc(128, GFP_KERNEL);
	if (!p) {
		pr_warn("lesson06: worker allocation failed\n");
		return;
	}
	pr_info("lesson06: worker allocated 128 bytes with GFP_KERNEL\n");
	kfree(p);
}

static void alloc_timer_fn(struct timer_list *timer)
{
	void *p;

	/* Timer callbacks run in softirq context: do not use GFP_KERNEL here. */
	p = kmalloc(64, GFP_ATOMIC);
	if (!p) {
		pr_warn("lesson06: timer allocation failed; atomic path must tolerate failure\n");
		return;
	}
	pr_info("lesson06: timer allocated 64 bytes with GFP_ATOMIC\n");
	kfree(p);
}

static void lesson06_release_allocations(void)
{
	if (cache_obj)
		kmem_cache_free(obj_cache, cache_obj);
	if (obj_cache)
		kmem_cache_destroy(obj_cache);
	vfree(vmalloc_buf);
	kfree(array_buf);
	kfree(kmalloc_buf);
}

static int __init lesson06_init(void)
{
	size_t array_bytes;

	if (!array_count || array_count > MAX_ARRAY_COUNT)
		return -EINVAL;
	if (check_mul_overflow((size_t)array_count, sizeof(*array_buf), &array_bytes))
		return -EOVERFLOW;

	kmalloc_buf = kzalloc(128, GFP_KERNEL);
	if (!kmalloc_buf)
		goto err_nomem;

	array_buf = kcalloc(array_count, sizeof(*array_buf), GFP_KERNEL);
	if (!array_buf)
		goto err_nomem;

	/* vmalloc gives virtually contiguous, not physically contiguous, memory. */
	vmalloc_buf = vzalloc(2 * PAGE_SIZE);
	if (!vmalloc_buf)
		goto err_nomem;

	obj_cache = kmem_cache_create(CACHE_NAME, sizeof(struct lesson06_obj),
				      0, SLAB_HWCACHE_ALIGN, NULL);
	if (!obj_cache)
		goto err_nomem;

	cache_obj = kmem_cache_zalloc(obj_cache, GFP_KERNEL);
	if (!cache_obj)
		goto err_nomem;
	cache_obj->sequence = 1;

	INIT_WORK(&alloc_work, alloc_work_fn);
	timer_setup(&alloc_timer, alloc_timer_fn, 0);
	schedule_work(&alloc_work);
	mod_timer(&alloc_timer, jiffies + msecs_to_jiffies(delay_ms));

	pr_info("lesson06: init ok array_count=%u array_bytes=%zu page_size=%lu\n",
		array_count, array_bytes, PAGE_SIZE);
	return 0;

err_nomem:
	lesson06_release_allocations();
	return -ENOMEM;
}

static void __exit lesson06_exit(void)
{
	/* Stop asynchronous users before releasing memory or destroying the cache. */
	del_timer_sync(&alloc_timer);
	cancel_work_sync(&alloc_work);
	lesson06_release_allocations();
	pr_info("lesson06: exit complete\n");
}

module_init(lesson06_init);
module_exit(lesson06_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Masterclass");
MODULE_DESCRIPTION("Demonstrate kernel allocation APIs and GFP execution contexts");
