#!/bin/sh

export TEGRA_BASE=/home/d/64_TX2/Linux_for_Tegra
export CROSS_COMPILE=$TEGRA_BASE/compiler/bin/aarch64-unknown-linux-gnu-
echo $CROSS_COMPILE

export TEGRA_KERNEL_OUT=$TEGRA_BASE/out/images
echo $TEGRA_KERNEL_OUT

export TEGRA_MODULES_OUT=$TEGRA_BASE/out/modules
echo $TEGRA_MODULES_OUT


mkdir -p $TEGRA_KERNEL_OUT
mkdir -p $TEGRA_MODULES_OUT
export ARCH=arm64

#Insert the hardware number here:
export LOCALVERSION=-HW4.2

export TEGRA_X2_ROOTFS=$TEGRA_BASE/rootfs

# If this is the first time, first do
#Clean kernel config
#make -C kernel/kernel-4.4/ mrproper
# make O=$TEGRA_KERNEL_OUT ARCH=$ARCH tegra18_defconfig
# and then edit the file $TEGRA_BASE/kernel/.config to enable drivers
# you need.

make -j 6 -C kernel/kernel-4.4/ O=$TEGRA_KERNEL_OUT ARCH=$ARCH zImage
make -j 6 -C kernel/kernel-4.4/ O=$TEGRA_KERNEL_OUT ARCH=$ARCH dtbs
make -j 6 -C kernel/kernel-4.4/ O=$TEGRA_KERNEL_OUT ARCH=$ARCH modules
make -C kernel/kernel-4.4/ O=$TEGRA_KERNEL_OUT ARCH=$ARCH modules_install INSTALL_MOD_PATH=$TEGRA_BASE/rootfs


cd $TEGRA_BASE/rootfs
tar cvfj $TEGRA_BASE/kernel/fresh-modules.tbz lib/modules
cd $TEGRA_BASE
cp kernel/arch/arm64/boot/dts/*.dtb kernel/dtb/
./apply_binaries.sh
