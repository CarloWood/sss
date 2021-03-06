# Shamir secret sharing library

See upstream repository for more information:
https://github.com/dsprenkels/sss

This fork adds the following:

### `generate_shares.sh`
A wrapper script around `sss_split` to generate a *new* 512 bits secret key,
and split it over three or more USB sticks.

Usage:

Prepare `N` USB sticks (N >= 3) each with one partition whose
label starts with "MASTERKEY" (in all caps). Each label must be
unique; for example, you could use "MASTERKEY1", "MASTERKEY2",
etc. The labels should show up in `/dev/disk/by-label`.

Running `sudo ./generate_shares.sh` will write `N` files,
of 65 bytes each, to each partition.

At least `N - 1` of the USB sticks need to be present in
order to recreate the secret key (see below). Because any
of the keys can be left out, you can lose one key and still
be able to decrypt a disk that was encrypted with the secret
key. However, it will not be possible (with this software)
to regenerate the lost key. Instead, just create N new keys.

It is advisable to do this anyway (using `sss_split`) once
every two years or so, in order to refresh the USB sticks.

### `combine_shares.sh`
A wrapper script around `sss_combine` that recreates the secret key.

Usage:

Insert at least `N - 1` of the USB sticks in your PC, then run
`combine_shares.sh`. This will print to stdout a filename that will
contain the secret key. As soon as you are done with the secret
key you must run `umount $(dirname $FILE)`.

Optionally, an USB stick may be replaced with a filename of one of
the shared keys as commandline parameter. It may *not* also be present
as file in a `MASTERKEY` partition in that case (reducing the
number of USB sticks further from `N - 1`).

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
It is called by `combine_shares.sh`.

Usage:

Have N-1 or N shared secrets available (e.g. /tmp/ramdisk/key1,
/tmp/ramdisk/key2 etc). And run:
`sss_combine /tmp/ramdisk/secret.key <key1> <key2> ...`
which then will restore the secret.key.
