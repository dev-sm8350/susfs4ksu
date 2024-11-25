## Introduction ##
- An addon root hiding kernel patches and userspace module for KernelSU.

- The userspace tool `ksu_susfs` and `sus_su`, as well as the ksu module, require a susfs patched kernel to work.

# Warning #
- This is only experimental code, that said it can harm your system or cause performance hit, **YOU ARE !! W A R N E D !!** already

## Compatibility ##
- The susfs kernel patches may differ for different kernel version or even on the same kernel version, you may need to create your own patches for your kernel.

## Patch Instruction (For GKI Kernel only and building from official google artifacts) ##
1. Make sure you follow the offical KSU guild here to clone and build the kernel with KSU: `https://kernelsu.org/guide/how-to-build.html`, the kernel root directory should be `$KERNEL_REPO/common`, you should run script to clone KSU in `$KERNEL_REPO`
2. Run `cp ./kernel_patches/KernelSU/10_enable_susfs_for_ksu.patch $KERNEL_REPO/KernelSU/`
3. Run `cp ./kernel_patches/50_add_susfs_in_kernel-<kernel_version>.patch $KERNEL_REPO/common/`
4. Run `cp ./kernel_patches/fs/susfs.c $KERNEL_REPO/common/fs/`
5. Run `cp ./kernel_patches/include/linux/susfs.h $KERNEL_REPO/common/include/linux/`
6. Run `cp ./kernel_patches/fs/sus_su.c $KERNEL_REPO/common/fs/`
7. Run `cp ./kernel_patches/include/linux/sus_su.h $KERNEL_REPO/common/include/linux/`
8. Run `cd $KERNEL_REPO/KernelSU` and then `patch -p1 < 10_enable_susfs_for_ksu.patch`
9. Run `cd $KERNEL_REPO/common` and then `patch -p1 < 50_add_susfs_in_kernel.patch`, **if there are failed patches, you may try to patch them manually by yourself.**
10. Make sure again to have `CONFIG_KSU` and `CONFIG_KSU_SUSFS` enabled before building the kernel, some other SUSFS feature may be disabled by default, you may enable/disable them via `menuconfig`, `kernel defconfig`, or change the `default [y|n]` option under each `config KSU_SUSFS_` option in `$KernelSU_repo/kernel/Kconfig` if you build with a new defconfig every time.
11. If your kernel already has the **KSU non-kprobe hook patches** applied, then you have to **`DISABLE`** the `CONFIG_KSU_SUSFS_SUS_SU` option.
12. For `gki kernel android14` or above, if you are building from google artifacts, it is necessary to delete the file `$KERNEL_REPO/common/android/abi_gki_protected_exports_aarch64` and `$KERNEL_REPO/common/android/abi_gki_protected_exports_x86_64`, otherwise some modules like WiFi will not work. Or you can just remove those files whenever those files exist in your kernel repo.
13. For gki kernel, when building from google artifacts, another thing you may need is to fix the `local spl_date` in function `build_gki_boot_images()` in `$KERNEL_REPO/build/kernel/build_utils.sh` to match the current boot security patch level of your phone.
14. Build and flash the kernel.
15. For some compilor error, please refer to the section **[Known Compilor Issues]** below.
16. For other building tips, please refer to the section **[Other Building Tips]** below.

## Patch Instruction (For non-GKI only) ##
1. Clone the repo with a tag that has release version, as tag with release version is more stable
2. Run `cp ./kernel_patches/KernelSU/10_enable_susfs_for_ksu.patch $KERNEL_ROOT/KernelSU/`
3. Run `cp ./kernel_patches/50_add_susfs_in_kernel-<kernel_version>.patch $KERNEL_ROOT/`
4. Run `cp ./kernel_patches/fs/susfs.c $KERNEL_ROOT/fs/`
5. Run `cp ./kernel_patches/include/linux/susfs.h $KERNEL_ROOT/include/linux/`
6. Run `cp ./kernel_patches/fs/sus_su.c $KERNEL_ROOT/fs/`
7. Run `cp ./kernel_patches/include/linux/sus_su.h $KERNEL_ROOT/include/linux/`
8. Run `cd $KERNEL_ROOT/KernelSU` and then `patch -p1 < 10_enable_susfs_for_ksu.patch`
9. Run `cd $KERNEL_ROOT` and then `patch -p1 < 50_add_susfs_in_kernel.patch`, **if there are failed patches, you may try to patch them manually by yourself.**
10. Make sure again to have `CONFIG_KSU` and `CONFIG_KSU_SUSFS` enabled before building the kernel, some other SUSFS feature may be disabled by default, you may enable/disable them via `menuconfig`, `kernel defconfig`, or change the `default [y|n]` option under each `config KSU_SUSFS_` option in `$KernelSU_repo/kernel/Kconfig` if you build with a new defconfig every time.
11. If your kernel already has the **KSU non-kprobe hook patches** applied, then you have to **`DISABLE`** the `CONFIG_KSU_SUSFS_SUS_SU` option.
12. For some compilor error, please refer to the section **[Known Compilor Issues]** below.
13. For other building tips, please refer to the section **[Other Building Tips]** below.

## Build ksu_susfs userspace tool ##
1. Run `./build_ksu_susfs_tool.sh` to build the userspace tool `ksu_susfs`, and the arm64 and arm binary will be copied to `ksu_module_susfs/tools/` as well.
2. Now you can also push the compiled `ksu_susfs` tool to `/data/adb/ksu/bin/` so that you can run it directly in adb root shell or termux root shell, as well as in your own ksu modules.

## Build sus_su userspace tool ##
**--Important Notes--**
- sus_su is now providing 2 modes, 1st mode requires sus_su userspace tool and overlayfs to work, 2nd mode requires no extra tools or mounts in userspace but literally just the same non-kprobe hooks su implementation for non-gki kernel.
- Newer xiaomi devices are found to have a root detection service running which is named "mrmd" and it is spawned by init process, and since sus_su mounted by overlayfs can't be umounted for process spawned by init process, so 1st mode will get detected unless there is a better umount scheme for init spawned process, that's why it is strongly suggested to use 2nd mode only.

**--Instruction for 1st mode--**
- sus_su userspace tool is an executable aimed to get a root shell by sending a request to a susfs fifo driver, this is exclusive for **"kprobe hook enabled KSU"** only, **DO NOT** use it if your KernelSU has kprobe **disabled**.
- Only apps with root access granted by ksu manager are allow to run 'su'.
- For best compatibility, sus_su requires overlayfs to allow all other 3rd party apps to execute 'su' to get root shell.
- See `service.sh` in module templete for more details.

1. Run `./build_sus_su_tool.sh` to build the sus_su executable, the arm64 and arm binary will be copied to `ksu_module_susfs/tools/`.
2. Uncomment the line `#enable_sus_su` in service.sh to enable sus_su
3. Run `./build_ksu_module.sh` to build the module and flash again.

**--Instruction for 2nd mode--**
- Just run `ksu_susfs sus_su 2` to disable core kprobe hooks and enable inline hooks su.


## Build susfs4ksu module ##
- The ksu module here is just a demo to show how to use it.
- It will also copy the `ksu_susfs` and `sus_su` tool to `/data/adb/ksu/bin/` as well when installing the module.

1. ksu_susfs tool can be run in any stage scripts, post-fs-data.sh, services.sh, boot-completed.sh according to your own need.
2. Run `./build_ksu_module.sh` to build the susfs KSU module.

## Usage of ksu_susfs and supported features ##
- Run `ksu_susfs` in root shell for detailed usages.
- See `$KernelSU_repo/kernel/Kconfig` for supported features after applying the susfs patches.

## Other Building Tips ##
1. To only remove the `-dirty` string from kernel release string, open file `$KERNEL_ROOT/scripts/setlocalversion`, then look for all the lines that containing `printf '%s' -dirty`, and replace it with `printf '%s' ''`
2. Alternatively, If you want to directly hardcode the whole kernel release string, then open file `$KERNEL_ROOT/scripts/setlocalversion`, look for the last line `echo "$res"`, and for example, replace it with `echo "-android13-01-gb123456789012-ab12345678"`
3. To hardcore your kernel version string, open `$KERNEL_ROOT/scripts/mkcompile_h`, and look for line `UTS_VERSION="$(echo $UTS_VERSION $CONFIG_FLAGS $TIMESTAMP | cut -b -$UTS_LEN)"`, then for example, replace it with `UTS_VERSION="#1 SMP PREEMPT Mon Jan 1 18:00:00 UTC 2024"`
4. To spoof the `/proc/config.gz` with the stock config, 

   - Make sure you are on the stock ROM and using stock kernel.
   - Use adb shell or root shell to pull your stock `/proc/config.gz` from your device to PC.
   - Decompress it using `gunzip` or whatever tools, then copy it to `$KERNEL_ROOT/arch/arm64/configs/stock_defconfig`
   - Open file `$KERNEL_ROOT/kernel/Makefile`.
   - Look for line `$(obj)/config_data: $(KCONFIG_CONFIG) FORCE`, and replace it with `$(obj)/config_data: arch/arm64/configs/stock_defconfig FORCE`

## Known Compiler Issues ##
1. error: use of undeclared identifier 'execve_kp', 'newfstatat_kp', etc..

   - Disable `CONFIG_KSU_SUSFS_SUS_SU` before compiling kernel.
2. error: no member named 'android_kabi_reservedx' in 'struct yyyyyyyy'

   - Because normally the memeber `u64 android_kabi_reservedx;` doesn't exist in all structs with all kernel version below 4.19, and sometimes it is not guaranteed existed with kernel version >= 4.19 and <= 5.4, and even with GKI kernel, like some of the custom kernels has all of them disabled. So at this point if the susfs patches didn't have them patched for you, then what you need to do is to manually append the member to the end of the corresponding struct definition, it should be `u64 android_kabi_reservedx;` with the last `x` starting from `1`, like `u64 android_kabi_reserved1;`, `u64 android_kabi_reserved2;` and so on. You may also refer to patch from other branches like `kernel-4.14`, `kernel-4.9` of this repo for extra `diff` of the missing kabi members.
 

## Other Known Issues ##
- You tell me, or kindly file an issue, or make a PR.

## Credits ##
- KernelSU: https://github.com/tiann/KernelSU
- @Kartatz: for ideas and original commit from https://github.com/Dominium-Apum/kernel_xiaomi_chime/pull/1/commits/74f8d4ecacd343432bb8137b7e7fbe3fd9fef189

## Telegram ##
- @simonpunk


## Buy me a coffee ##
- PayPal: kingjeffkimo@yahoo.com.tw
- BTC: bc1qgkwvsfln02463zpjf7z6tds8xnpeykggtgk4kw
