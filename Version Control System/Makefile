all: client server
		
client: ./Client/WTF.c
	gcc ./Client/WTF.c -o ./Client/WTF -lcrypto

server: ./Server/WTFserver.c
	gcc ./Server/WTFserver.c -pthread -o ./Server/WTFserver -lcrypto

test: ./WTFtest.c
	gcc WTFtest.c -o WTFtest

clean: ./Client/WTF ./Server/WTFserver.c
	rm ./Client/WTF; rm ./Server/WTFserver; rm WTFtest
