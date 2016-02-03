img2array: main.c
	gcc -std=c11 -O2 -msse -msse2 -o img2array main.c

clean:
	rm -f *.o img2array

