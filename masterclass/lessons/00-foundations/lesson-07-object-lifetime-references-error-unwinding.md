# Lesson 07 — Kernel Object Lifetime, References and Error Unwinding

**Stage:** 2 — Kernel C and essential APIs; bridge into Stage 4 driver fundamentals  
**Time:** 90 minutes  
**Source baseline:** Linux v7.4-rc4, with separately identified post-tag mainline case studies  
**Prerequisites:** Lessons 03, 05 and 06  
**Lab:** external module using `kref` and delayed work; compiled against Debian 6.12.107 headers, not loaded on the 6.18.57 host

## 1. Learning objectives

By the end, you should be able to:

- Distinguish an allocated object, a valid pointer, a held reference, published visibility and hardware ownership.
- Use `ERR_PTR()`, `IS_ERR()`, `PTR_ERR()` and `PTR_ERR_OR_ZERO()` correctly.
- Design an explicit ownership protocol with `refcount_t`, `kref`, `get_device()` and `put_device()`.
- Explain why a reference count does not replace a lock, RCU, memory ordering or a hardware stop sequence.
- Choose between devres-managed and explicitly reference-counted resources.
- Unwind partial initialization and probe failures in a safe order.
- Drain work, timers and callbacks before releasing their context or module code.
- Diagnose use-after-free, refcount underflow, double put, leaked reference and teardown races.

## 2. Why lifetime is a first-class kernel design problem

A driver object is rarely used by only one function. It may be referenced by probe/remove, a file descriptor, sysfs, an IRQ thread, a work item, a timer, a DMA completion, a firmware callback and a subsystem's own object model.

The hard question is not merely “who allocated this memory?” It is:

> Who can still reach this object, what proves that it is alive, and what event proves that every user has stopped before the final release?

A pointer value is not proof of ownership. A reference count is not a lock. A module reference is not proof that a bus master has stopped. A devm allocation is not automatically safe for an object that must outlive device detach.

### Five separate contracts

| Contract | What it answers | Typical mechanism |
|---|---|---|
| Storage lifetime | When may memory be freed? | `kref`, `refcount_t`, device/kobject references |
| Publication/lookup | Can a reader safely find the object? | Mutex/spinlock, RCU, subsystem registry |
| State synchronization | Can two users mutate state safely? | Mutex, spinlock, atomics, seqcount |
| Asynchronous execution | Can callbacks still run? | cancel/flush/synchronize/drain APIs |
| Hardware ownership | Can a device/firmware still access memory? | Stop engine, clear bus mastering, terminate DMA, protocol acknowledgement |

These contracts overlap, but none substitutes for all the others.

## 3. The ownership model

Use one initial reference for the creator/owner. Every independent user that keeps the object beyond the current call must acquire a reference before receiving the pointer and must release exactly that reference once.

```text
allocate object
     |
     v
kref_init()  ---- initial owner reference = 1
     |
     +---- kref_get() for queued work ------> work owns one reference
     |
     +---- kref_get() for open file --------> file owns one reference
     |
     +---- kref_get() for registry ---------> registry owns one reference
     |
     v
remove from publication / reject new users
     |
stop hardware + drain asynchronous users
     |
each owner calls kref_put()
     |
last put invokes release callback
     |
free object (or defer final free through RCU)
```

### Rules to memorize

1. Initialize a new `kref` with `kref_init()`; its count becomes one.
2. If you already hold a valid reference, call `kref_get()` before transferring an independent ownership to another thread or subsystem.
3. Every ownership must have one matching `kref_put()`.
4. The last `kref_put()` invokes the release callback.
5. Never acquire a reference from an unprotected raw pointer. Serialize lookup plus reference acquisition with a lock, or use a correctly designed RCU lookup and `kref_get_unless_zero()`.
6. A reference count protects lifetime, not concurrent field updates. Use the appropriate lock/atomic protocol for mutable state.
7. The release callback runs in the context of the final putter. Do not sleep there unless that caller context is guaranteed sleepable; defer sleeping cleanup to work if necessary.

## 4. Error pointers are not NULL pointers

Many kernel APIs return either a valid pointer or an encoded negative errno:

```c
foo = foo_get(dev);
if (IS_ERR(foo))
        return dev_err_probe(dev, PTR_ERR(foo), "failed to get foo\n");
```

The common helpers are:

| Helper | Meaning |
|---|---|
| `ERR_PTR(-errno)` | Encode a negative errno as a pointer return |
| `IS_ERR(ptr)` | Is this pointer an encoded error? |
| `PTR_ERR(ptr)` | Recover the negative errno |
| `PTR_ERR_OR_ZERO(ptr)` | Return the error or zero |
| `ERR_CAST(ptr)` | Cast an error pointer while preserving its encoded value |
| `IS_ERR_OR_NULL(ptr)` | True for either an error pointer or NULL; use only if the API allows both |

Example of an API that deliberately distinguishes “not present” from “defer”:

```c
foo = devm_foo_get_optional(dev);
if (IS_ERR(foo))
        return dev_err_probe(dev, PTR_ERR(foo), "foo lookup failed\n");
if (!foo)
        return 0; /* optional resource is absent */
```

Do not turn `-EPROBE_DEFER` into success. Do not dereference an error pointer. Do not use `IS_ERR_OR_NULL()` merely because it is shorter: it can erase a meaningful distinction between “optional resource absent” and “provider not ready”.

Pinned source: [include/linux/err.h at v7.4-rc4](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/err.h).

## 5. refcount_t and kref

### 5.1 refcount_t

`refcount_t` is a reference-count-specific type, not a general-purpose integer. It detects/saturates problematic overflow and underflow cases to avoid wraparound turning a dead object into an apparently live one. Its memory-ordering guarantees are intentionally narrower than a general lock or publication primitive.

Useful APIs include:

- `refcount_set()`, `refcount_read()`
- `refcount_inc()`, `refcount_inc_not_zero()`
- `refcount_dec_and_test()`

Do not replace it with `atomic_t` just because the latter has familiar arithmetic operations. Conversely, do not use a reference count to count arbitrary events, packets or statistics.

### 5.2 kref

`struct kref` wraps a `refcount_t` and standardizes object lifetime with a release callback:

```c
struct command_context {
        struct kref ref;
        struct mutex lock;
        bool stopping;
};

static void command_context_release(struct kref *ref)
{
        struct command_context *ctx =
                container_of(ref, struct command_context, ref);

        kfree(ctx);
}

static struct command_context *command_context_alloc(void)
{
        struct command_context *ctx;

        ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
        if (!ctx)
                return NULL;

        kref_init(&ctx->ref);
        mutex_init(&ctx->lock);
        return ctx;
}

/* Caller must already own a valid reference to ctx. */
static void command_context_get(struct command_context *ctx)
{
        kref_get(&ctx->ref);
}

static void command_context_put(struct command_context *ctx)
{
        kref_put(&ctx->ref, command_context_release);
}
```

### 5.3 Safe lookup is a separate problem

This is unsafe if another CPU can remove and free `entry` between the lookup and `kref_get()`:

```c
entry = lookup_without_protection(key);
if (entry)
        kref_get(&entry->ref); /* possibly already freed */
```

A lock-based registry can hold its lookup lock while acquiring a reference. An RCU registry can use a carefully paired publication/removal scheme:

```c
rcu_read_lock();
entry = rcu_dereference(registry->entry);
if (entry && !kref_get_unless_zero(&entry->ref))
        entry = NULL;
rcu_read_unlock();

if (entry) {
        /* This caller now owns a reference. */
        use_entry(entry);
        kref_put(&entry->ref, entry_release);
}
```

The writer must remove the pointer from the registry and defer the final memory reclamation until pre-existing RCU readers can no longer be using the pointer. A typical design uses `rcu_assign_pointer()` for publication and an RCU-safe release strategy. Review the exact lock/RCU/refcount protocol; copying this fragment alone does not create a complete registry.

**Key distinction:** RCU protects a reader's temporary pointer traversal. The acquired kref allows the object to outlive the RCU read-side critical section. The kref alone does not make the lookup safe.

Pinned sources: [include/linux/kref.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/kref.h), [include/linux/refcount.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/refcount.h), [Documentation/core-api/kref.rst](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/core-api/kref.rst).

## 6. Device and kobject references

The driver model has its own object lifetime:

- `get_device(dev)` obtains a reference to a `struct device`.
- `put_device(dev)` releases it.
- The device's release callback is called when the final device reference is dropped.
- `kobject_get()` / `kobject_put()` manage a kobject's lifetime.
- `kobject_del()` removes sysfs visibility; it is not the same operation as dropping the final reference with `kobject_put()`.

A subtle but important trap: holding a reference to `struct device` does not automatically extend the lifetime of arbitrary private memory allocated with `devm_kzalloc()`. Devres is released on driver detach. If an open file, callback or another subsystem must outlive detach, give that private state an explicit lifetime (or embed it in an object whose release is tied to the device's final reference).

For a character device, design the remove/open race deliberately: stop new opens, mark the device dead under the state lock, remove public registration, let existing file instances hold their own object references, and release the private object only after the last file/callback reference goes away. Use the subsystem's established cdev/device pattern rather than inventing a parallel lifetime model.

Pinned source: [include/linux/device.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/device.h), [drivers/base/core.c](https://github.com/torvalds/linux/blob/v7.4-rc4/drivers/base/core.c).

## 7. devres: convenient cleanup, not reference counting

Devres associates resources with a `struct device`. When probe fails or the driver detaches, the devres core invokes the registered release functions and frees the records. It exists partly to reduce the historically error-prone cleanup paths for mapped registers, IRQs, clocks, GPIOs and memory.

Common APIs include:

- `devm_kzalloc()`, `devm_kcalloc()`
- `devm_ioremap_resource()`, `devm_platform_ioremap_resource()`
- `devm_request_irq()`
- `devm_clk_get()`, `devm_regulator_get()`
- `devm_add_action_or_reset()`
- `devres_open_group()`, `devres_release_group()`

Example custom action:

```c
static void stop_engine_action(void *data)
{
        struct mydev *mdev = data;

        mydev_stop_engine(mdev);
}

ret = devm_add_action_or_reset(dev, stop_engine_action, mdev);
if (ret)
        return ret;
```

The `_or_reset` form runs the action immediately if registering the action itself fails, so the caller does not leak the resource it intended to manage.

### When devm is appropriate

- The resource belongs to the device binding and must disappear at detach.
- Its release ordering relative to other devres resources is understood.
- The driver stops asynchronous/hardware activity in `remove()` before devres automatically releases memory and mappings.

### When explicit lifetime is needed

- An object remains accessible through an open file after device unbind.
- A firmware/DSP callback can arrive after the initiating function returns.
- Multiple devices/interfaces share one private object.
- The object is registered in a subsystem that can retain it after the driver's remove callback.
- The final release must happen only after independent users have dropped their references.

Devres does not reference-count the data area. Its bookkeeping operations are protected, but concurrent access to the resource itself remains the driver's responsibility. Devres groups can provide transactional rollback for a mid-layer operation that acquires several resources and must release all of them if a later step fails.

Pinned source: [Documentation/driver-api/driver-model/devres.rst](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/driver-api/driver-model/devres.rst), [drivers/base/devres.c](https://github.com/torvalds/linux/blob/v7.4-rc4/drivers/base/devres.c).

## 8. Error unwinding: every edge is a path

Probe is not a transaction automatically rolled back by the driver core. If a manually managed acquisition succeeds and a later step fails, the driver must release it. A failed `probe()` does not call the driver's `remove()` as a substitute for probe cleanup.

A manual pattern:

```c
static int my_probe(struct platform_device *pdev)
{
        struct mydev *mdev;
        int ret;

        mdev = kzalloc(sizeof(*mdev), GFP_KERNEL);
        if (!mdev)
                return -ENOMEM;

        ret = my_clock_setup(mdev);
        if (ret)
                goto err_free;

        ret = my_register_subsystem(mdev);
        if (ret)
                goto err_clock;

        ret = my_start_async_activity(mdev);
        if (ret)
                goto err_unregister;

        return 0;

err_unregister:
        my_unregister_subsystem(mdev);
err_clock:
        my_clock_teardown(mdev);
err_free:
        kfree(mdev);
        return ret;
}
```

The labels unwind in reverse acquisition order. Real code must include every partially acquired sub-resource and must ensure that asynchronous users are stopped before the memory they reference is freed.

### Error-path audit table

| Acquisition / publication | Failure or remove action |
|---|---|
| Allocate private object | Free it, unless transferred to a refcounted owner |
| Enable clock / regulator | Disable it after hardware is quiesced |
| Map/request MMIO | Unmap/release, or let devres do so after remove |
| Request IRQ | Stop device interrupt generation, then synchronize/free IRQ |
| Allocate/map DMA | Stop bus mastering, terminate DMA, unmap/free |
| Register subsystem object | Unregister/unpublish before final object release |
| Queue work/timer | Prevent requeue, cancel/flush synchronously, then drop callback ref |
| Publish in lookup table | Remove from lookup first; then drop registry ref |
| Register firmware endpoint | Stop new messages, unregister endpoint, drain callbacks, release shared memory |

A common ordering mistake is to free a DMA ring and only then disable the engine. The device may still fetch descriptors after the CPU has returned the pages to the allocator.

## 9. Asynchronous work and the module/device lifetime boundary

For every queued callback, answer two questions:

1. Which reference keeps its context alive until the callback finishes?
2. What prevents the callback's code from executing after the module or driver is gone?

The lab takes one kref for the module owner and one for the queued work. The callback drops the work reference. During unload, the module sets a stopping state, calls `cancel_delayed_work_sync()`, accounts for whether pending work was canceled or had already run, and finally drops the owner's reference.

Important API distinctions:

- `cancel_work()` / `cancel_delayed_work()` do not wait for a running callback to finish.
- `cancel_work_sync()` / `cancel_delayed_work_sync()` wait for execution to finish, subject to the API's racing-enqueue rules.
- `flush_work()` waits for a particular work instance; it does not generally mean “prevent all future queueing”.
- `drain_workqueue()` drains a workqueue, including permitted chained work. The caller must first prevent unrelated new producers.
- Never call a synchronous wait/cancel function from a context in which it may sleep, or while holding a lock that the callback needs.

For a real PCIe/DMA driver, teardown normally needs to: stop new users; mark the object stopping; disable/mask the device source; stop the DMA engine and verify it is idle; synchronize IRQ/threaded handlers; cancel/flush work and timers; unregister subsystem/firmware callbacks; unmap/free DMA; drop object references; and only then let devres release the remaining device resources. The exact order depends on the hardware contract and subsystem.

Pinned source: [Documentation/core-api/workqueue.rst](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/core-api/workqueue.rst), [include/linux/workqueue.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/workqueue.h).

## 10. RCU grace periods and reference counts are different

A refcount answers: “How many owners have promised to keep this object alive?”

An RCU grace period answers: “Have all readers that could have obtained the old published pointer before removal passed through a quiescent state?”

If the final release callback only needs to defer memory freeing, `kfree_rcu()` may be suitable. If dismantling the object requires operations that may sleep, an RCU callback cannot simply perform those operations. One design is to use `rcu_work` or have the RCU callback queue process-context work, and defer the full teardown until after the grace period.

Do not free sub-objects before the grace period if an RCU reader can still dereference them. Deferring only the outer `kfree()` is insufficient when members were already dismantled.

## 11. Mainline source archaeology: lifetime fixes that motivated this lesson

### io_uring: defer the whole context teardown to RCU work

- [Merged mainline pull: 7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046](https://github.com/torvalds/linux/commit/7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046) — merged 2026-11-28 02:56:58 UTC.
- [Child fix: a905ff43362e80590e54f6ea9ddb2d1a998aa01c](https://github.com/torvalds/linux/commit/a905ff43362e80590e54f6ea9ddb2d1a998aa01c).

The commit explains that a final `kfree_rcu(ctx)` alone was not enough: io_uring task-work readers could still inspect ring fields such as `rings`, `rings_rcu` and `submitter_task`, while the exit worker freed those members before the final context free. The fix queues the full `io_ring_ctx_free()` as RCU work. That both waits for the relevant grace period and runs the teardown in process context, where its cleanup can sleep.

**Driver lesson:** defer the whole teardown when readers may still touch members—not just the outer allocation. This pattern is relevant to firmware IPC contexts, audio stream state and PCIe queues that are visible through lockless lookup paths.

### io_uring: release cached task references on ring release

- [Child fix: 6f3159575d53eed2d54557aef2dd8373da7c0208](https://github.com/torvalds/linux/commit/6f3159575d53eed2d54557aef2dd8373da7c0208).

The commit describes a task_struct reference cache used to avoid an atomic operation for every request. A task could exit while a request remained in flight; the later ring-release cancellation path could put a task reference into a cache after the normal exit path had already performed its final cache drain. The fix explicitly drops those cached references after cancellation.

**Driver lesson:** batching/caching reference drops changes the lifetime proof. Every exit path—including error, close, cancellation and cross-task completion—must drain the cache at the correct boundary.

### Workqueue drain: chained work can cross worker pools

- [Merged workqueue fix: 94ff2d27fafad99c63c02ab4d7e148c37e4eff80](https://github.com/torvalds/linux/commit/94ff2d27fafad99c63c02ab4d7e148c37e4eff80).
- [Maintainer pull discussion](https://lkml.iu.edu/hypermail/linux/kernel/2611.3/04049.html).

The maintainer report explains that `drain_workqueue()` could see one pool-workqueue (pwq) empty, then miss a chained work item queued onto that pwq by work running on another pwq. The drain could return early; destroying the queue could then warn and leak it. The fix tracks started work and performs a second check so the drain detects work chained across pwqs.

**Driver lesson:** “I flushed the workqueue” is only as strong as the exact API contract and producer restrictions. Chained work, requeueing, multiple worker pools and teardown ordering all matter.

### Checkpoint reconciliation

The previous upstream checkpoint did not list the Nov 25 workqueue pull or several Nov 28 morning merges that were already visible before that checkpoint. This lesson records them as reconciled activity, not as new changes after the checkpoint. The 7cd8495 io_uring pull is genuinely newer than the prior 08:12 IST checkpoint.

## 12. Lab — kref plus delayed-work ownership

Files:

- [lesson07_lifetime.c](lesson-07-object-lifetime-references-error-unwinding/lesson07_lifetime.c)
- [Makefile](lesson-07-object-lifetime-references-error-unwinding/Makefile)
- [Lab README](lesson-07-object-lifetime-references-error-unwinding/README.md)

Build against the exact target kernel:

```sh
make KDIR=/path/to/exact/kernel/build
modinfo ./lesson07_lifetime.ko
readelf -h ./lesson07_lifetime.ko
```

The optional runtime test belongs only in a disposable VM booted into the exact kernel used for the build:

```sh
sudo insmod ./lesson07_lifetime.ko delay_ms=5000
sudo dmesg | tail -n 20
sudo rmmod lesson07_lifetime
sudo dmesg | tail -n 20

# Failed module_init() path: init must unwind its own resources.
sudo insmod ./lesson07_lifetime.ko fail_after_queue=1
sudo dmesg | tail -n 20
```

Expected ownership behavior:

| Event | Owner reference | Work reference | Total |
|---|---:|---:|---:|
| `kref_init()` | 1 | 0 | 1 |
| `kref_get()` before queue | 1 | 1 | 2 |
| Callback runs and puts | 1 | 0 | 1 |
| Or pending work is canceled synchronously and cancel path puts | 1 | 0 | 1 |
| Owner puts after work is drained | 0 | 0 | 0 |

If the delayed callback has already started, `cancel_delayed_work_sync()` waits for it; the callback owns the work-reference put. If the delayed item was still pending and gets canceled, the cancel path owns that put. Do not put both references from the same path.

### Validation record

- Build command: `make KDIR=/usr/src/linux-headers-6.12.107+deb13-amd64 clean all`.
- Kbuild compile, modpost and link succeeded.
- `modinfo` showed the module metadata, parameters and 6.12.107 vermagic.
- `readelf -h` identified an ELF64 little-endian x86-64 relocatable module.
- Running host: 6.18.57. The module was not loaded because the build headers do not match the running kernel.
- Runtime cancellation, failed-init behavior, KASAN/KCSAN, lockdep, Sparse, arm64 and hardware behavior are **not validated**.

## 13. Debugging playbook

| Symptom | Likely cause | Evidence to collect |
|---|---|---|
| `refcount_t: underflow` | Double put or wrong ownership transfer | Every get/put path, error unwind, close/remove race |
| `refcount_t: addition on 0` | Attempt to resurrect an object after final put | Lookup serialization, RCU protocol, registry removal |
| KASAN use-after-free in workqueue | Callback still holds context after free | Work queue/cancel/flush call paths and callback references |
| Leak after probe failure | A failure edge missed a release or reference put | Inject failure after each acquisition; compare ownership ledger |
| Crash only during unbind | Public entry point or async producer remained active | Open file, sysfs, IRQ, timer, work, firmware and DMA users |
| Hang in remove | Synchronous drain waits for a callback that needs a lock held by remove | Lockdep, worker stack, wait-for graph, workqueue tracepoints |
| DMA corruption after remove | Hardware still owns/fetches a descriptor or payload | Device idle status, bus mastering, DMA termination and IOMMU faults |

Recommended tools: KASAN for use-after-free, KCSAN for data races, lockdep for lock-order problems, kmemleak for leaks, fault injection for error paths, ftrace/workqueue tracepoints for callback ordering, and targeted counters/logs for reference ownership. Logging itself must not dereference an object after its lifetime ended.

## 14. Exercises

- **Beginner:** draw an ownership table for one object with a creator, registry and work item. Mark every get and put.
- **Intermediate:** extend the lab with a second independent callback and prove that unload cannot free the object while either callback is pending/running.
- **Advanced:** design a private object shared by two SoundWire interfaces and an ALSA PCM stream. Device unbind must stop new stream operations, quiesce DMA, drain firmware/IRQ/work callbacks, and preserve the object for already-open users until their final put.
- **Advanced:** design an RCU-published registry where a reader obtains a kref safely and the writer removes the object without freeing it before pre-existing readers complete.
- **Review exercise:** inspect the io_uring RCU-work fix and explain why delaying only the final `kfree()` was insufficient.

## 15. Knowledge check

1. Why does `kref_get()` require an already-valid reference?
2. What is the difference between `IS_ERR()` and `IS_ERR_OR_NULL()`?
3. Does a kref protect an object found through an unlocked raw pointer lookup?
4. Does `get_device()` automatically extend the lifetime of `devm_kzalloc()` private data?
5. Why is devres not a substitute for reference counting?
6. What must happen before a DMA buffer is freed?
7. Why is `cancel_delayed_work()` weaker than `cancel_delayed_work_sync()` for teardown?
8. When is `kfree_rcu()` insufficient and a full RCU-work teardown useful?
9. Why must a failed `probe()` unwind its own explicit resources?
10. How can a cached/batched reference scheme leak an object even when individual get/put operations look balanced?

Answer checkpoints are in the [interview question bank](../../INTERVIEW-QUESTION-BANK.md), questions 187–196.

## 16. References

All implementation links below are pinned to v7.4-rc4 unless a separate commit is explicitly named:

- [include/linux/kref.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/kref.h)
- [include/linux/refcount.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/refcount.h)
- [include/linux/err.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/err.h)
- [include/linux/device.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/device.h)
- [drivers/base/core.c](https://github.com/torvalds/linux/blob/v7.4-rc4/drivers/base/core.c)
- [drivers/base/devres.c](https://github.com/torvalds/linux/blob/v7.4-rc4/drivers/base/devres.c)
- [Documentation/core-api/kref.rst](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/core-api/kref.rst)
- [Documentation/driver-api/driver-model/devres.rst](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/driver-api/driver-model/devres.rst)
- [Documentation/core-api/workqueue.rst](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/core-api/workqueue.rst)
- [Workqueue maintainer pull discussion](https://lkml.iu.edu/hypermail/linux/kernel/2611.3/04049.html)
- [io_uring RCU-work commit](https://github.com/torvalds/linux/commit/a905ff43362e80590e54f6ea9ddb2d1a998aa01c)
- [io_uring cached task-reference fix](https://github.com/torvalds/linux/commit/6f3159575d53eed2d54557aef2dd8373da7c0208)

## 17. Completion criteria

A design is not complete until its ownership graph, reference accounting, publication/removal protocol, asynchronous drain, hardware stop, error unwind and final release context can all be explained. A successful module build proves compilation and linking only; it does not prove runtime lifetime correctness.
