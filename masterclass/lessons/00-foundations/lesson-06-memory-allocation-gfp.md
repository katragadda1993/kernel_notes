# Lesson 06 — Kernel Memory Allocation and GFP Context

**Stage:** 2 — Kernel C and essential APIs (bridge to Stage 3 memory internals)  
**Time:** 90 minutes  
**Source baseline:** Linux v7.4-rc4  
**Prerequisites:** Lessons 01–05  
**Lab build headers:** Debian 6.12.107+deb13-amd64  
**Lab host:** Linux 6.18.57 x86-64  
**Validation:** the module compiled and was inspected, but was not loaded because its build headers do not match the running kernel.

## 1. Learning objectives

After this lesson, you should be able to:

- Select between kmalloc, kzalloc, kcalloc, kmem_cache_alloc, alloc_pages, vmalloc and kvmalloc.
- Explain why a GFP mask is part of the correctness contract, not merely an allocation preference.
- Decide whether the current execution context may sleep or enter direct reclaim.
- Distinguish virtual contiguity, physical contiguity and device-visible DMA addressability.
- Explain the roles of the buddy page allocator and SLUB object allocator.
- Handle allocation failure, integer overflow, partial initialization and teardown safely.
- Inspect memory pressure without assuming that one counter proves the cause.

## 2. Why Linux has multiple allocation APIs

A driver does not simply ask for “some memory”. It asks for memory with constraints:

- How large is the object?
- Must the CPU see one virtually contiguous range?
- Must physical pages be contiguous?
- Must a device be able to DMA to it?
- Can the caller sleep while memory is reclaimed?
- Can the request recurse into filesystem or I/O reclaim?
- Is the allocation driven by untrusted userspace and subject to memory-cgroup accounting?
- Who owns the memory, and which event ends its lifetime?

One API cannot satisfy every combination efficiently. Linux therefore layers typed object allocation, page allocation, virtual mapping and DMA-specific allocation/mapping interfaces.

### Allocator stack

```text
Driver / subsystem
   |
   +-- kmalloc family --------> SLUB size-class cache ----+
   |                                                      |
   +-- kmem_cache_alloc ------> dedicated SLUB cache -----+--> backing pages
   |                                                      |       |
   +-- alloc_pages -----------> buddy page allocator -----+       +--> zones / NUMA
   |                                                              +--> reclaim / compaction
   +-- vmalloc ---------------> page allocator + kernel VA mapping
   |
   +-- DMA API ---------------> device-visible DMA address / IOVA
```

This is a conceptual map. Exact wrappers and fast paths depend on configuration, architecture, instrumentation and allocator implementation. Follow the pinned source rather than memorizing one internal function name.

## 3. Address and contiguity model

| Allocation | CPU virtual range | Physical pages | DMA suitability |
|---|---|---|---|
| kmalloc() | Contiguous | Contiguous for returned allocation | Still use DMA API; addressability/coherency is not implied |
| kmem_cache_alloc() | Contiguous object | Backed by slab pages | Not automatically a DMA buffer |
| alloc_pages(gfp, order) | Direct-map address when available | 2^order physically contiguous pages | Still requires DMA API and device constraints |
| vmalloc() | Contiguous | May be physically discontiguous | Do not pass to dma_map_single() as a kmalloc buffer |
| kvmalloc() | Contiguous | May be kmalloc- or vmalloc-backed | No physical-contiguity guarantee |
| dma_alloc_coherent() | CPU mapping returned by DMA API | Platform/IOMMU dependent | Coherent device/CPU shared buffer; use returned DMA handle |

Three terms must never be conflated:

1. **Virtual contiguity:** adjacent CPU virtual addresses.
2. **Physical contiguity:** adjacent physical page frames.
3. **DMA addressability:** the address and mapping the device is allowed to use, as returned/managed by the DMA API.

A device DMA address can be an IOVA. Never cast a CPU pointer or physical address into dma_addr_t.

## 4. Choosing an API

### 4.1 Small objects and arrays

- kmalloc(size, gfp): uninitialized memory.
- kzalloc(size, gfp): zero-initialized object.
- kmalloc_array(n, size, gfp): checks multiplication overflow.
- kcalloc(n, size, gfp): overflow-safe, zero-initialized array.
- krealloc()/krealloc_array(): resize an existing allocation; preserve and check the returned pointer.
- kfree(): release a kmalloc-family allocation.

Prefer kzalloc when zero initialization is part of the object invariant. Do not rely on freshly allocated memory containing zeroes.

For a structure with a flexible array member, use struct_size() rather than open-coded arithmetic:

```c
struct command_batch {
        u32 count;
        struct command commands[];
};

struct command_batch *batch;

batch = kzalloc(struct_size(batch, commands, nr_commands), GFP_KERNEL);
if (!batch)
        return -ENOMEM;
```

For a plain array, use kcalloc(n, sizeof(*array), GFP_KERNEL). An unchecked n * sizeof(*array) can wrap and allocate less memory than the caller later indexes.

### 4.2 Repeated fixed-size objects

Use kmem_cache_create() when a subsystem repeatedly allocates objects of one type:

```c
cache = kmem_cache_create("my_driver_desc",
                          sizeof(struct my_desc),
                          0, SLAB_HWCACHE_ALIGN, NULL);
if (!cache)
        return -ENOMEM;

desc = kmem_cache_zalloc(cache, GFP_KERNEL);
if (!desc)
        goto err_cache;

/* ... */

kmem_cache_free(cache, desc);
kmem_cache_destroy(cache); /* only after every object is returned */
```

A dedicated cache can provide type-specific alignment, reuse and diagnostic visibility. The optional constructor is not a substitute for resetting per-allocation state: use kmem_cache_zalloc() or initialize every field required by the object invariant.

If some bytes of an object are intentionally copied to userspace, investigate kmem_cache_create_usercopy() and the usercopy rules rather than exposing an arbitrary cache object.

### 4.3 Page allocations

alloc_pages(gfp, order) requests 2^order physically contiguous pages and returns a struct page *. Release the matching allocation with __free_pages(page, order).

- Order 0: one base page.
- Order 1: two adjacent base pages.
- Order 2: four adjacent base pages.

Do not assume the base page size is always 4 KiB. It is architecture/configuration dependent.

High-order allocations can fail even when MemAvailable looks large: free memory may be fragmented into smaller blocks, or zone watermarks and migratetype constraints may prevent satisfying the request.

### 4.4 Virtually contiguous large buffers

- vmalloc()/vzalloc(): allocate backing pages and map them into a contiguous kernel virtual range. Physical pages need not be adjacent.
- vfree(): release a vmalloc allocation.
- kvmalloc()/kvzalloc(): try kmalloc-style allocation first and can fall back to vmalloc.
- kvfree(): free memory that may have come from kmalloc, vmalloc or kvmalloc families.

Use these for CPU-only large tables or buffers when physical contiguity is unnecessary. Do not assume a kvmalloc result is physically contiguous.

### 4.5 Device-managed allocations

devm_kzalloc(dev, size, GFP_KERNEL) ties memory release to the device's devres lifetime. This simplifies probe error paths, but it does not stop DMA, cancel a work item, synchronize an IRQ or terminate firmware access.

Before devres releases memory, the driver must quiesce every asynchronous user. Managed lifetime is not a substitute for a teardown protocol.

## 5. GFP flags: allocation context is correctness

GFP means “get free pages”. The mask tells the allocator what it may do to satisfy the request.

| Flag | May sleep / direct reclaim? | Intended use |
|---|---|---|
| GFP_KERNEL | Yes | Normal process/workqueue/probe context |
| GFP_NOWAIT | No direct reclaim | Fast/atomic path with a fallback; can fail readily |
| GFP_ATOMIC | No | Critical atomic allocation that may use reserves; use sparingly |
| GFP_NOIO | Sleepable, but avoids I/O reclaim | Prevent reclaim recursion into I/O |
| GFP_NOFS | Sleepable, but avoids filesystem reclaim | Prevent reclaim recursion into filesystem paths |
| GFP_KERNEL_ACCOUNT | Yes | Kernel memory attributable to a memory cgroup |
| GFP_DMA32 | Depends on other flags | Legacy zone constraint; normally prefer DMA API constraints |

### 5.1 Context matrix

| Call site | Typical rule |
|---|---|
| probe() in normal driver-core context | GFP_KERNEL is usually appropriate |
| Workqueue function | May sleep; GFP_KERNEL is usually appropriate |
| Threaded IRQ handler | Kernel thread; may sleep subject to its locks/subsystem rules |
| Hard IRQ handler | Must not sleep; avoid allocating if possible |
| Softirq/tasklet/timer callback | Must not sleep; avoid allocation or use a justified nonblocking strategy |
| While holding a spinlock / with IRQs disabled | Must not enter a sleeping allocator path |
| Reclaim path that could recurse into filesystem/I/O | Use the appropriate NOFS/NOIO scope or carefully chosen mask |

**GFP_KERNEL can enter direct reclaim.** Reclaim can write pages, perform I/O and call filesystem code. If the caller holds a resource that reclaim needs, allocation can deadlock.

**GFP_NOWAIT is not “GFP_KERNEL but faster”.** It avoids direct reclaim and does not access the same emergency reserves as GFP_ATOMIC. The caller must have a fallback or tolerate failure.

**GFP_ATOMIC is not a fix for bad design.** It can dip into reserves intended to keep critical kernel paths alive. For a high-rate interrupt or audio completion path, preallocate descriptors/buffers during initialization and recycle them.

For reclaim recursion, prefer scoped helpers such as memalloc_noio_save()/restore() or memalloc_nofs_save()/restore() around the relevant call chain when appropriate. Do not blanket every allocation with GFP_NOFS or GFP_NOIO.

### 5.2 Allocation failure is normal control flow

Even GFP_KERNEL allocations can fail. Every result must be checked.

- Return -ENOMEM when the caller can fail cleanly.
- Release earlier resources on partial initialization failure.
- Do not spin/retry forever in a driver.
- Avoid __GFP_NOFAIL as a generic workaround; it can create unbounded stalls and is reserved for narrowly justified kernel invariants.
- Do not request an oversized allocation and assume the page allocator will always find a high-order block.

## 6. Buddy allocator and SLUB: two different jobs

### 6.1 Buddy page allocator

The page allocator manages physical page frames. Free blocks are organized by order. When a larger block is requested, a larger free block may be split; when compatible buddy blocks are freed, they can be coalesced.

Important constraints include NUMA node and zone, watermarks and reserved pages, migratetype, reclaim/compaction state, fragmentation and allocation order.

The buddy allocator is not the same as the C kmalloc() API. It is a lower-level source of backing pages.

### 6.2 SLUB object allocator

SLUB manages objects carved from slab pages. The general kmalloc() family uses size classes; typed kmem_cache instances serve repeated objects of a known size/layout.

A simplified view:

1. A CPU requests an object from a cache.
2. The fast path checks its local freelist.
3. If needed, allocator slow paths obtain/refill objects from partial slabs.
4. If more backing memory is required, pages are obtained from the page allocator.
5. Empty slabs can eventually be returned to the page allocator.

The exact implementation has evolved; inspect mm/slub.c at the lesson's pinned tag.

### 6.3 Why both layers matter

A driver allocating thousands of small descriptors stresses object allocation, cache locality and object lifetime. A driver requesting a large high-order page block stresses physical fragmentation and reclaim/compaction. Looking only at total free RAM cannot explain both.

## 7. DMA is a separate allocation contract

For a PCIe device, audio DMA engine or DSP:

- A descriptor ring often uses dma_alloc_coherent() and its returned dma_addr_t.
- Streaming payloads commonly use dma_map_single() or dma_map_sg(), with matching unmap/synchronization operations.
- A buffer allocated with vmalloc() is not a single physically contiguous buffer and must not be passed to dma_map_single() as though it were kmalloc memory.
- A kmalloc pointer is still not a DMA address. Use the DMA API and set the device DMA mask.
- For ALSA PCM buffers, use the ALSA/platform DMA-buffer mechanisms appropriate to the driver rather than inventing a private allocation path.
- For DSP shared memory, follow the platform's reserved-memory/CMA/IOMMU and firmware ownership contract.

Example design for an audio or PCIe engine:

```text
CPU control structures       -> kzalloc()/kmem_cache_zalloc()
Descriptor ring              -> dma_alloc_coherent()
Streaming audio/payload data -> DMA API mapping / ALSA buffer API
Large CPU-only lookup table   -> kvzalloc() if physical contiguity is unnecessary
```

Do not use GFP_DMA32 as a replacement for setting a DMA mask. The DMA API understands the device's addressing constraints and any IOMMU translation.

## 8. Hands-on lab: observe and allocate safely

Files:

- [lesson06_alloc.c](lesson-06-memory-allocation-gfp/lesson06_alloc.c)
- [Makefile](lesson-06-memory-allocation-gfp/Makefile)
- [memory-observe.sh](lesson-06-memory-allocation-gfp/memory-observe.sh)
- [Lab README](lesson-06-memory-allocation-gfp/README.md)

The module demonstrates:
- kzalloc(), kcalloc(), vzalloc() and a dedicated kmem_cache;
- a workqueue allocation with GFP_KERNEL;
- a one-shot timer callback allocation with GFP_ATOMIC;
- synchronous timer/work cancellation before freeing memory.

The timer allocation is pedagogical. Production interrupt/timer paths should generally recycle preallocated objects instead of allocating on every event.

### Build against the exact target kernel

```sh
make KDIR=/path/to/exact/target/kernel/build clean all
modinfo ./lesson06_alloc.ko
readelf -h ./lesson06_alloc.ko
```

Only in a disposable VM/board running the same kernel build and with module loading permitted:

```sh
sudo insmod ./lesson06_alloc.ko array_count=32 delay_ms=100
dmesg | tail -30
sudo rmmod lesson06_alloc
dmesg | tail -30
```

Expected messages include an init line, a worker allocation line, a timer allocation line (if the atomic allocation succeeds), and an exit line. The timer allocation may legitimately fail under pressure; the callback logs and returns safely.

Observe the host's current allocator state:

```sh
chmod +x ./memory-observe.sh
./memory-observe.sh
```

It reads /proc/meminfo, /proc/buddyinfo, /proc/pagetypeinfo, /proc/slabinfo and a sample of /sys/kernel/slab when available. It does not modify system state.

## 9. Debugging exercises

### Scenario A — “sleeping function called from invalid context”

A driver calls kmalloc(..., GFP_KERNEL) from a hard IRQ or while holding a spinlock.

Investigate:
1. Capture the warning stack.
2. Identify the actual call context, not just the source function name.
3. Find whether the path can be moved to a threaded IRQ/workqueue.
4. Prefer preallocation if latency is strict.
5. If allocation must remain nonblocking, define a fallback for failure.

### Scenario B — high-order allocation fails while free RAM is available

Investigate:
1. Check the requested order and base page size.
2. Inspect /proc/buddyinfo for free blocks at the requested order and zone.
3. Inspect /proc/pagetypeinfo, watermarks and NUMA placement.
4. Determine whether compaction/reclaim is allowed in the calling context.
5. Consider scatter-gather/page-based design rather than requiring one large physical block.

### Scenario C — DMA corruption after switching to vmalloc

The CPU code works, but the device reads invalid data.

Investigate:
1. Verify whether the buffer is physically contiguous.
2. Verify the DMA API used and the mapping direction.
3. Check that the device uses the returned DMA address, not a CPU pointer.
4. Check IOMMU mapping, DMA mask and cache synchronization.
5. Use coherent DMA allocation for a descriptor ring where appropriate; use SG mapping for noncontiguous payload pages.

### Scenario D — memory usage grows after repeated probe/remove

Investigate:
1. Track every allocation and every early-return edge.
2. Verify cache objects are freed before kmem_cache_destroy().
3. Stop DMA, synchronize IRQs and cancel work before freeing callback-visible memory.
4. Check reference counts and devres lifetime.
5. Use kmemleak, KASAN/KFENCE, fault injection and tracepoints when configured.

## 10. Observability and tools

- /proc/meminfo: broad counters, including Slab, SReclaimable and SUnreclaim.
- /proc/buddyinfo: free blocks by order and zone; useful for fragmentation hypotheses, not a complete allocation oracle.
- /proc/pagetypeinfo: zone/migratetype detail.
- /proc/slabinfo and slabtop: object cache occupancy.
- /sys/kernel/slab/<cache>/: cache attributes when exposed.
- Tracepoints such as kmem:kmalloc, kmem:kfree, kmem:mm_page_alloc and kmem:mm_page_free where enabled.
- CONFIG_PAGE_OWNER and page_owner debugfs for page allocation provenance.
- CONFIG_FAILSLAB / CONFIG_FAIL_PAGE_ALLOC for controlled failure injection.
- KASAN, KFENCE and kmemleak for memory safety/lifetime issues.

A counter is evidence for a hypothesis, not proof. Correlate the trace with the exact allocation size, GFP mask, order, node, zone and call stack.

## 11. Exercises

- **Beginner:** replace an unchecked kmalloc(n * size) with kcalloc() or kmalloc_array(); explain the overflow risk.
- **Intermediate:** modify the lab module to add a second cache object, then verify every failure edge frees all prior allocations.
- **Advanced:** design a PCIe descriptor ring and streaming payload path. Specify which allocations use coherent DMA, which use streaming mappings, the DMA mask, ownership transitions and remove-path ordering.
- **Advanced:** reproduce a high-order allocation failure in a disposable VM and distinguish fragmentation from low-memory pressure without assuming that MemFree alone explains it.

## 12. Knowledge check

1. Why is GFP_KERNEL invalid in a hard IRQ handler?
2. What does GFP_ATOMIC permit that GFP_NOWAIT does not?
3. Why can an order-3 allocation fail while many order-0 pages are free?
4. Which allocator backs most small kmalloc() requests?
5. What is the difference between vmalloc() and kvmalloc()?
6. Why is kvmalloc() not a generic DMA buffer allocator?
7. Why must kmem_cache_destroy() happen after all objects are returned?
8. When is GFP_KERNEL_ACCOUNT appropriate?
9. What can go wrong if a devm allocation is still used by a work item during device removal?
10. What is the difference between a DMA address and a CPU virtual address?

## 13. Answer checkpoints

1. GFP_KERNEL can sleep and enter direct reclaim; hard IRQ context cannot sleep.
2. GFP_ATOMIC can access emergency reserves; GFP_NOWAIT avoids direct reclaim without that reserve behavior and may fail sooner.
3. Buddy blocks may be fragmented; free pages are not necessarily adjacent in the required order/zone/migratetype.
4. SLUB size-class caches in the standard v7.4-rc4 configuration.
5. vmalloc maps noncontiguous physical pages into a contiguous virtual range; kvmalloc tries kmalloc and may fall back to vmalloc.
6. Its backing may be physically discontiguous; it does not supply the device mapping/coherency contract.
7. Destroying the cache while objects remain allocated violates its lifetime contract and can leak/corrupt allocator state.
8. For kernel allocations attributable to a memory cgroup, especially user-triggered/untrusted allocation paths.
9. The work item can dereference freed memory; cancel/synchronize asynchronous users before devres releases the allocation.
10. A CPU virtual address is used by CPU loads/stores; a DMA address is the device-visible address supplied by the DMA API and may be an IOVA.

## 14. References and exact source baseline

All source links below are pinned to **v7.4-rc4**:

- [Memory Allocation Guide](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/core-api/memory-allocation.rst)
- [Memory Management API reference](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/core-api/mm-api.rst)
- [GFP type definitions](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/gfp_types.h)
- [Slab allocation API](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/slab.h)
- [Slab common implementation](https://github.com/torvalds/linux/blob/v7.4-rc4/mm/slab_common.c)
- [SLUB implementation](https://github.com/torvalds/linux/blob/v7.4-rc4/mm/slub.c)
- [Buddy/page allocator](https://github.com/torvalds/linux/blob/v7.4-rc4/mm/page_alloc.c)
- [vmalloc implementation](https://github.com/torvalds/linux/blob/v7.4-rc4/mm/vmalloc.c)
- [DMA mapping API](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/dma-mapping.h)
- [DMA mapping implementation](https://github.com/torvalds/linux/blob/v7.4-rc4/kernel/dma/mapping.c)
- [Scoped NOFS/NOIO helpers](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/sched/mm.h)

Historical/design context:
- [Jeff Bonwick, The Slab Allocator, USENIX 1994](https://www.usenix.org/conference/usenix-summer-1994-technical-conference/slab-allocator-object-caching-kernel)
- [Bonwick and Adams, Magazines and Vmem, USENIX 2001](https://www.usenix.org/conference/2001-usenix-annual-technical-conference/magazines-and-vmem-extending-slab-allocator-many)
- [Linux 7.4 slab merge](https://github.com/torvalds/linux/commit/7a78f7ba9f24962c8033e0152bf34a7903ff61ba)
- [Linux 7.4-rc5 core MM merge](https://github.com/torvalds/linux/commit/1f1b6ca3145a1179d88b0033a871e4e187da0d4e)
