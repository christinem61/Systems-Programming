all: fileCompressor.c test.o
	gcc fileCompressor.c test.o -o fileCompressor

test.o: test.c
	gcc -c test.c

clean:
	rm fileCompressor; rm test.o
