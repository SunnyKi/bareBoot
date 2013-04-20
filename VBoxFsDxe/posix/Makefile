VSRC	= ../VBoxFs

SRCS	= lslr.c \
		fsw_posix.c \
		${VSRC}/fsw_core.c \
		${VSRC}/fsw_lib.c \
		${VSRC}/fsw_hfs.c

CFLAGS	= -g -Wall -I. -I${VSRC} -DHOST_POSIX -DFSTYPE=hfs -DFSW_DEBUG_LEVEL=3

hfstest:	${SRCS}
	${CC} ${CFLAGS} ${SRCS}
