DIRS := aop bop lop cop cond syscall mem general world

.PHONY: $(DIRS) clean

default: all

%.aop:
	$(MAKE) -C aop $*

%.bop:
	$(MAKE) -C bop $*

%.lop:
	$(MAKE) -C lop $*

%.cop:
	$(MAKE) -C cop $*

%.cond:
	$(MAKE) -C cond $*

%.mem:
	$(MAKE) -C mem $*

%.general:
	$(MAKE) -C general $*

%.world:
	$(MAKE) -C world $*

%.syscall:
	$(MAKE) -C syscall $*

all: $(addprefix all.,$(DIRS))
clean: $(addprefix clean.,$(DIRS))
