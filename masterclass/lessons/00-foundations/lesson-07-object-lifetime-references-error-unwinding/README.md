# Lesson 07 lab — kref ownership and asynchronous work

This lab builds a small external module that gives one `kref` reference to the module owner and one to a delayed-work callback. It demonstrates the ownership protocol and init-failure cleanup. The callback is one-shot so the reference transfer is easy to audit.

## Build against an exact target kernel

```sh
make KDIR=/path/to/the/exact/target/kernel/build
modinfo ./lesson07_lifetime.ko
readelf -h ./lesson07_lifetime.ko
```

Do not load a module built against headers that do not match the running kernel's release, configuration, symbol versions and signing policy.

## Optional disposable-VM runtime exercise

Only in a disposable VM booted into the exact kernel build tree used above:

```sh
sudo insmod ./lesson07_lifetime.ko delay_ms=5000
sudo dmesg | tail -n 20
sudo rmmod lesson07_lifetime
sudo dmesg | tail -n 20
```

Expected sequence: load message; either the delayed callback message or cancellation during unload; exactly one final-release message; unload message. Timing determines whether the work callback runs before removal.

Exercise the failed-init path:

```sh
sudo insmod ./lesson07_lifetime.ko fail_after_queue=1
# Expected: insmod reports an I/O error; module is not resident.
sudo dmesg | tail -n 20
```

The failed-init path explicitly cancels/drains the work and drops both references because `module_exit()` is not called after failed initialization.

## Ownership accounting

| Event | Module-owner ref | Work-item ref | Total |
|---|---:|---:|---:|
| `kref_init()` | 1 | 0 | 1 |
| `kref_get()` before queue | 1 | 1 | 2 |
| Callback runs | 1 | 0 | 1 |
| Or pending work canceled synchronously | 1 | 0 | 1 |
| Module owner drops after drain | 0 | 0 | 0 |

If cancellation returns true, the callback will not run, so the cancel path drops the work reference. If it returns false, synchronous cancellation has waited for a running callback to finish, and the callback has dropped its own reference.

## Important limitations

- The lab demonstrates memory-object lifetime, not DMA ownership. A real device must be stopped and bus mastering quiesced before DMA buffers are released.
- The kref protects the object memory; it does not keep module text alive. The module exit path must synchronously drain work before returning.
- This module is intentionally simple and does not demonstrate a concurrent lookup table, RCU publication, file-descriptor lifetime or a real device's remove callback.
- Never test this on a production machine.
