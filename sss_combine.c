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
 * Usage: ./sss_combine secret.key key1.txt key2.txt ...
 */
int main(int argc, char* argv[])
{
  initialize(argc, argv);
  if (!g.silent)
    printf("Reconstructing \"%s\" from %d secret shares...\n", g.secret_filename, g.number_of_shares);

  do
  {
    /* Read the shares from disk. */
    for (int j = 0; j < g.number_of_shares; ++j)
    {
      char const* filename = argv[j + 2];
      if (!g.silent)
        printf("Reading \"%s\"...\n", filename);
      int fd = sss_open_read(filename);
      if (fd == -1)
        break;
      for (int i = 0; i < MULTIPLICITY; ++i)
      {
        int offset = i == 0 ? 0 : 1;
        if (offset == 1)
          g.shares[i][j][0] = g.shares[0][j][0];
        int len = sizeof(sss_Keyshare) - offset;
        int res = read(fd, g.shares[i][j] + offset, len);
        if (res != len)
        {
          fprintf(stderr, "Failed to read %d bytes from \"%s\": %s\n", len, filename, strerror(errno));
          g.ret = 1;
          break;
        }
      }
      close(fd);
      if (g.ret != 0)
        break;
    }
    if (g.ret != 0)
      break;

    /* Combine the secret shares. */
    for (int i = 0; i < MULTIPLICITY; ++i)
      sss_combine_keyshares(g.secrets[i], (sss_Keyshare const*)g.shares[i], g.number_of_shares);

    if (g.secret_file_exists)
    {
      /* Read existing secret key into memory. */
      int fd = sss_open_read(g.secret_filename);
      if (fd == -1)
        break;
      int res = read(fd, g.diskbuf, KEYLEN);
      if (res != KEYLEN)
      {
        fprintf(stderr, "Failed to read \"%s\": %s\n", g.secret_filename, strerror(errno));
        g.ret = 1;
        break;
      }
      close(fd);
      if (memcmp(g.diskbuf, g.memory, KEYLEN) != 0)
      {
        if (!g.silent)
          printf("No match\n");
        g.ret = -1;
        break;
      }
      if (!g.silent)
        printf("Match\n");
    }
    else
    {
      /* Write the secret key to memory. */
      int fd = sss_open_write(g.secret_filename);
      if (fd == -1)
        break;
      int res = write(fd, g.memory, KEYLEN);
      if (res != KEYLEN)
      {
        if (res == -1)
          fprintf(stderr, "Failed to write to \"%s\": %s\n", g.secret_filename, strerror(errno));
        else
          fprintf(stderr, "Failed to write %d bytes to \"%s\".\n", KEYLEN, g.secret_filename);
        g.ret = 3;
        break;
      }
      close(fd);
    }
  }
  while (0);

  deinitialize();
  return g.ret;
}
