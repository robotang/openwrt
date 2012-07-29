#
# Environment setup for openwrt
#

export TOPDIR=~/dev/openwrt/trunk
export STAGING_DIR=${TOPDIR}/staging_dir/toolchain-mips_r2_gcc-4.6-linaro_uClibc-0.9.33
export PATH=${STAGING_DIR}/bin:${PATH}
export KERNELDIR=${TOPDIR}/build_dir/linux-ar71xx_generic/linux-3.2.5
export PACKAGEDIR=${TOPDIR}/bin/ar71xx/packages

unset CFLAGS CPPFLAGS CXXFLAGS LDFLAGS MACHINE

export ARCH=mips
export CROSS_COMPILE=mips-openwrt-linux-
export CC=${CROSS_COMPILE}gcc
export CXX=${CROSS_COMPILE}g++
export LD=${CROSS_COMPILE}ld
export RANLIB=${CROSS_COMPILE}ranlib
export STRIP=${CROSS_COMPILE}strip
export OBJCOPY=${CROSS_COMPILE}objcopy
export OBJDUMP=${CROSS_COMPILE}objdump
export SIZE=${CROSS_COMPILE}size

if [ "$PS1" ]; then
   if [ "$BASH" ]; then
     export PS1="\[\033[02;32m\]openwrt\[\033[00m\] ${PS1}"
   fi
fi

umask 0002

echo "Altered environment for cross compiling openwrt"
