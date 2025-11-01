CFLAGS += -g -O2 -m64 -std=c99 -pedantic \
	-Wall -Wshadow -Wpointer-arith -Wcast-qual -Wformat -Wformat-security \
	-Werror=format-security -Wstrict-prototypes -Wmissing-prototypes \
	-D_FORTIFY_SOURCE=2 -fPIC -fno-strict-overflow
SRCS = hazmat.c sss.c tweetnacl.c
OBJS := ${SRCS:.c=.o}
LIBS := -lsodium
UNAME_S := $(shell uname -s)

all: libsss.a

libsss.a: $(OBJS)
    ifeq ($(UNAME_S),Linux)
		$(AR) -rcs libsss.a $^
    endif
    ifeq ($(UNAME_S),Darwin)
		libtool -static -o libsss.a $^
    endif

# Force unrolling loops on hazmat.c
hazmat.o: CFLAGS += -funroll-loops

%.out: %.o
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(MEMCHECK) ./$@

test_hazmat.out: $(OBJS)
test_sss.out: $(OBJS)

.PHONY: check
check: test_hazmat.out test_sss.out

.PHONY: clean
clean:
	$(RM) *.o *.gch *.a *.out
