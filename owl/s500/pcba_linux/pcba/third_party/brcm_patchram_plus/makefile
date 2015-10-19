CC=aarch64-linux-gnu-gcc
all:brcm_patchram_plus

brcm_patchram_plus: brcm_patchram_plus.o
	$(CC) -o $@ $^
brcm_patchram_plus.o: brcm_patchram_plus.c
	$(CC) -c $^ -o $@
clean:
	rm -f brcm_patchram_plus *.o
