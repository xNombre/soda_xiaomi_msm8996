# AnyKernel2 Ramdisk Mod Script
# osm0sis @ xda-developers

## AnyKernel setup
# begin properties
properties() { '
kernel.string=Flashing: The Soda Kernel...
do.devicecheck=1
do.modules=0
do.cleanup=1
do.cleanuponabort=0
device.name1=dummy-device
supported.versions=8 - 9
'; } # end properties

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

ui_print " ";
ui_print "== WARNING ==";
ui_print "This is EAS kernel!";
ui_print "If you flash it on HMP rom you warranty is void...";
ui_print "...and don't even try to report any issues";
ui_print "=============";
ui_print " ";

## AnyKernel install
dump_boot;

# begin ramdisk changes

ui_print "Patch ramdisk...";

# Restore stock ramdisks
restore_file /vendor/etc/init/hw/init.qcom.rc;
restore_file /vendor/etc/init/hw/init.qcom.power.rc;
backup_file /vendor/etc/init/hw/init.qcom.rc;
backup_file /vendor/etc/init/hw/init.qcom.power.rc;

# Remove zram
  remove_line /vendor/etc/fstab.qcom "/dev/block/zram0";

# Tweak flags
  patch_fstab /vendor/etc/fstab.qcom /data ext4 options "nosuid,nodev,noatime,barrier=1,noauto_da_alloc" "nosuid,nodev,noatime,barrier=1,noauto_da_alloc,nodiratime,discard";
  patch_fstab /vendor/etc/fstab.qcom /data f2fs options "nosuid,nodev,noatime,data_flush" "nosuid,nodev,noatime,data_flush,nodiratime,discard";

# Remove qcom tweaks
  remove_line /vendor/etc/init/hw/init.qcom.rc "write /sys/block/zram0/comp_algorithm lz4";
  remove_line /vendor/etc/init/hw/init.qcom.rc "write /sys/block/sda/queue/iostats 0";
  remove_line /vendor/etc/init/hw/init.qcom.rc "write /sys/block/sda/queue/scheduler cfq";
  remove_line /vendor/etc/init/hw/init.qcom.rc "write /sys/block/sda/queue/iosched/slice_idle 0";
  remove_line /vendor/etc/init/hw/init.qcom.rc "write /sys/block/sda/queue/read_ahead_kb 2048";
  remove_line /vendor/etc/init/hw/init.qcom.rc "write /sys/block/sda/queue/nr_requests 256";
  remove_line /vendor/etc/init/hw/init.qcom.rc "swapon_all /vendor/etc/fstab.qcom";
  remove_line /vendor/etc/init/hw/init.qcom.rc "write /sys/block/sda/queue/read_ahead_kb 128";
  remove_line /vendor/etc/init/hw/init.qcom.rc "write /sys/block/sda/queue/nr_requests 128";
  remove_line /vendor/etc/init/hw/init.qcom.rc "write /sys/block/sda/queue/iostats 1";

ui_print "Inject soda tweaks...";

# Inject soda script
  insert_file /vendor/etc/init/hw/init.qcom.power.rc "on apply-soda" before "on enable-low-power" soda.rc;
  remove_line /vendor/etc/init/hw/init.qcom.power.rc "Set I/O";
  replace_line /vendor/etc/init/hw/init.qcom.power.rc "setprop sys.io.scheduler" "    trigger apply-soda";

ui_print "Patched!";

# end ramdisk changes

write_boot;

## end install

