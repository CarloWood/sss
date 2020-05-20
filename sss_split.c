#define _GNU_SOURCE

#include "hazmat.h"
#include "sss_common.h"
#include <sodium/core.h>
#include <sodium/utils.h>
#include <string.h>
#include <assert.h>
#include <stdalign.h>
#include <stdio.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/*
 * Usage: ./sss_split secret.key key1.txt key2.txt ...
 */
int main(int argc, char* argv[])
{
  initialize(argc, argv);
  printf("Reading \"%s\" and splitting it into %d secret shares with a threshold of %d.\n", g.secret_filename, g.number_of_shares, g.threshold);

  do
  {
    /* Read the secret key into memory. */
    int fd = open(g.secret_filename, O_RDONLY | O_CLOEXEC | O_DIRECT);
    if (fd == -1)
    {
      fd = open(g.secret_filename, O_RDONLY);
      if (fd == -1)
      {
        fprintf(stderr, "1. Failed to open \"%s\": %s\n", g.secret_filename, strerror(errno));
        g.ret = 1;
        break;
      }
    }
    int res = read(fd, g.memory, KEYLEN);
    if (res != KEYLEN)
    {
      if (res == -1)
        fprintf(stderr, "Failed to read \"%s\": %s\n", g.secret_filename, strerror(errno));
      else
        fprintf(stderr, "Failed to read %d bytes from \"%s\".\n", KEYLEN, g.secret_filename);
      g.ret = 1;
      break;
    }
    close(fd);

    /* Generate the secret shares. */
    for (int i = 0; i < MULTIPLICITY; ++i)
      sss_create_keyshares(g.shares[i], g.secrets[i], g.number_of_shares, g.threshold);

    /* Write the shares to disk. */
    for (int j = 0; j < g.number_of_shares; ++j)
    {
      char const* filename = argv[j + 2];
      printf("Writing \"%s\"...\n", filename);
      fd = open(filename, O_WRONLY | O_CLOEXEC | O_CREAT | O_DIRECT | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
      if (fd == -1)
      {
        fprintf(stderr, "2. Failed to open \"%s\": %s\n", filename, strerror(errno));
        g.ret = 1;
        continue;
      }
      unsigned char* buf = g.diskbuf;
      for (int i = 0; i < MULTIPLICITY; ++i)
      {
        int offset = i == 0 ? 0 : 1;
        int len = sizeof(sss_Keyshare) - offset;
        memcpy(buf, g.shares[i][j] + offset, len);
        buf += len;
      }
      res = write(fd, g.diskbuf, buf - g.diskbuf);
      if (res != buf - g.diskbuf)
      {
        fprintf(stderr, "Failed to write (in full) to \"%s\": %s\n", filename, strerror(errno));
        g.ret = 1;
      }
      close(fd);
    }
  }
  while (0);

  deinitialize();
  return g.ret;
}
