#define _GNU_SOURCE

#include "hazmat.h"
#include "sss_common.h"

#include <sodium/core.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void
usage(const char *progname)
{
        fprintf(stderr, "Usage: %s OUTPUT INDEX SHARE...\n", progname);
}

static int
read_full(int fd, void *buf, size_t len)
{
        unsigned char *ptr = buf;
        size_t total = 0;

        while (total < len) {
                ssize_t res = read(fd, ptr + total, len - total);
                if (res == 0) {
                        return -1;
                }
                if (res < 0) {
                        if (errno == EINTR) {
                                continue;
                        }
                        return -1;
                }
                total += (size_t) res;
        }

        return 0;
}

int
main(int argc, char *argv[])
{
        if (argc < 4) {
                usage(argv[0]);
                return 2;
        }

        const char *output = argv[1];
        char *endptr = NULL;
        long index_long = strtol(argv[2], &endptr, 10);
        if (*argv[2] == '\0' || (endptr != NULL && *endptr != '\0') ||
            index_long <= 0 || index_long > 255) {
                fprintf(stderr, "Invalid share index: %s\n", argv[2]);
                return 2;
        }
        uint8_t share_index = (uint8_t) index_long;

        int num_shares = argc - 3;
        if (num_shares < 2) {
                fprintf(stderr, "Need at least two shares to recover a new one.\n");
                return 2;
        }

        if (sodium_init() == -1) {
                fprintf(stderr, "Failed to initialise libsodium.\n");
                return 1;
        }

        sss_Keyshare *share_sets = calloc((size_t) num_shares * MULTIPLICITY,
                                          sizeof(sss_Keyshare));
        if (share_sets == NULL) {
                fprintf(stderr, "Failed to allocate memory.\n");
                return 1;
        }

        for (int j = 0; j < num_shares; ++j) {
                const char *filename = argv[j + 3];
                int fd = open(filename, O_RDONLY | O_CLOEXEC);
                if (fd == -1) {
                        fprintf(stderr, "Failed to open \"%s\": %s\n", filename, strerror(errno));
                        free(share_sets);
                        return 1;
                }

                sss_Keyshare *layer0 = share_sets + j;
                if (read_full(fd, layer0[0], sizeof(sss_Keyshare)) != 0) {
                        fprintf(stderr, "Failed to read from \"%s\".\n", filename);
                        close(fd);
                        free(share_sets);
                        return 1;
                }

                for (int i = 1; i < MULTIPLICITY; ++i) {
                        sss_Keyshare *layer = share_sets + i * num_shares + j;
                        layer[0][0] = layer0[0][0];
                        if (read_full(fd, &layer[0][1], sss_KEYSHARE_LEN - 1) != 0) {
                                fprintf(stderr, "Failed to read from \"%s\".\n", filename);
                                close(fd);
                                free(share_sets);
                                return 1;
                        }
                }

                close(fd);
        }

        uint8_t recovered[MULTIPLICITY][sss_KEY_LEN];
        for (int i = 0; i < MULTIPLICITY; ++i) {
                const sss_Keyshare *layer = (const sss_Keyshare *)share_sets + i * num_shares;
                sss_evaluate_keyshares(recovered[i], layer, (uint8_t) num_shares, share_index);
        }

        free(share_sets);

        size_t total_len = 1 + (size_t) MULTIPLICITY * sss_KEY_LEN;
        unsigned char *buffer = malloc(total_len);
        if (buffer == NULL) {
                fprintf(stderr, "Failed to allocate output buffer.\n");
                return 1;
        }

        buffer[0] = share_index;
        unsigned char *ptr = buffer + 1;
        for (int i = 0; i < MULTIPLICITY; ++i) {
                memcpy(ptr, recovered[i], sss_KEY_LEN);
                ptr += sss_KEY_LEN;
        }

        int fd = open(output, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, S_IRUSR | S_IWUSR);
        if (fd == -1) {
                fprintf(stderr, "Failed to open \"%s\" for writing: %s\n", output, strerror(errno));
                free(buffer);
                return 1;
        }

        size_t written = 0;
        while (written < total_len) {
                ssize_t res = write(fd, buffer + written, total_len - written);
                if (res < 0) {
                        if (errno == EINTR) {
                                continue;
                        }
                        fprintf(stderr, "Failed to write to \"%s\": %s\n", output, strerror(errno));
                        close(fd);
                        free(buffer);
                        return 1;
                }
                written += (size_t) res;
        }

        if (close(fd) != 0) {
                fprintf(stderr, "Failed to close \"%s\": %s\n", output, strerror(errno));
                free(buffer);
                return 1;
        }

        free(buffer);
        return 0;
}
