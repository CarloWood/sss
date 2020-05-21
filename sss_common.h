#pragma once

#define KEYLEN 64               /* Our secret key length. */
#define sss_KEY_LEN 32          /* Given by hazmat implementation. */

/* Number of hazmat keys required per secret key. */
#define MULTIPLICITY ((KEYLEN + sss_KEY_LEN - 1) / sss_KEY_LEN)

/*
 * One cryptographic key which that is shared using Shamir's the `sss_create_keyshares` function.
 */
typedef uint8_t sss_Key[32];

/*
 * Global variables.
 */
struct GlobalVars {
  int number_of_shares;
  int threshold;
  char const* secret_filename;
  int secret_file_exists;
  int total_memory_size;
  unsigned char* memory;
  unsigned char* diskbuf;
  sss_Key* secrets;
  sss_Keyshare* shares[MULTIPLICITY];
  int ret;
  int silent;
};

extern struct GlobalVars g;

void initialize(int argc, char* argv[]);
void deinitialize(void);

int sss_open_read(char const* filename);
int sss_open_write(char const* filename);
