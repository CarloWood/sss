#! /bin/bash

# Create a ramdisk.
RAMDISK=/tmp/ramdisk_$$
if [ -e "$RAMDISK" ]; then
  # This should never happen, but if it does, then try hard to kill
  # any old ramdisk from a previous run.
  while umount "$RAMDISK" 2>/dev/null; do
    true
  done
  rm -rf "$RAMDISK"
fi
mkdir $RAMDISK
mount -t tmpfs -o size=1M,mode=700 ramdisk_$$ $RAMDISK

# Find all USB sticks.
disks=$(ls /dev/disk/by-label/MASTERKEY*)
echo -e "The following partitions were found:\n$disks" >&2

# Mount all USB sticks.
shares="$*"     # Also read shares from files given on the command line.
for f in $disks; do
  label=$(basename $f)
  nr=$(echo "$label" | sed -e 's/MASTERKEY//')
  mkdir $RAMDISK/$label
  mount -L $label $RAMDISK/$label
  if [ -e $RAMDISK/$label/key$nr ]; then
    shares+=" $RAMDISK/$label/key$nr"
  else
    echo "WARNING: $RAMDISK/$label/key$nr doesn't exist!" >&2
  fi
done

sss_combine $RAMDISK/secret.key $shares
RET=$?

# Clean up.
for f in $disks; do
  umount $f
done

if [ $RET -eq 0 ]; then
  echo "$RAMDISK/secret.key"
else
  umount $RAMDISK
  rmdir $RAMDISK
fi

exit $RET
