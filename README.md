## Introduction ##
- An addon root hiding kernel patches and userspace module for KernelSU.

- The userspace tool `ksu_susfs` and 'sus_su', as well as the ksu module, require a susfs patched kernel to work.

# Warning #
- This is only experimental code, that said it can harm your system or cause performance hit, **YOU ARE !! W A R N E D !!** already

## Compatibility ##
- The susfs kernel patches may differ for different kernel version or even on the same kernel version, you may need to create your own patches for your kernel.

## Patch Instruction ##
1. Clone the repo with a tag that has release version, as tag with release version is more stable
2. Run `cp ./kernel_patches/KernelSU/10_enable_susfs_for_ksu.patch $KERNEL_ROOT/KernelSU/`
2. Run `cp ./kernel_patches/KernelSU/kernel/sucompat.h $KERNEL_ROOT/KernelSU/kernel/`
3. Run `cp ./kernel_patches/50_add_susfs_in_kernel-<kernel_version>.patch $KERNEL_ROOT/`
4. Run `cp ./kernel_patches/fs/susfs.c $KERNEL_ROOT/fs/`
5. Run `cp ./kernel_patches/include/linux/susfs.h $KERNEL_ROOT/include/linux/`
6. Run `cd $KERNEL_ROOT/KernelSU` and then `patch -p1 < 10_enable_susfs_for_ksu.patch`
7. Run `cd $KERNEL_ROOT` and then `patch -p1 < 50_add_susfs_in_kernel.patch`, **if there are failed patches, you may try to patch them manually by yourself.**
8. Make sure again to have `CONFIG_KSU` and `CONFIG_KSU_SUSFS` enabled before building the kernel, some other SUSFS feature are disabled by default, you may turn it on via menuconfig or change it in ksu Kconfig or kernel defconfig.
9. Build and flash the kernel.

## Build ksu_susfs userspace tool ##
1. Run `./build_ksu_susfs_tool.sh` to build the userspace tool `ksu_susfs`, and the arm64 and arm binary will be copied to `ksu_module_susfs/tools/` as well.
2. Now you can also push the compiled `ksu_susfs` tool to `/data/adb/ksu/bin/` so that you can run it directly in adb root shell or termux root shell, as well as in your own ksu modules.

## Build sus_su ##
- sus_su is an executable aimed to get a root shell by sending a request to a susfs fifo driver.
- Only apps with root access granted by ksu manager are allow to run 'su'.
- For best compatibility, sus_su requires overlayfs to allow all other 3rd party apps to execute 'su' to get root shell.
- See `service.sh` in module templete for more details.

1. Make sure you have `CONFIG_KSU_SUSFS_SUS_SU` option enabled in Kconfig when building the kernel, it is disabled by default.
2. Run `./build_sus_su_tool.sh` to build the sus_su executable, the arm64 and arm binary will be copied to `ksu_module_susfs/tools/`.
3. Uncomment the line `#enable_sus_su` in service.sh to enable sus_su
4. Run `./build_ksu_module.sh` to build the module and flash again.

## Build ksu module ##
- The ksu module here is just a demo to show how to use it.
- It will also copy the `ksu_susfs` and `sus_su` tool to `/data/adb/ksu/bin/` as well when installing the module.

1. ksu_susfs tool can be run in any stage scripts, post-fs-data.sh, services.sh, boot-completed.sh according to your own need.
2. Run `./build_ksu_module.sh` to build the susfs KSU module.

## Usage of ksu_susfs ##
- Run ksu_susfs in root shell for detailed usages.

## Known Issues ##
- Version 1.4.0 can only apply on 4.9 kernel right now, other kernels need to be tested.
- mnt_id_reorder feature causes bootloop for non-gki devices, more test is needed.

## Credits ##
- KernelSU: https://github.com/tiann/KernelSU
- @Kartatz: for ideas and original commit from https://github.com/Dominium-Apum/kernel_xiaomi_chime/pull/1/commits/74f8d4ecacd343432bb8137b7e7fbe3fd9fef189

## Telegram ##
- @simonpunk
