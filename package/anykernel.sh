# AnyKernel2 Ramdisk Mod Script
# osm0sis @ xda-developers

## AnyKernel setup
# begin properties
properties() {
kernel.string=Flashing: The Soda Kernel
do.devicecheck=1
do.modules=0
do.cleanup=1
do.cleanuponabort=0
device.name1=dummy-device
} # end properties

# shell variables
block=/dev/block/bootdevice/by-name/boot;
is_slot_device=0;
ramdisk_compression=auto;


## AnyKernel methods (DO NOT CHANGE)
# import patching functions/variables - see for reference
. /tmp/anykernel/tools/ak2-core.sh;


## AnyKernel file attributes
# set permissions/ownership for included ramdisk files
chmod -R 750 $ramdisk/*;
chown -R root:root $ramdisk/*;


## AnyKernel install
dump_boot;

write_boot;

#### init ####
ui_print "Patch ramdisks..."

# Remove zram
  remove_line /vendor/etc/fstab.qcom "/dev/block/zram0"
  remove_line /vendor/etc/init/hw/init.qcom.rc "write /sys/block/zram0/comp_algorithm"
  remove_line /vendor/etc/init/hw/init.qcom.rc "swapon_all /vendor/etc/fstab.qcom"

# Tweak flags
  patch_fstab /vendor/etc/fstab.qcom /data ext4 options "nosuid,nodev,noatime,barrier=1,noauto_da_alloc" "nosuid,nodev,noatime,barrier=1,noauto_da_alloc,nodiratime,discard"
  patch_fstab /vendor/etc/fstab.qcom /data f2fs options "nosuid,nodev,noatime,data_flush" "nosuid,nodev,noatime,data_flush,nodiratime,discard"

ui_print "All succeeded!"

## end install

