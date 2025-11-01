#! /bin/bash

read -p "Insert all USB sticks. Make sure they have different labels that start with \"MASTERKEY\" and hit Enter." -s
echo

# Create a ramdisk.
RAMDISK=/tmp/ramdisk666
mkdir $RAMDISK
mount -t tmpfs -o size=1M,mode=700 ramdisk666 $RAMDISK

# Find all USB sticks.
disks=$(ls /dev/disk/by-label/MASTERKEY*)
echo -e "The following partitions were found:\n$disks"

# Mount all USB sticks.
shares="$*"     # Also write shares to files given on the command line.
exists=no
for f in $disks; do
  label=$(basename $f)
  nr=$(echo "$label" | sed -e 's/MASTERKEY//')
  mkdir $RAMDISK/$label
  mount -L $label $RAMDISK/$label
  if [ -e $RAMDISK/$label/key$nr ]; then
    echo "WARNING: $RAMDISK/$label/key$nr already exists!"
    exists=yes
  fi
  shares+=" $RAMDISK/$label/key$nr"
done

# Make sure the user wants to overwrite existing key files.
if [ "$exists" = "yes" ]; then
  echo -n "ARE YOU SURE? THIS ACTION WILL OVERWRITE THE EXISTING KEYS!!! (type yes in uppercase): "
  read ANSWER
  echo
  if [ "$ANSWER" != "YES" ]; then
    exit 1
  fi
fi

# Generate a new secret key.
echo "Generating new secret key of 64 bytes..."
dd if=/dev/random of=$RAMDISK/secret.key bs=64 count=1 status=none

# Split the key up into shares.
echo "shares = \"$shares\"."
sss_split $RAMDISK/secret.key $shares
if [ $? -eq 0 ]; then
  echo "Success"
fi

# Clean up.
for f in $disks; do
  umount $f
done
umount $RAMDISK
rmdir $RAMDISK
