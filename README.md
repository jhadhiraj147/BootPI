# BootPI

A small AArch64 bare-metal program for the Raspberry Pi 4 Model B
(BCM2711, Cortex-A72). The first boot milestone parks secondary cores,
sets up a stack, clears BSS and enters a freestanding C loop.

Build with Clang, LLD, LLVM binary utilities and Make:

```sh
make
```

The output is `kernel8.img`, linked at physical address `0x80000`.
No C runtime or libraries are linked. Compilation and ELF inspection
can be checked on the host; physical boot has not been tested.
