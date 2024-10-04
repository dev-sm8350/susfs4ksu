## Introduction ##
- An addon root hiding kernel patches and userspace module for KernelSU.

- The userspace tool `ksu_susfs` and 'sus_su', as well as the ksu module, require a susfs patched kernel to work.

# Warning #
- This is only experimental code, that said it can harm your system or cause performance hit, **YOU ARE !! W A R N E D !!** already

## Compatibility ##
- The susfs kernel patches may differ for different kernel version or even on the same kernel version, you may need to create your own patches for your kernel.

## Patch Instruction (For GKI only and building from official google artifacts) ##
1. Make sure you follow the offical KSU guild here to clone and build the kernel with KSU: `https://kernelsu.org/guide/how-to-build.html`, the kernel root directory should be `$KERNEL_REPO/common`, you should run script to clone KSU in `$KERNEL_REPO`
2. Run `cp ./kernel_patches/KernelSU/10_enable_susfs_for_ksu.patch $KERNEL_REPO/KernelSU/`
3. Run `cp ./kernel_patches/KernelSU/kernel/sucompat.h $KERNEL_REPO/KernelSU/kernel/`
4. Run `cp ./kernel_patches/50_add_susfs_in_kernel-<kernel_version>.patch $KERNEL_REPO/common/`
5. Run `cp ./kernel_patches/fs/susfs.c $KERNEL_REPO/common/fs/`
6. Run `cp ./kernel_patches/include/linux/susfs.h $KERNEL_REPO/common/include/linux/`
7. Run `cp ./kernel_patches/fs/sus_su.c $KERNEL_REPO/common/fs/`
8. Run `cp ./kernel_patches/include/linux/sus_su.h $KERNEL_REPO/common/include/linux/`
9. Run `cd $KERNEL_REPO/KernelSU` and then `patch -p1 < 10_enable_susfs_for_ksu.patch`
10. Run `cd $KERNEL_REPO/common` and then `patch -p1 < 50_add_susfs_in_kernel.patch`, **if there are failed patches, you may try to patch them manually by yourself.**
11. Make sure again to have `CONFIG_KSU` and `CONFIG_KSU_SUSFS` enabled before building the kernel, some other SUSFS feature are disabled by default, you may turn it on via menuconfig or change it in ksu Kconfig or kernel defconfig.
12. For gki kernel android14 or above, if you are building from google artifacts, it is necessary to remove `$KERNEL_REPO/common/android/abi_gki_protected_exports_aarch64` and `$KERNEL_REPO/common/android/abi_gki_protected_exports_x86_64`, otherwise some modules like WiFi will not work. Or you can just remove those files whenever those files exist in your kernel repo.
13. For gki kernel, when building from google artifacts, another thing you may need is to fix the `local spl_date` in function `build_gki_boot_images()` in `$KERNEL_REPO/build/kernel/build_utils.sh` to match the current boot security patch level of your phone.
14. Build and flash the kernel.

## Patch Instruction (For non-GKI only) ##
1. Clone the repo with a tag that has release version, as tag with release version is more stable
2. Run `cp ./kernel_patches/KernelSU/10_enable_susfs_for_ksu.patch $KERNEL_ROOT/KernelSU/`
3. Run `cp ./kernel_patches/KernelSU/kernel/sucompat.h $KERNEL_ROOT/KernelSU/kernel/`
4. Run `cp ./kernel_patches/50_add_susfs_in_kernel-<kernel_version>.patch $KERNEL_ROOT/`
5. Run `cp ./kernel_patches/fs/susfs.c $KERNEL_ROOT/fs/`
6. Run `cp ./kernel_patches/include/linux/susfs.h $KERNEL_ROOT/include/linux/`
7. Run `cp ./kernel_patches/fs/sus_su.c $KERNEL_ROOT/fs/`
8. Run `cp ./kernel_patches/include/linux/sus_su.h $KERNEL_ROOT/include/linux/`
9. Run `cd $KERNEL_ROOT/KernelSU` and then `patch -p1 < 10_enable_susfs_for_ksu.patch`
10. Run `cd $KERNEL_ROOT` and then `patch -p1 < 50_add_susfs_in_kernel.patch`, **if there are failed patches, you may try to patch them manually by yourself.**
11. Make sure again to have `CONFIG_KSU` and `CONFIG_KSU_SUSFS` enabled before building the kernel, some other SUSFS feature are disabled by default, you may turn it on via menuconfig or change it in ksu Kconfig or kernel defconfig.
12. Build and flash the kernel.

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
- You tell me, or kindly file an issue, or make a PR.

## Credits ##
- KernelSU: https://github.com/tiann/KernelSU
- @Kartatz: for ideas and original commit from https://github.com/Dominium-Apum/kernel_xiaomi_chime/pull/1/commits/74f8d4ecacd343432bb8137b7e7fbe3fd9fef189

## Telegram ##
- @simonpunk


## Buy me a coffee ##
- PayPal: kingjeffkimo@yahoo.com.tw
- BTC: bc1qgkwvsfln02463zpjf7z6tds8xnpeykggtgk4kw
