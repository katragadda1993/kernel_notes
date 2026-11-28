# Lesson 05 lab — runtime module, parameters, logging and teardown

## Build against a specific target kernel

    make KDIR=/path/to/exact/kernel/build clean all
    modinfo ./lesson05_runtime.ko

Do not load a module unless its build tree, architecture, configuration, vermagic and symbol versions match the running target kernel. Use a disposable VM or test board.

## Safe load/unload exercise (only on a matching disposable test kernel)

    sudo insmod ./lesson05_runtime.ko message="DSP diagnostic heartbeat" repeat=5
    sudo dmesg -w
    # Enable the module's pr_debug call if dynamic debug is enabled:
    echo 'module lesson05_runtime +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
    cat /sys/module/lesson05_runtime/parameters/repeat
    # A writable parameter update affects the worker's next limit read:
    echo 2 | sudo tee /sys/module/lesson05_runtime/parameters/repeat
    sudo rmmod lesson05_runtime

The worker is intentionally finite. If it has already completed, increasing repeat does not restart it; reload the module for another run.

## Init failure and unwind exercise

    sudo insmod ./lesson05_runtime.ko fail_after_alloc=1
    # Expected: insmod fails with EIO and dmesg reports the injected failure.

The module allocates a context and copied message before the injected error, then frees both in reverse acquisition order. module_exit() is not called when module_init() fails.

## Dynamic debug

Mount debugfs if the target configuration supports it:

    sudo mount -t debugfs none /sys/kernel/debug
    cat /sys/kernel/debug/dynamic_debug/control | grep lesson05_runtime

Dynamic debug controls pr_debug() callsites; it is distinct from printk log-level filtering. Availability depends on the target kernel configuration.
