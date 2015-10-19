#CC=arm-none-linux-gnueabi-gcc
CC=aarch64-linux-gnu-gcc

rtk_hciattach:hciattach.c hciattach_rtk.o  
	$(CC) -o rtk_hciattach hciattach.c hciattach_rtk.o  

hciattach_rtk.o:hciattach_rtk.c
	$(CC) -c hciattach_rtk.c

clean:
	rm -f *.o  rtk_hciattach
