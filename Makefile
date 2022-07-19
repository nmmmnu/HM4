MYCC		= clang++
#MYCC		= g++
#MYCC		= /usr/bin/../lib/clang/c++-analyzer

# ======================================================

EXTRA_CLEAN	:=
EXTRA_INCL	:=

# ======================================================

UNAME		= $(shell uname -s)

# ======================================================

CF_DEPS		= -MMD -MP
CF_INCL		= -Iinclude $(EXTRA_INCL)
CF_OPTIM	= -O3 -g
#CF_OPTIM	= -O3 -DNDEBUG
CF_OPTIM	+= -mavx -msse4.2 -maes -mpclmul
CF_WARN		= -Wall -Wextra -Wpedantic -Wdeprecated -Wconversion -Wno-unknown-warning-option -Wno-stringop-truncation

#CF_MISC	= -DNOT_HAVE_CHARCONV

CF_ALL		= -std=c++17	\
			$(CF_DEPS)	\
			$(CF_INCL)	\
			$(CF_OPTIM)	\
			$(CF_WARN)	\
			$(CF_MISC)

CXX		= $(MYCC) $(CF_ALL)

# ======================================================

LD_ALL		=
LL_ALL		= -lstdc++

LINK		= $(MYCC) $(LD_ALL) -o $@ $^ $(LL_ALL)

# https://stackoverflow.com/questions/9002264/starting-a-stdthread-with-static-linking-causes-segmentation-fault
LINK		+= -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
#LINK		+= -lpthread

# ======================================================

A		= bin/
O		= obj/

# ======================================================

ifeq ($(UNAME), Linux)

##### LINUX #####

#CF_MISC	+= -DNOT_HAVE_CHARCONV

# add epoll support...

EXTRA_INCL	+= -Iinclude.linux/
CF_MISC		+= -DSELECTOR_EPOOL
LL_SELECTOR	 = $(O)epollselector.o

else ifeq ($(UNAME), FreeBSD)

##### FreeBSD #####

# add correct endian for FreeBSD
# fix compilation for FreeBSD
EXTRA_INCL	+= -Iinclude.freebsd/
CF_MISC		+= -D_GLIBCXX_USE_C99 -D_GLIBCXX_USE_C99_MATH -D_GLIBCXX_USE_C99_MATH_TR1
CF_MISC		+= -DNOT_HAVE_CHARCONV
LL_ALL		+= -lm

# add kqueue support...

CF_MISC		+= -DSELECTOR_KQUEUE
LL_SELECTOR	 = $(O)kqueueselector.o

else ifeq ($(UNAME), Darwin)

##### MAC OS #####

EXTRA_INCL	+= -Iinclude.darwin/
CF_MISC		+= -DNOT_HAVE_CHARCONV

# add kqueue support...

CF_MISC		+= -DSELECTOR_KQUEUE
LL_SELECTOR	 = $(O)kqueueselector.o

else

# add poll support...

CF_MISC		+= -DSELECTOR_POOL
LL_SELECTOR	 = $(O)pollselector.o

endif

# ======================================================

include Makefile.dirlist

include $(addsuffix /Makefile.dir, $(SUBDIRS))

-include $(wildcard $(O)*.d)

all:

clean:
	rm -f \
		$(O)*.o		\
		$(O)*.d		\
		$(EXTRA_CLEAN)

.PHONY: all clean

