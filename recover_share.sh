#! /bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
RAMDISK=/tmp/ramdisk_$$
MOUNT_POINTS=()
SHARES=()
SHARE_INDEXES=()
BLANK_POINTS=()
BLANK_INDEXES=()

cleanup() {
  local status=$?
  for (( idx=${#MOUNT_POINTS[@]}-1; idx>=0; idx-- )); do
    local mp=${MOUNT_POINTS[$idx]}
    if mountpoint -q "$mp"; then
      umount "$mp" || true
    fi
  done
  if mountpoint -q "$RAMDISK"; then
    umount "$RAMDISK" || true
  fi
  if [ -d "$RAMDISK" ]; then
    rmdir "$RAMDISK" || true
  fi
  exit $status
}

trap cleanup EXIT

if [ -e "$RAMDISK" ]; then
  while umount "$RAMDISK" 2>/dev/null; do
    true
  done
  rm -rf "$RAMDISK"
fi
mkdir "$RAMDISK"
mount -t tmpfs -o size=1M,mode=700 "ramdisk_$$" "$RAMDISK"

mapfile -t disks < <(ls /dev/disk/by-label/MASTERKEY* 2>/dev/null || true)
if [ ${#disks[@]} -eq 0 ]; then
  echo "ERROR: No MASTERKEY USB sticks found." >&2
  exit 1
fi

echo -e "The following partitions were found:\n${disks[*]}" >&2

for dev in "${disks[@]}"; do
  label=$(basename "$dev")
  nr=${label#MASTERKEY}
  if [[ ! $nr =~ ^[0-9]+$ ]]; then
    echo "WARNING: Skipping $label because it does not end with a numeric index." >&2
    continue
  fi
  mount_point="$RAMDISK/$label"
  mkdir "$mount_point"
  mount -L "$label" "$mount_point"
  MOUNT_POINTS+=("$mount_point")

  key_path="$mount_point/key$nr"
  if compgen -G "$mount_point/key*" > /dev/null; then
    if [ -e "$key_path" ]; then
      SHARES+=("$key_path")
      SHARE_INDEXES+=("$nr")
    else
      echo "WARNING: $mount_point contains existing key files but not key$nr; skipping for output." >&2
    fi
  else
    BLANK_POINTS+=("$mount_point")
    BLANK_INDEXES+=("$nr")
  fi

done

needed=${#SHARES[@]}
if [ "$needed" -lt 2 ]; then
  echo "ERROR: Not enough shares found (found $needed)." >&2
  exit 1
fi

if [ ${#BLANK_POINTS[@]} -eq 0 ]; then
  echo "ERROR: Could not find an unused USB stick." >&2
  exit 1
fi

echo "Using $needed existing share(s) to recover new ones." >&2

for ((i=0; i<${#BLANK_POINTS[@]}; i++)); do
  mount_point=${BLANK_POINTS[$i]}
  index=${BLANK_INDEXES[$i]}
  output="$mount_point/key$index"

  for existing in "${SHARE_INDEXES[@]}"; do
    if [ "$existing" = "$index" ]; then
      echo "WARNING: key$index already exists on another USB stick; writing duplicate." >&2
      break
    fi
  done

  tmpfile=$(mktemp "$RAMDISK/recovered.$index.XXXXXX")
  "$SCRIPT_DIR/sss_recover" "$tmpfile" "$index" "${SHARES[@]}"
  mv "$tmpfile" "$output"
  sync "$output" 2>/dev/null || true
  echo "Wrote recovered share to $output" >&2
  SHARE_INDEXES+=("$index")
  SHARES+=("$output")

done

trap - EXIT
cleanup
