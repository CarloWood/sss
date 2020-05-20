# Shamir secret sharing library

See upstream repository for more information:
https://github.com/dsprenkels/sss

This fork adds the following:

### `generate_shares.sh`
A script to generate a new secret key of 512 bits,split over several USB sticks.

  Usage:

  Prepare N USB sticks (minimal three) each with one partition whose
  label starts with "MASTERKEY" (in all caps). Each label must be
  unique however, so you could for example use "MASTERKEY1", "MASTERKEY2",
  etc. The labels should show up in /dev/disk/by-label.

  Run: `sudo ./generate_shares.sh` and follow the instructions.

  After this only N-1 of the USB sticks need to be present in
  order to recreate the secret key (using `sss_combine`).

### `sss_split`
This utility is installed in `/usr/local/sbin` upon install (`sudo make install`).
It is called by `generate_shares.sh`.

  Usage:

  Prepare a file with 64 cryptographically random bytes. Lets
  say this file is called `/tmp/ramdisk/secret.key`. Then run:
  `sss_split /tmp/ramdisk/secret.key file1 file2 file3 ...`
  which will split the secret over as many shared secrets as
  are given on the command line.

### `sss_combine`
This utility is installed in `/usr/local/sbin` upon install (`sudo make install`).

  Usage:

  Have N-1 or N shared secrets available (e.g. /tmp/ramdisk/key1,
  /tmp/ramdisk/key2 etc). And run:
  `sss_combine /tmp/ramdisk/secret.key <key1> <key2> ...`
  which then will restore the secret.key.
