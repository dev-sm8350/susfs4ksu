## Introduction ##

An addon root hiding kernel patches and userspace module for KernelSU

This ksu module requires a susfs patched kernel to work

# Warning #
This is only experimental code, that said it can harm your system or cause performance hit, **YOU ARE !! W A R N E D !! already**

## Compatibility ##

The susfs kernel patches may differ for different kernel version. You may need to create your own patches for your kernel.

## Patch Instruction ##
1. Run `cp ./kernel_patches/KernelSU/10_enable_susfs_for_ksu.patch $KERNEL_ROOT/KernelSU/`
2. Run `cp ./kernel_patches/<kernel_version>/50_add_susfs_in_kernel.patch $KERNEL_ROOT/`
3. Run `cp ./kernel_patches/<kernel_version>/fs/susfs.c $KERNEL_ROOT/fs/`
4. Run `cp ./kernel_patches/<kernel_version>/include/linux/susfs.h $KERNEL_ROOT/include/linux/`
5. Run `cd $KERNEL_ROOT/KernelSU` and then `patch -p1 < 10_enable_susfs_for_ksu.patch`
6. Run `cd $KERNEL_ROOT` and then `patch -p1 < 50_add_susfs_in_kernel.patch`, **if there are failed patches, please find your own proper defined syscall functions and patch them yourself.**
7. Make sure again to have `CONFIG_KSU` and `CONFIG_KSU_SUSFS` enabled before building the kernel.
8. Build and flash the kernel.

## Build userspace tool and ksu module ##
1. Run `./build_ksu_susfs_tool.sh` to build the userspace tool `ksu_susfs`, and the arm64 and arm binary will be copied to `ksu_module_susfs/tools/`
2. Edit `./ksu_module_susfs/post-fs-data.sh` to add your own path for hiding.
3. Run `./build_ksu_module.sh` to build the KernelSU module.


## Credits ##
- KernelSU: https://github.com/tiann/KernelSU
- @Kartatz: for ideas and original commit from https://github.com/Dominium-Apum/kernel_xiaomi_chime/pull/1/commits/74f8d4ecacd343432bb8137b7e7fbe3fd9fef189
