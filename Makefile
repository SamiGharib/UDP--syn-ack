#Flags for GCC
CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -Wshadow
CFLAGS += -Wextra
CFLAGS += -O2 -D_FORTIFY_SOURCE=2
CFLAGS += -fstack-protector-all
CFLAGS += -D_POSIX_C_SOURCE=201112L -D_XOPEN_SOURCE


