obj-m+=dma.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

modules_install: all
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install
	$(DEPMOD)

