MYCC		= clang++
MYCC		= g++
#MYCC		= /usr/bin/../lib/clang/c++-analyzer

# ======================================================

EXTRA_CLEAN	:=
EXTRA_INCL	:=

# ======================================================

UNAME		= $(shell uname -s)

# ======================================================

CF_DEPS		= -MMD -MP
CF_INCL		= -Iinclude -Imyallocator $(EXTRA_INCL)
CF_OPTIM	= -O0 -g
#CF_LTO		= -flto
#CF_OPTIM	= -O3 -DNDEBUG
CF_OPTIM	+= -mavx -msse4.2 -maes -mpclmul
#CF_OPTIM	+= -fassociative-math -freciprocal-math -fno-signed-zeros
CF_WARN		= -Wall -Wextra -Wpedantic -Wdeprecated -Wconversion -Wsuggest-override -Wno-unknown-warning-option -Wno-stringop-truncation -fopt-info-vec-missed

#CF_MISC	= -DNOT_HAVE_CHARCONV

CF_ALL		= -std=c++17	\
			$(CF_LTO)	\
			$(CF_DEPS)	\
			$(CF_INCL)	\
			$(CF_OPTIM)	\
			$(CF_WARN)	\
			$(CF_MISC)

CXX		= $(MYCC) $(CF_ALL)

# ======================================================

LD_ALL		=
LL_ALL		= -lstdc++
LL_LTO		= -flto

LINK		= $(MYCC) $(LL_LTO) $(LD_ALL) -o $@ $^ $(LL_ALL)

STATIC_LINK	= YES
#STATIC_LINK	= NO

# https://stackoverflow.com/questions/9002264/starting-a-stdthread-with-static-linking-causes-segmentation-fault
#LINK		+= -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
# gcc 15 works without it
#LINK		+= -lpthread

# ======================================================

A		= bin/
O		= obj/

# ======================================================



ifeq ($(UNAME), Linux)

##### LINUX #####

$(info Linux detected				)
$(info  - epoll support...			)
$(info  - commit support...			)
$(info  - hugetlb support...			)
$(info						)

#CF_MISC	+= -DNOT_HAVE_CHARCONV

EXTRA_INCL	+= -Iinclude.linux/
CF_MISC		+= -DSELECTOR_EPOLL -DUSE_MAP_PAGES -DUSE_HUGETLB

ifeq ($(STATIC_LINK), YES)
LINK		+= -static
endif

# add epoll support...

LL_SELECTOR	 = $(O)epollselector.o



else ifeq ($(UNAME), FreeBSD)

##### FreeBSD #####

$(info FreeBSD detected				)
$(info  - kqueue support...			)
$(info						)



# add correct endian for FreeBSD
EXTRA_INCL	+= -Iinclude.freebsd/
CF_MISC		+= -D_GLIBCXX_USE_C99 -D_GLIBCXX_USE_C99_MATH -D_GLIBCXX_USE_C99_MATH_TR1

CF_MISC		+= -DNOT_HAVE_CHARCONV
LL_ALL		+= -lm

# add kqueue support...

CF_MISC		+= -DSELECTOR_KQUEUE
LL_SELECTOR	 = $(O)kqueueselector.o



else ifeq ($(UNAME), Darwin)

##### MAC OS #####

$(info MAC OS detected				)
$(info  - kqueue support...			)
$(info						)



EXTRA_INCL	+= -Iinclude.darwin/
CF_MISC		+= -DNOT_HAVE_CHARCONV -DSIZE_T_SEPARATE_FROM_UINT64_T

# add kqueue support...

CF_MISC		+= -DSELECTOR_KQUEUE
LL_SELECTOR	 = $(O)kqueueselector.o



else ifeq ($(UNAME), Haiku)

##### Haiku #####

$(info Haiku detected				)
$(info  - standard poll support... for now...	)
$(info  - hard rlimit_nofile			)
$(info						)



EXTRA_INCL	+= -Iinclude.haiku/

LL_ALL		+= -lnetwork

# add poll support...

CF_MISC		+= -DSELECTOR_POLL -DHARD_RLIMIT_NO_FILES=512
LL_SELECTOR	 = $(O)pollselector.o



else

##### Posix OS #####

$(info fallback to standard Posix OS		)
$(info  - standard poll support...		)
$(info						)



# add poll support...

CF_MISC		+= -DSELECTOR_POLL
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

