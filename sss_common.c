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
#include <stdlib.h>

struct GlobalVars g;

void initialize(int argc, char* argv[])
{
  g.number_of_shares = argc - 2;
  g.threshold = g.number_of_shares - 1;

  g.secret_filename = argv[1];
  g.secret_file_exists = access(g.secret_filename, F_OK) != -1;

  /* This algorithm makes no sense with less than this number of shares. */
  if (g.number_of_shares < (g.secret_file_exists ? 3 : 2))
  {
    fprintf(stderr, "Not enough key shares.\n");
    exit(2);
  }

  /* Disable core dumps. */
  struct rlimit rl = { 0, 0 };
  int res = setrlimit(RLIMIT_CORE, &rl);
  assert(res == 0);

  /* Initialize libsodium. */
  res = sodium_init();
  assert(res == 0);

  /* Hazmat uses key sizes of sss_KEY_LEN bytes. The key shares are one byte longer. */
  assert(sizeof(sss_Keyshare) == sss_KEY_LEN + 1);
  /* We need m times the size of a hazmat key for our secret key. */
  assert(MULTIPLICITY * sizeof(sss_Keyshare) >= KEYLEN);

  /* Allocate and zero secure memory. */
  assert(sizeof(sss_Keyshare) % alignof(sss_Keyshare) == 0);    /* Make sure each element will be properly aligned. */
  g.total_memory_size = MULTIPLICITY * (g.number_of_shares + 1) * sizeof(sss_Keyshare);
  g.memory = (unsigned char*)sodium_malloc(g.total_memory_size);
  assert(g.memory != NULL);
  res = sodium_mlock(g.memory, g.total_memory_size);
  assert(res == 0);
  sodium_memzero(g.memory, g.total_memory_size);
  /* Allocate an aligned block for writing with O_DIRECT. */
  g.diskbuf = (unsigned char*)sodium_malloc(KEYLEN + 1);
  res = sodium_mlock(g.diskbuf, KEYLEN + 1);
  assert(res == 0);

  /* Variables for easy access. */
  g.secrets = (sss_Key*)g.memory;                                                                               /* m secret keys. */
  for (int i = 0; i < MULTIPLICITY; ++i)
    g.shares[i] = (sss_Keyshare*)(g.memory + (MULTIPLICITY + g.number_of_shares * i) * sizeof(sss_Keyshare));   /* g.number_of_shares shared keys. */

  g.ret = 0;
}

void deinitialize()
{
  /* Zero, unlock and free secure memory. */
  sodium_munlock(g.diskbuf, KEYLEN + 1);
  sodium_free(g.diskbuf);
  sodium_munlock(g.memory, g.total_memory_size);
  sodium_free(g.memory);

  if (g.ret == 0)
    printf("Successfully finished.\n");
  else
    fprintf(stderr, "Terminating with exit code %d.\n", g.ret);
}
