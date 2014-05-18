obj-m += lvwnet_node.o 
obj-m += lvwnet_controller.o 
all:
#		make -C /usr/src/kernels/3.11.10-301.fc20.x86_64 M=$(PWD) modules ARCH=x86
#		make -C /usr/src/kernels/3.12.5-302.fc20.x86_64 M=$(PWD) modules ARCH=x86
#		make -C /usr/src/kernels/3.12.6-300.fc20.x86_64 M=$(PWD) modules ARCH=x86
#		make -C /usr/src/kernels/`uname -r` M=$(PWD) modules ARCH=x86
#		make -C /lib/modules/`uname -r`/build M=$(PWD) modules  
		make -C /lib/modules/3.13.9-200.fc20.x86_64/build M=$(PWD) modules  
#		make -C /home/lodantas/Dropbox/master/kernel/linux-3.12.5 M=$(PWD) modules ARCH=x86

#new:
#		make -C /usr/src/kernels/3.12.5-302.fc20.x86_64 M=$(PWD) modules ARCH=x86

clean:
		#make -C /home/lodantas/master/kernel/linux-3.10.2 M=$(PWD) clean ARCH=x86
		#make -C /usr/src/kernels/3.12.5-302.fc20.x86_64 M=$(PWD) clean ARCH=x86
		make -C /lib/modules/`uname -r`/build M=$(PWD) clean
#		make -C /usr/src/kernels/`uname -r` M=$(PWD) clean ARCH=x86
		#make -C /usr/src/kernels/3.11.10-301.fc20.x86_64 M=$(PWD) clean ARCH=x86
