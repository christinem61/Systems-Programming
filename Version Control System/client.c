#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <dirent.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>

char* host;
int port;
struct sockaddr_in serverAddress;
int sockfd;
int con;

char hash[33];
char* createHash(char* fileName){

	unsigned char code[MD5_DIGEST_LENGTH];
	int fd = open(fileName, O_RDONLY);
	MD5_CTX mdContext;
	struct stat st;
	stat(fileName, &st);
	int size = st.st_size;
	int status = 1;
	int readIn = 0;
	char buffer[size+1];
	memset(buffer, '\0', size+1);
	MD5_Init(&mdContext);
	do{
		status = read(fd, buffer+readIn, sizeof(buffer)-readIn);
		readIn += status;
		MD5_Update(&mdContext, buffer, status);
	}
	while(status > 0 && readIn < size);
	MD5_Final(code, &mdContext);
	memset(hash, '\0', 33);
	int i;
	for(i=0; i<16; i++){
		sprintf(&hash[i*2], "%02x", (unsigned int)code[i]);
	}
	close(fd);

	return hash;
}

char* substring(char* src, int m, int n){
	
	int len = n-m;
	char* dest = (char*)malloc(len+1);
	int i;
	for(i=m; i<n && (*(src+i) != '\0'); i++){
		*dest = *(src+i);
		dest++;
	}
	*dest = '\0';

	return dest - len;
}

int fileI;
int fileE;
int verStartI;
int verEndI;
int findFile(char* projName, char* filePath){

	char* projMan = (char*)malloc(strlen(projName) + 11);
	strcpy(projMan, projName);
	strcat(projMan, "/.Manifest");
	strcat(projMan, "\0");

	int fd = open(projMan, O_RDWR, 00777);

	struct stat st;
	stat(projMan, &st);
	int size = st.st_size;
	char buffer[size+1];
	memset(buffer, '\0', size+1);
	int status = 1;
	int readIn = 0;

	do{
		status = read(fd, buffer+readIn, sizeof(buffer)-readIn);
		readIn += status;
	}
	while(status > 0 && readIn < size);

	int i = 0;
	int s = 1;
	int n = 0;
	while(i < size){
		if(s == 2 && n == 1){
			char* dest = substring(buffer, i, i+strlen(filePath));
			if(strcmp(dest, filePath) == 0){
				if(buffer[i+strlen(filePath)] == ' '){
					fileE = i + strlen(filePath) + 36;
					free(dest);
					return 1;
				}
			}
			free(dest);
			s = 0;
			n = 0;
		}
		if(buffer[i] == ' '){
			s++;
			if(s == 1){
				i = i + 34;
			}
			else if(s == 2){
				verEndI = i;
			}
		}
		if(buffer[i] == '\n'){
			n++;
			fileI = i+1;
			verStartI = i+1;
		}
		i++;
	}

	close(fd);
	free(projMan);
	return 0;
}

void addFile(char* version, char* projName, char* filePath){

	char* projMan = (char*)malloc(strlen(projName) + 11);
	strcpy(projMan, projName);
	strcat(projMan, "/.Manifest");
	strcat(projMan, "\0");

	createHash(filePath);

	int fd = open(projMan, O_RDWR | O_APPEND, 00777);
	write(fd, version, strlen(version));
	write(fd, " ", 1);
	write(fd, filePath, strlen(filePath));
	write(fd, " ", 1);
	write(fd, hash, strlen(hash)); 	
	write(fd, " 0\n", 3);	

	close(fd);
	free(projMan);
}

void removeFile(char* projName){

	char* projMan = (char*)malloc(strlen(projName) + 11);
	strcpy(projMan, projName);
	strcat(projMan, "/.Manifest");
	strcat(projMan, "\0");
	char* tempFile = (char*)malloc(strlen(projName) + 15);
	strcpy(tempFile, projName);
	strcat(tempFile, "/.ManifestTemp");
	strcat(tempFile, "\0");

	int fd = open(projMan, O_RDONLY);

	struct stat st;
	stat(projMan, &st);
	int size = st.st_size;
	int status = 1;
	int readIn = 0;
	char buffer[size+1];
	bzero(buffer, size+1);

	do{
		status = read(fd, buffer+readIn, sizeof(buffer)-readIn);
		readIn += status;
	}
	while(status > 0 && readIn < size);
	close(fd);
	
	char* begin = substring(buffer, 0, fileI);
	char* end = substring(buffer, fileE, size);

	int file = open(tempFile, O_CREAT | O_RDWR | O_TRUNC, 00777);
	write(file, begin, strlen(begin));
	write(file, end, strlen(end));
	close(file);

	remove(projMan);
	rename(tempFile, projMan);

	free(begin);
	free(end);
	free(projMan);
	free(tempFile);
}

int sVerCI;
int eVerCI;
int sHashCI;
int eHashCI;
char* cli;
int compareSerMan(char* serLine, int cliLen){

	int i = 0;
	int s = 0;
	int n = 0;
	while(i < cliLen){
		if(s == 2 && n == 1){
			char* dest = substring(cli, eVerCI+1, sHashCI-1);
			if(strcmp(dest, serLine) == 0){	
				*(cli+(i-1)) = '1';
				free(dest);
				return 1;
			}
			free(dest);
			s = 0;
			n = 0;
		}
		if(*(cli+i) == ' '){
			s++;
			if(s == 1){
				eVerCI = i;
			}
			else if(s == 2){
				sHashCI = i+1;
				i = i+32;
				eHashCI = i+1;
				i = i+2;
			}
		}
		if(*(cli+i) == '\n'){
			n++;
			sVerCI = i+1;
		}
		i++;
	}
	return 0;
}

int sVerSI;
int eVerSI;
int sHashSI;
int eHashSI;
char* ser;
int compareCliMan(char* cliLine, int serLen){

	int i = 0;
	int s = 0;
	int n = 0;
	while(i < serLen){
		if(s == 2 && n == 1){
			char* dest = substring(ser, eVerSI+1, sHashSI-1);
			if(strcmp(dest, cliLine) == 0){	
				*(ser+(i-1)) = '1';
				free(dest);
				return 1;
			}
			free(dest);
			s = 0;
			n = 0;
		}
		if(*(ser+i) == ' '){
			s++;
			if(s == 1){
				eVerSI = i;
			}
			else if(s == 2){
				sHashSI = i+1;
				i = i+32;
				eHashSI = i+1;
				i = i+2;
			}
		}
		if(*(ser+i) == '\n'){
			n++;
			sVerSI = i+1;
		}
		i++;
	}
	return 0;
}

void sendComFile(char* comFile){

	int fd = open(comFile, O_RDONLY);

	struct stat st;
	stat(comFile, &st);
	int contentsSize = st.st_size;
	char contents[contentsSize+1];
	memset(contents, '\0', contentsSize+1);
	int status = 1;
	int readIn = 0;

	do{
		status = read(fd, contents+readIn, sizeof(contents)-readIn);
		readIn += status;
	}
	while(status > 0 && readIn < contentsSize);

	close(fd);

	char hostbuf[256];
	int hostname = gethostname(hostbuf, sizeof(hostbuf));
	struct hostent* host_entry = gethostbyname(hostbuf);
	char* IPBuf = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));

	char* serverCom = (char*)malloc(strlen(comFile) + strlen(IPBuf) + 2);
	strcpy(serverCom, comFile);
	strcat(serverCom, "_");
	strcat(serverCom, IPBuf);
	strcat(serverCom, "\0");

	int i = 0;
	char scLen[strlen(serverCom)+1];
	bzero(scLen, strlen(serverCom)+1);
	char send[2];
	bzero(send, 2);
	sprintf(scLen, "%d", strlen(serverCom));
	while(i<strlen(scLen)){
		send[0] = scLen[i];
		write(sockfd, send, 1);
		bzero(send, 2);
		i++;
	}
	write(sockfd, ":", 1);
	write(sockfd, serverCom, strlen(serverCom));

	i = 0;
	char cSize[contentsSize+1];
	bzero(cSize, contentsSize+1);
	sprintf(cSize, "%d", contentsSize);
	while(i<strlen(cSize)){
		send[0] = cSize[i];
		write(sockfd, send, 1);
		bzero(send, 2);
		i++;
	}
	write(sockfd, ":", 1);
	write(sockfd, contents, contentsSize);

	char buffer[22];
	bzero(buffer, 22);
	read(sockfd, buffer, 22);
	printf("From server: %s\n", buffer);

	free(serverCom);
}

void createSubDir(char* path){

	int start = 0;
	int s = 0;
	int i = 0;
	while(i < strlen(path)){
		if(*(path+i) == '/'){
			s++;
			if(s == 1){
				start = i+1;
			}
			else if(s > 2){
				char* sub = substring(path, start, i);
				mkdir(sub, 00777);
				free(sub);
			}
		}
		i++;
	}
}

char* findPath(char* proj, char* file){

	char* filePath;
	char* checkFile = substring(file, 0, 2);
	if(strcmp(checkFile, "./") == 0){
		filePath = (char*)malloc(strlen(file)+1);
		strcpy(filePath, file);
		strcat(filePath, "\0");
	}
	else{
		char* checkProj = substring(proj, 0, 2);
		if(strcmp(checkProj, "./") == 0){
			filePath = (char*)malloc(strlen(proj) + strlen(file) + 2);
			strcpy(filePath, proj);
			strcat(filePath, "/");
			strcat(filePath, file);
			strcat(filePath, "\0");				
		}
		else{
			filePath = (char*)malloc(strlen(proj) + strlen(file) + 4);
			strcpy(filePath, "./");
			strcat(filePath, proj);
			strcat(filePath, "/");
			strcat(filePath, file);
			strcat(filePath, "\0");
		}
		free(checkProj);
	}
	free(checkFile);

	return filePath;
}

void updateProjVer(char* projName, char* version, int vLen){

	char* projMan = (char*)malloc(strlen(projName) + 11);
	strcpy(projMan, projName);
	strcat(projMan, "/.Manifest");
	strcat(projMan, "\0");
	char* tempFile = (char*)malloc(strlen(projName) + 15);
	strcpy(tempFile, projName);
	strcat(tempFile, "/.ManifestTemp");
	strcat(tempFile, "\0");

	int fd = open(projMan, O_RDONLY);

	struct stat st;
	stat(projMan, &st);
	int size = st.st_size;
	int status = 1;
	int readIn = 0;
	char buffer[size+1];
	bzero(buffer, size+1);

	do{
		status = read(fd, buffer+readIn, sizeof(buffer)-readIn);
		readIn += status;
	}
	while(status > 0 && readIn < size);

	int i = 0;
	while(i < size){
		if(buffer[i] == '\n'){
			break;
		}
		i++;
	}

	char* end = substring(buffer, i, size);
	close(fd);

	int file = open(tempFile, O_CREAT | O_RDWR | O_TRUNC, 00777);
	write(file, version, vLen);
	write(file, end, strlen(end));
	close(file);

	remove(projMan);
	rename(tempFile, projMan);
	free(end);
	free(projMan);
	free(tempFile);

}

void sigHandler(int signum){

	free(host);
	printf("\nClient disconnected.\n");
	close(sockfd);
	exit(signum);
}

int main(int argc, char** argv){

	signal(SIGINT, sigHandler);

	if(strcmp(argv[1],"configure") != 0 && strcmp(argv[1],"checkout") != 0 && strcmp(argv[1],"update") != 0 && strcmp(argv[1],"upgrade") != 0 && strcmp(argv[1],"commit") != 0 && strcmp(argv[1],"push") != 0 && strcmp(argv[1],"add") != 0 && strcmp(argv[1],"remove") != 0 && strcmp(argv[1],"create") != 0 && strcmp(argv[1],"destroy") != 0 && strcmp(argv[1],"history") != 0 && strcmp(argv[1],"rollback") != 0 && strcmp(argv[1],"currentversion") != 0){
		printf("Error: Not a valid command.\n");
		return 0;
	}
	
	if(strcmp(argv[1], "configure") != 0 && strcmp(argv[1],"add") != 0 && strcmp(argv[1],"remove") != 0 && strcmp(argv[1],"rollback") != 0){
		if(argc != 3){
			printf("Error: Incorrect number of arguments.\n");
			return 0;
		}
	}

	if(strcmp(argv[1], "configure") == 0 || strcmp(argv[1],"add") == 0 || strcmp(argv[1],"remove") == 0 || strcmp(argv[1],"rollback") == 0){
		if(argc != 4){
			printf("Error: Incorrect number of arguments.\n");
			return 0;
		}
	}

	if(strcmp(argv[1], "add") == 0){
		int exists = mkdir(argv[2], 00777);
		if(exists != -1){
			rmdir(argv[2]);
			printf("Error: Project does not exist.\n");
			return 0;
		}
		else if(strcmp(strerror(errno), "File exists") == 0){
			char* filePath = findPath(argv[2], argv[3]);
			if(access(filePath, F_OK) == -1){
				printf("Error: File does not exist in project folder.\n");
				free(filePath);
				return 0;
			}		

			int found = findFile(argv[2], filePath);
			if(found == 1){
				free(filePath);
				printf("Error: File already exists in the Manifest.\n");
				return 0;
			}
			addFile("0", argv[2], filePath);
			free(filePath);
			return 0;	
		}
	}

	if(strcmp(argv[1], "remove") == 0){
		int exists = mkdir(argv[2], 00777);
		if(exists != -1){
			rmdir(argv[2]);
			printf("Error: Project does not exist.\n");
			return 0;
		}
		else if(strcmp(strerror(errno), "File exists") == 0){
			char* filePath = findPath(argv[2], argv[3]);
			int found = findFile(argv[2], filePath);
			if(found == 0){
				free(filePath);
				printf("Error: File does not exist in the Manifest.\n");
				return 0;
			}
			free(filePath);
			removeFile(argv[2]);
			return 0;
		}
	}

	if(strcmp(argv[1], "configure") == 0){
		int fd = open("./.configure", O_CREAT | O_RDWR | O_TRUNC, 00777);
		write(fd, argv[2], strlen(argv[2]));
		write(fd, "\n", 1);
		write(fd, argv[3], strlen(argv[3]));
		close(fd);
		return 0;
	}

	int file = open("./.configure", O_RDONLY);
	if(access("./.configure", F_OK) == -1){
		printf("Error: Configure must be run first.\n");
		close(file);
		return 0;
	}

	struct stat st;
	stat("./.configure", &st);
	int size = st.st_size;
	char config[size+1];
	memset(config, '\0', size+1);
	int status = 1;
	int readIn = 0;

	do{
		status = read(file, config+readIn, sizeof(config)-readIn);
		readIn += status;
	}
	while(status > 0 && readIn < size);

	int i = 0;
	while(i < size){
		if(config[i] == '\n'){
			host = substring(config, 0, i);
			char* portStr = substring(config, i+1, size);
			port = atoi(portStr);
			free(portStr);
			break;
		}
		i++;
	}
	close(file);
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		printf("Socket creation failed.\n");
		close(sockfd);
		free(host);
		return 0;
	}
	else{
		printf("Socket created!\n");
	}

	bzero(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	struct hostent* result = gethostbyname(host);
	bcopy((char*)result->h_addr, (char*)&serverAddress.sin_addr.s_addr, result->h_length);
	serverAddress.sin_port = htons(port);

	con = connect(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
	while(con != 0){
		printf("Searching for server...\n");
		sleep(3);
		con = connect(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
	}
	printf("Server & Client Connected!\n");

	i = 0;
	int funcL = strlen(argv[1]);
	char funcLen[funcL+1];
	bzero(funcLen, funcL+1);
	char sendFunc[2];
	bzero(sendFunc, 2);
	sprintf(funcLen, "%d", funcL);

	while(i<strlen(funcLen)){
		sendFunc[0] = funcLen[i];
		write(sockfd, sendFunc, 1);
		bzero(sendFunc, 2);
		i++;
	}
	write(sockfd, ":", 1);
	write(sockfd, argv[1], strlen(argv[1]));

	char* project;
	char* check = substring(argv[2], 0, 2);
	if(strcmp(check, "./") == 0){
		project = (char*)malloc(strlen(argv[2]) - 1);
		char* ending = substring(argv[2], 2, strlen(argv[2]));
		strcpy(project, ending);
		strcat(project, "\0");
		free(ending); 
	}
	else{
		project = (char*)malloc(strlen(argv[2]) + 1);
		strcpy(project, argv[2]);
		strcat(project, "\0");
	}
	free(check);

	i = 0;
	int strL = strlen(project);
	char len[strL+1];
	bzero(len, strL+1);
	char send[2];
	bzero(send, 2);
	sprintf(len, "%d", strL);

	while(i<strlen(len)){
		send[0] = len[i];
		write(sockfd, send, 1);
		bzero(send, 2);
		i++;
	}
	write(sockfd, ":", 1);
	write(sockfd, project, strlen(project));
	free(project);	

	if(strcmp(argv[1], "update") == 0){
		char confirm[2]; 
		bzero(confirm, 2);
		read(sockfd, confirm, 1);
		if(strcmp(confirm, "0") == 0){
			printf("Error: Project name doesn't exist on server.\n");
		}
		else if(strcmp(confirm, "1") == 0){
			int bufLen = 0;
			char get[2];
			bzero(get, 2);
			while(1){			
				read(sockfd, get, 1);
				if(get[0] == ':'){
					bzero(get, 2);
					break;
				}	
				bufLen = bufLen*10 + atoi(get);
				bzero(get, 2);
			}
	
			char sMan[bufLen+1];
			bzero(sMan, bufLen+1);
			read(sockfd, sMan, bufLen);

			char buffer[23];
			bzero(buffer, 23);
			read(sockfd, buffer, 22);
			printf("From server: %s\n", buffer);
			
			char* projMan = (char*)malloc(strlen(argv[2])+11);
			strcpy(projMan, argv[2]);
			strcat(projMan, "/.Manifest");
			strcat(projMan, "\0");

			int fd = open(projMan, O_RDONLY);

			struct stat st;
			stat(projMan, &st);
			int size = st.st_size;
			char cMan[size+1];
			bzero(cMan, size+1);
			int status = 1;
			int readIn = 0;

			do{
				status = read(fd, cMan+readIn, sizeof(cMan)-readIn);
				readIn += status;
			}
			while(status > 0 && readIn < size);

			cli = cMan;
			close(fd);

			char* updateFile = (char*)malloc(strlen(argv[2])+9);
			strcpy(updateFile, argv[2]);
			strcat(updateFile, "/.Update");
			strcat(updateFile, "\0");
			char* conflictFile = (char*)malloc(strlen(argv[2])+11);
			strcpy(conflictFile, argv[2]);
			strcat(conflictFile, "/.Conflict");
			strcat(conflictFile, "\0");
			int upFP = open(updateFile, O_CREAT | O_RDWR | O_TRUNC, 00777);
			int conFP = open(conflictFile, O_CREAT | O_RDWR | O_TRUNC, 00777);

			i = 0;
			int j = 0;
			while(sMan[i] != '\n'){
				i++;
			}
			while(cMan[j] != '\n'){
				j++;
			}

			int conflict = 0;

			char* sManVer = substring(sMan, 0, i);
			char* cManVer = substring(cMan, 0, j);
			
			if(strcmp(sManVer, cManVer) == 0){
				if(access(conflictFile, F_OK) == -1){}
				else{
					remove(conflictFile);
				}
				printf("Up To Date\n");
			}
			else{
				int k = 0;
				int s = 0;
				int n = 0;
				while(k < bufLen){
					if(s==2 && n==1){
						char* serLine = substring(sMan, eVerSI+1, sHashSI-1);
						int match = compareSerMan(serLine, size);
						if(match == 1){
							char* cliFile = substring(cMan, eVerCI+1, sHashCI-1);
							createHash(cliFile);
							char* clientHash = substring(cMan, sHashCI, eHashCI);
							char* serverHash = substring(sMan, sHashSI, eHashSI);
							char* serVer = substring(sMan, sVerSI, eVerSI);
							char* cliVer = substring(cMan, sVerCI, eVerCI);

							if(strcmp(serverHash, clientHash) != 0){
								if(strcmp(clientHash, hash) != 0){
									char* filepath = substring(cMan, eVerCI+1, sHashCI-1);
									write(conFP, "C ", 2);
									write(conFP, filepath, (sHashCI-1)-(eVerCI+1));
									write(conFP, " ", 1);
									write(conFP, hash, 32);
									write(conFP, "\n", 1);

									printf("C %s\n", filepath);
									free(filepath);
									conflict = 1;
								}
								else if(strcmp(clientHash, hash) == 0 && strcmp(serVer, cliVer) != 0){
									char* filepath = substring(cMan, eVerCI+1, sHashCI-1);
									write(upFP, "M ", 2);
									write(upFP, filepath, (sHashCI-1)-(eVerCI+1));
									write(upFP, " ", 1);
									write(upFP, serverHash, 32);
									write(upFP, "\n", 1);

									printf("M %s\n", filepath);
									free(filepath);
								}
								
							}
							free(serVer);
							free(cliVer);
							free(cliFile);
							free(clientHash);
							free(serverHash);
						}
						else if(match == 0){
							char* filepath = substring(sMan, eVerSI+1, sHashSI-1);
							char* serverHash = substring(sMan, sHashSI, eHashSI);
							write(upFP, "A ", 2);
							write(upFP, filepath, (sHashSI-1)-(eVerSI+1));
							write(upFP, " ", 1);
							write(upFP, serverHash, 32);
							write(upFP, "\n", 1);
				
							printf("A %s\n", filepath);
							free(filepath);
							free(serverHash);
						}
						free(serLine);
						n = 0;
						s = 0;				
					}
					if(sMan[k] == ' '){
						s++;
						if(s==1){
							eVerSI = k;
						}
						else if(s==2){
							sHashSI = k+1;
							k = k+32;
							eHashSI = k+1;
							k = k+2;
						}
					}
					if(sMan[k] == '\n'){
						n++;
						sVerSI = k+1;
					}
					k++;
				}

				k = 0;
				n = 0;
				s = 0;
				while(k < size){
					if(s==2 && n==1){
						if(cMan[k-1] == '0'){
							char* filepath = substring(cMan, eVerCI+1, sHashCI-1);
							char* clientHash = substring(cMan, sHashCI, eHashCI);
							write(upFP, "D ", 2);
							write(upFP, filepath, (sHashCI-1)-(eVerCI+1));
							write(upFP, " ", 1);
							write(upFP, clientHash, 32);
							write(upFP, "\n", 1);
				
							printf("D %s\n", filepath);
							free(filepath);
							free(clientHash);
						}
						else if(cMan[k-1] == '1'){
							cMan[k-1] = '0';
						}
						s = 0;
						n = 0;
					}
					if(cMan[k] == ' '){
						s++;
						if(s==1){
							eVerCI = k;
						}
						else if(s==2){
							sHashCI = k+1;
							k = k+32;
							eHashCI = k+1;
							k = k+2;
						}
					}
					if(cMan[k] == '\n'){
						n++;
						sVerCI = k+1;
					}
					k++;
				}
			}
			
			close(upFP);
			close(conFP);

			if(conflict == 1){
				remove(updateFile);
				printf("Conflicts were found and must be resolved before update can happen.\n");
			}
			else{
				remove(conflictFile);
				printf("Update successful!\n");
			}

			free(projMan);
			free(updateFile);
			free(conflictFile);
			free(sManVer);
			free(cManVer);

		}	
	}


	else if(strcmp(argv[1], "upgrade") == 0){
		char confirm[2]; 
		bzero(confirm, 2);
		read(sockfd, confirm, 1);

		if(strcmp(confirm, "0") == 0){
			printf("Error: Project name doesn't exist on server.\n");
		}
		else if(strcmp(confirm, "1") == 0){
			char* conflictFile = (char*)malloc(strlen(argv[2]) + 11);
			strcpy(conflictFile, argv[2]);
			strcat(conflictFile, "/.Conflict"); 
			strcat(conflictFile, "\0");
			char* upFile = (char*)malloc(strlen(argv[2]) + 9);
			strcpy(upFile, argv[2]);
			strcat(upFile, "/.Update");
			strcat(upFile, "\0");

			if(access(conflictFile, F_OK) != -1){
				write(sockfd, "0", 1);
				printf("Error: Must resolve all conflicts and then update.\n");
				free(conflictFile);
				free(upFile);
				free(host);
				printf("Client disconnected.\n");
				return 0;
			}
			else if(access(upFile, F_OK) == -1){
				write(sockfd, "0", 1);
				printf("Error: Must update before upgrading.\n");
				free(conflictFile);
				free(upFile);
				printf("Client disconnected.\n");
				return 0;
			}
	
			write(sockfd, "1", 1);
			int upfd = open(upFile, O_RDONLY);
			
			struct stat st;
			stat(upFile, &st);
			int size = st.st_size;
			char upContents[size+1];
			memset(upContents, '\0', size+1);
			int status = 1;
			int readIn = 0;

			if(size == 0){
				close(upfd);
				write(sockfd, "1", 1);
				printf("Project is up to date.\n");
				remove(upFile);
				free(conflictFile);
				free(upFile);
				free(host);
				printf("Client disconnected.\n");
				return 0;
			}

			do{
				status = read(upfd, upContents+readIn, sizeof(upContents)-readIn);
				readIn += status;
			}
			while(status > 0 && readIn < size);

			char* manifest = (char*)malloc(strlen(argv[2]) + 11);
			strcpy(manifest, argv[2]);
			strcat(manifest, "/.Manifest");
			strcat(manifest, "\0");

			int manFD = open(manifest, O_RDONLY);
			struct stat a;
			stat(manifest, &a);
			int manS = a.st_size;
			char cMan[manS+1];
			memset(cMan, '\0', manS+1);
			int bytes = 1;
			int rIn = 0;

			do{
				bytes = read(manFD, cMan+rIn, sizeof(cMan)-rIn);
				rIn += bytes;
			}
			while(bytes > 0 && rIn < manS);
			close(manFD);
			free(manifest);

			int k = 0;
			int s = 0;
			int start = 0;
			int end = 0;
			char func[2];
			bzero(func, 2);
			char send[2];
			bzero(send, 2);
			write(sockfd, "0", 1);

			while(k < size){
				if(upContents[k] == ' '){
					s++;	
					if(s==1){
						start = k+1;
					}
					else if(s==2){
						end = k;
						k = k+32;
					}
				}
				if(s==0){
					func[0] = upContents[k];
				}
				if(upContents[k] == '\n'){
					if(func[0] == 'A'){
						write(sockfd, "0", 1);		
						char* file = substring(upContents,start,end);

						i = 0;
						char fSize[end-start+1];
						bzero(fSize, end-start+1);
						char send[2];
						bzero(send, 2);
						sprintf(fSize, "%d", end-start);
						while(i < strlen(fSize)){
							send[0] = fSize[i];
							write(sockfd, send, 1);
							bzero(send, 2);
							i++;
						}
						write(sockfd, ":", 1);
						write(sockfd, file, end-start);

						char get[2];
						bzero(get, 2);
						int fLen = 0;
						while(1){
							read(sockfd, get, 1);
							if(get[0] == ':'){
								bzero(get, 2);
								break;
							}
							fLen = fLen*10 + atoi(get);
							bzero(get, 2);
						}
						char fName[fLen+1];
						bzero(fName, fLen+1);
						read(sockfd, fName, fLen);

						write(sockfd, "1", 1);

						int vLen = 0;
						while(1){
							read(sockfd, get, 1);
							if(get[0] == ':'){
								bzero(get, 2);
								break;
							}
							vLen = vLen*10 + atoi(get);
							bzero(get, 2);
						}
						char version[vLen+1];
						bzero(version, vLen+1);
						read(sockfd, version, vLen);

						if((k+1) == size){
							write(sockfd, "1", 1);			
						}
						else{
							write(sockfd, "0", 1);
						}

						createSubDir(file);
						int tempfd = open(file, O_CREAT | O_RDWR | O_TRUNC, 00777);
						write(tempfd, fName, fLen);
						close(tempfd);
						addFile(version, argv[2], file);
						free(file);
					}
					else if(func[0] == 'D'){
						write(sockfd, "1", 1);
						char* file = substring(upContents,start,end);
						findFile(argv[2], file);
						removeFile(argv[2]);
						free(file);

						if((k+1) == size){
							write(sockfd, "1", 1);
						}
						else{
							write(sockfd, "0", 1);
						}
					}
					else if(func[0] == 'M'){
						write(sockfd, "0", 1);
						char* file = substring(upContents,start,end);
						remove(file);

						i = 0;
						char fSize[end-start+1];
						bzero(fSize, end-start+1);
						char send[2];
						bzero(send, 2);
						sprintf(fSize, "%d", end-start);
						while(i < strlen(fSize)){
							send[0] = fSize[i];
							write(sockfd, send, 1);
							bzero(send, 2);
							i++;
						}
						write(sockfd, ":", 1);
						write(sockfd, file, end-start);

						char get[2];
						bzero(get, 2);
						int fLen = 0;
						while(1){
							read(sockfd, get, 1);
							if(get[0] == ':'){
								bzero(get, 2);
								break;
							}
							fLen = fLen*10 + atoi(get);
							bzero(get, 2);
						}
						char fName[fLen+1];
						bzero(fName, fLen+1);
						read(sockfd, fName, fLen);

						write(sockfd, "0", 1);

						if((k+1) == size){
							write(sockfd, "1", 1);
						}
						else{
							write(sockfd, "0", 1);
						}
						int tempfd = open(file, O_CREAT | O_RDWR | O_TRUNC, 00777);
						write(tempfd, fName, fLen);
						close(tempfd);
						findFile(argv[2], file);
						removeFile(argv[2]);
						char* vers = substring(cMan, verStartI, verEndI);
						addFile(vers, argv[2], file);
						free(file);
						free(vers);
					}
					s = 0;
					bzero(func, 2);
				}
				k++;
			}
			close(upfd);
			remove(upFile);
			free(upFile);
			free(conflictFile);

			char get[2];
			bzero(get, 2);
			int mvLen = 0;
			while(1){
				read(sockfd, get, 1);
				if(get[0] == ':'){
					bzero(get, 2);
					break;
				}
				mvLen = mvLen*10 + atoi(get);
				bzero(get, 2);
			}
			char mVer[mvLen+1];
			bzero(mVer, mvLen+1);
			read(sockfd, mVer, mvLen);
			updateProjVer(argv[2], mVer, mvLen);

			printf("Upgrade successful!\n");
		}
	}

	else if(strcmp(argv[1], "commit") == 0){
		char confirm[2]; 
		bzero(confirm, 2);
		read(sockfd, confirm, 1);
		if(strcmp(confirm, "0") == 0){
			printf("Error: Project name doesn't exist on server.\n");
		}
		else if(strcmp(confirm, "1") == 0){
			char* conflictFile = (char*)malloc(strlen(argv[2]) + 11);
			strcpy(conflictFile, argv[2]);
			strcat(conflictFile, "/.Conflict");
			strcat(conflictFile, "\0"); 
			char* upFile = (char*)malloc(strlen(argv[2]) + 9);
			strcpy(upFile, argv[2]);
			strcat(upFile, "/.Update");
			strcat(upFile, "\0");

			if(access(conflictFile, F_OK) != -1){
				write(sockfd, "0", 1);
				printf("Error: Must resolve all conflicts and then update.\n");
				free(conflictFile);
				free(upFile);
				free(host);
				printf("Client disconnected.\n");
				return 0;
			}
			else if(access(upFile, F_OK) != -1){
				struct stat st;
				stat(upFile, &st);
				int upSize = st.st_size;
				if(upSize != 0){
					write(sockfd, "0", 1);
					printf("Error: A non-empty Update file exists.\n");
					free(conflictFile);
					free(upFile);
					free(host);
					printf("Client disconnected.\n");
					return 0;
				}
			}
			free(upFile);
			free(conflictFile);

			int bufLen = 0;
			char get[2];
			bzero(get, 2);
			while(1){			
				read(sockfd, get, 1);
				if(get[0] == ':'){
					bzero(get, 2);
					break;
				}	
				bufLen = bufLen*10 + atoi(get);
				bzero(get, 2);
			}
	
			char sMan[bufLen+1];
			bzero(sMan, bufLen+1);
			read(sockfd, sMan, bufLen);

			char buffer[23];
			bzero(buffer, 23);
			read(sockfd, buffer, 22);
			printf("From server: %s\n", buffer);
			
			char* projMan = (char*)malloc(strlen(argv[2])+10);
			strcpy(projMan, argv[2]);
			strcat(projMan, "/.Manifest");

			int fd = open(projMan, O_RDONLY);

			struct stat st;
			stat(projMan, &st);
			int size = st.st_size;
			char cMan[size+1];
			bzero(cMan, size+1);
			int status = 1;
			int readIn = 0;

			do{
				status = read(fd, cMan+readIn, sizeof(cMan)-readIn);
				readIn += status;
			}
			while(status > 0 && readIn < size);
	
			close(fd);
		
			i = 0;
			int j = 0;
			while(sMan[i] != '\n'){
				i++;
			}
			while(cMan[j] != '\n'){
				j++;
			}

			char* sManVer = substring(sMan, 0, i);
			char* cManVer = substring(cMan, 0, j);

			if(strcmp(sManVer, cManVer) != 0){
				free(projMan);
				free(sManVer);
				free(cManVer);
				free(host);
				printf("Error: Must update local project first.\n");
				printf("Client disconnected.\n");
				return 0;
			}

			ser = sMan;
			char* comFile = (char*)malloc(strlen(argv[2]) + 9);
			strcpy(comFile, argv[2]);
			strcat(comFile, "/.Commit");
			strcat(comFile, "\0");

			int comFD = open(comFile, O_CREAT | O_RDWR | O_TRUNC, 00777);

			int k = 0;
			int s = 0;
			int n = 0;
			while(k < size){

				if(s==2 && n==1){
					char* cliFile = substring(cMan, eVerCI+1, sHashCI-1);
					char* cliHash = substring(cMan, sHashCI, eHashCI);
					createHash(cliFile);
					int match = compareCliMan(cliFile, bufLen);
					if(match == 1){
						char* serHash = substring(sMan, sHashSI, eHashSI);
						if(strcmp(serHash, cliHash) == 0){
							if(strcmp(cliHash, hash) != 0){
								char* version = substring(cMan, sVerCI, eVerCI);
								int ver = atoi(version) + 1;
								char incVer[ver+1];
								bzero(incVer, ver+1);
								sprintf(incVer, "%d", ver);

								write(comFD, "M ", 2);
								write(comFD, incVer, strlen(incVer));
								write(comFD, " ", 1);
								write(comFD, cliFile, (sHashCI-1)-(eVerCI+1));
								write(comFD, " ", 1);
								write(comFD, hash, 32);
								write(comFD, "\n", 1);
	
								printf("M %s\n", cliFile);
								free(version);
							}
						}
						else{
							char* sVer = substring(sMan, sVerSI, eVerSI);
							char* cVer = substring(cMan, sVerCI, eVerCI);
							if(strcmp(sVer, cVer) > 0){
								printf("Error: Client must synch with repository before committing changes.\n");
								close(comFD);
								remove(comFile);
								free(comFile);
								free(projMan);
								free(sVer);
								free(cVer);
								free(serHash);
								free(cliHash);
								free(cliFile);
								free(host);
								printf("Client disconnected.\n");
								return 0;
							}
							free(sVer);
							free(cVer);
						}
						free(serHash);
						
					}
					else if(match == 0){
						char* version = substring(cMan, sVerCI, eVerCI);
						int ver = atoi(version) + 1;
						char incVer[ver+1];
						bzero(incVer, ver+1);
						sprintf(incVer, "%d", ver);

						write(comFD, "A ", 2);
						write(comFD, incVer, strlen(incVer));
						write(comFD, " ", 1);
						write(comFD, cliFile, (sHashCI-1) - (eVerCI+1));
						write(comFD," ", 1);
						write(comFD, cliHash, 32);
						write(comFD, "\n", 1);

						printf("A %s\n", cliFile);
						free(version);
					}	
					free(cliFile);
					free(cliHash);	
					s = 0;
					n = 0;
				}
				if(cMan[k] == ' '){
					s++;
					if(s==1){
						eVerCI = k;
					}
					else if(s==2){
						sHashCI = k+1;
						k = k+32;
						eHashCI = k+1;
						k = k+2;
					}
				}
				if(cMan[k] == '\n'){
					n++;
					sVerCI = k+1;
				}
				k++;
			}

			k = 0;
			n = 0;
			s = 0;
			while(k < bufLen){
				if(s==2 && n==1){
					if(sMan[k-1] == '0'){
						char* filepath = substring(sMan, eVerSI+1, sHashSI-1);
						char* serHash = substring(sMan, sHashSI, eHashSI);
						char* version = substring(sMan, sVerSI, eVerSI);
						int ver = atoi(version) + 1;
						char incVer[ver+1];
						bzero(incVer, ver+1);
						sprintf(incVer, "%d", ver);

						write(comFD, "D ", 2);
						write(comFD, incVer, strlen(incVer));
						write(comFD, " ", 1);
						write(comFD, filepath, (sHashSI-1)-(eVerSI+1));
						write(comFD, " ", 1);
						write(comFD, serHash, 32);
						write(comFD, "\n", 1);
			
						printf("D %s\n", filepath);
						free(filepath);
						free(serHash);
						free(version);
					}
					else if(sMan[k-1] == '1'){
						sMan[k-1] = '0';
					}
					s = 0;
					n = 0;
				}
				if(sMan[k] == ' '){
					s++;
					if(s==1){
						eVerSI = k;
					}
					else if(s==2){
						sHashSI = k+1;
						k = k+32;
						eHashSI = k+1;
						k = k+2;
					}
				}
				if(sMan[k] == '\n'){
					n++;
					sVerSI = k+1;
				}
				k++;
			}
	
			struct stat a;
			stat(comFile,&a);
			int comS = a.st_size;
			close(comFD);

			if(comS == 0){
				remove(comFile);
				printf("Up to Date.\n");
			}
			else{
				sendComFile(comFile);
			}

			free(projMan);
			free(comFile);
			free(sManVer);
			free(cManVer);

			printf("Commit successful!\n");
		}	
	}	

	else if(strcmp(argv[1], "push") == 0){
		char confirm[2]; 
		bzero(confirm, 2);
		read(sockfd, confirm, 1);
		if(strcmp(confirm, "0") == 0){
			printf("Error: Project name doesn't exist on server.\n");
		}
		else if(strcmp(confirm, "1") == 0){
			char* comFile = (char*)malloc(strlen(argv[2]) + 9);
			strcpy(comFile, argv[2]);
			strcat(comFile, "/.Commit");
			strcat(comFile, "\0");			

			if(access(comFile, F_OK) == -1){
				write(sockfd, "0", 1);
				printf("Error: Need to run commit before pushing.\n");
			}
			else{
				write(sockfd, "1", 1);
				
				char hostbuf[256];
				int hostname = gethostname(hostbuf, sizeof(hostbuf));
				struct hostent* host_entry = gethostbyname(hostbuf);
				char* IPBuf = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));

				i = 0;
				char IPSize[strlen(IPBuf)+1];
				bzero(IPSize, strlen(IPBuf)+1);
				char send[2];
				bzero(send, 2);
				sprintf(IPSize, "%d", strlen(IPBuf));
				while(i < strlen(IPSize)){
					send[0] = IPSize[i];
					write(sockfd, send, 1);
					bzero(send, 2);
					i++;
				}
				write(sockfd, ":", 1);
				write(sockfd, IPBuf, strlen(IPBuf));
				
				char found[2];
				bzero(found, 2);
				read(sockfd, found, 1);

				if(strcmp(found, "0") == 0){
					printf("Error: Commit not found on server.\n");
					remove(comFile);
					printf("Client disconnected.\n");
					free(comFile);
					free(host);
					return 0;
				}

				int comFD = open(comFile, O_RDONLY);

				struct stat st;
				stat(comFile, &st);
				int size = st.st_size;
				char contents[size+1];
				bzero(contents, size+1);
				int status = 1;
				int readIn = 0;

				do{
					status = read(comFD, contents+readIn, sizeof(contents)-readIn);
					readIn += status;
				}
				while(status > 0 && readIn < size);
				close(comFD);

				i = 0;
				char comSize[size+1];
				bzero(comSize, size+1);
				sprintf(comSize, "%d", size);
				while(i < strlen(comSize)){
					send[0] = comSize[i];
					write(sockfd, send, 1);
					bzero(send, 2);
					i++;
				}
				write(sockfd, ":", 1);
				write(sockfd, contents, size);

				char match[2];
				bzero(match, 2);
				read(sockfd, match, 1);
				if(strcmp(match, "0") == 0){
					printf("Error: Server and client commits do not match.\n");
					printf("Client disconnected.\n");
					remove(comFile);
					free(comFile);
					free(host);
					return 0;
				}				
				
				char delete[2];
				bzero(delete, 2);
				char stop[2];
				bzero(stop, 2);

				while(1){
					read(sockfd, delete, 1);
					if(delete[0] == '1'){
						bzero(delete, 2);
						read(sockfd, stop, 1);
						if(stop[0] == '1'){
							bzero(stop, 2);
							break;
						}
						bzero(stop, 2);
					}
					else if(delete[0] == '0'){
						bzero(delete, 2);
						char get[2];
						bzero(get, 2);
						int fileNLen = 0;
						while(1){
							read(sockfd, get, 1);
							if(get[0] == ':'){
								bzero(get, 2);
								break;
							}
							fileNLen = fileNLen*10 + atoi(get);
							bzero(get, 2);
						}
						char fName[fileNLen+1];
						memset(fName, '\0', fileNLen+1);
						read(sockfd, fName, fileNLen);

						int request = open(fName, O_RDONLY);
						
						struct stat s;
						stat(fName, &s);
						int fSize = s.st_size;
						char fContents[fSize+1];
						memset(fContents, '\0', fSize+1);
						int stats = 1;
						int in = 0;
		
						do{
							stats = read(request, fContents+in, sizeof(fContents)-in);
							in += stats;
						}
						while(stats > 0 && in < fSize);
						close(request);

						i = 0;
						char fContentSize[fSize+1];
						bzero(fContentSize, fSize+1);
						sprintf(fContentSize, "%d", fSize);
						while(i < strlen(fContentSize)){
							send[0] = fContentSize[i];
							write(sockfd, send, 1);
							bzero(send, 2);
							i++;
						}
						write(sockfd, ":", 1);
						write(sockfd, fContents, fSize);
	
						read(sockfd, stop, 1);
						if(stop[0] == '1'){
							bzero(stop, 2);
							break;
						}
						bzero(stop, 2);
					}

				}				

				char message[17];
				bzero(message, 17);
				read(sockfd, message, 16);
				printf("%s\n", message);
				
				remove(comFile);
				free(comFile);
			}
		}
	}

	else if(strcmp(argv[1], "create") == 0){	
		char buffer[21]; 
		bzero(buffer, 21);
		read(sockfd, buffer, 20);

		if(strcmp(buffer, "1") == 0){
			printf("Error: Project name already exists on server.\n");
		}
		else if(strcmp(buffer, "New project created!") == 0){
			mkdir(argv[2], 00777);
			char* man = (char*)malloc(strlen(argv[2]) + 11);
			strcpy(man, argv[2]);
			strcat(man, "/.Manifest");
			strcat(man, "\0");
			int fd = open(man, O_CREAT | O_RDWR | O_TRUNC, 00777);
			write(fd, "0\n", 2);
			close(fd);
			free(man);

			printf("From server: %s\n", buffer);
		}
	}

	else if(strcmp(argv[1], "currentversion") == 0){	
		char confirm[2]; 
		bzero(confirm, 2);
		read(sockfd, confirm, 1);
		if(strcmp(confirm, "0") == 0){
			printf("Error: Project name doesn't exist on server.\n");
		}
		else if(strcmp(confirm, "1") == 0){
			int bufLen = 0;
			char get[2];
			bzero(get, 2);
			while(1){			
				read(sockfd, get, 1);
				if(get[0] == ':'){
					bzero(get, 2);
					break;
				}	
				bufLen = bufLen*10 + atoi(get);
				bzero(get, 2);
			}
	
			char man[bufLen+1];
			bzero(man, bufLen+1);
			read(sockfd, man, bufLen);

			char buffer[23];
			bzero(buffer, 23);
			read(sockfd, buffer, 22);
			printf("From server: %s\n", buffer);
		
			i = 0;
			int startI;
			int s = 0;
			while(i < bufLen){
				if(man[i] == ' '){
					s++;
				}
				if(s == 2){
					char* dest = substring(man, startI, i);
					printf("%s\n", dest);
					free(dest);
					s = 0;
					i = i + 34;
				}
				if(man[i] == '\n'){
					startI = i+1;
				}
				i++;
			}

			printf("Currentversion successful!\n");
		}
	}

	else if(strcmp(argv[1], "checkout") == 0){
		char confirm[2];
		bzero(confirm, 2);
		int exists = mkdir(argv[2], 00777);
		if(strcmp(strerror(errno), "File exists") == 0){
			printf("Error: Project name already exists on client.\n");
			write(sockfd, "1", 1);
			read(sockfd, confirm, 1);
			if(strcmp(confirm, "0") == 0){
				printf("Error: Project name does not exist on server.\n");
			}
		}
		else{
			rmdir(argv[2]);
			write(sockfd, "0", 1);
			read(sockfd, confirm, 1);
			if(strcmp(confirm, "0") == 0){
				printf("Error: Project name does not exist on server.\n");
			}
			else{
				int dirLen = 0;
				char get[2];
				bzero(get, 2);
				char stop[2];
				bzero(stop, 2);

				while(1){

					read(sockfd, stop, 1);
					if(stop[0] == '0'){
						bzero(stop, 2);
						break;
					}
					bzero(stop, 2);

					while(1){
						read(sockfd, get, 1);
						if(get[0] == ':'){
							bzero(get, 2);
							break;
						}
						dirLen = dirLen*10 + atoi(get);
						bzero(get, 2);
					}

					char dirName[dirLen+1];
					bzero(dirName, dirLen+1);
					read(sockfd, dirName, dirLen);
					mkdir(dirName, 00777);
					dirLen = 0;
				}

				int fileNameLen = 0;
				int fileLen = 0;

				while(1){

					read(sockfd, stop, 1);
					if(stop[0] == '0'){
						bzero(stop, 2);
						break;
					}
					bzero(stop, 2);

					while(1){
						read(sockfd, get, 1);
						if(get[0] == ':'){
							bzero(get, 2);
							break;
						}
						fileNameLen = fileNameLen*10 + atoi(get);
						bzero(get, 2);
					}

					char fileName[fileNameLen+1];
					bzero(fileName, fileNameLen+1);
					read(sockfd, fileName, fileNameLen);
					int fileptr = open(fileName, O_CREAT | O_RDWR | O_TRUNC, 00777);

					while(1){
						read(sockfd, get, 1);
						if(get[0] == ':'){
							bzero(get, 2);
							break;
						}
						fileLen = fileLen*10 + atoi(get);
						bzero(get, 2);
					}

					char fileContents[fileLen+1];
					bzero(fileContents, fileLen+1);
					read(sockfd, fileContents, fileLen);

					write(fileptr, fileContents, fileLen);

					close(fileptr);
					fileNameLen = 0;
					fileLen = 0;
				}

				printf("Checkout successful!\n");
			}
		}
	}

	else if(strcmp(argv[1], "destroy") == 0){	
		char confirm[2]; 
		bzero(confirm, 2);
		read(sockfd, confirm, 1);

		if(strcmp(confirm, "0") == 0){
			printf("Error: Project does not exist on server.\n");
		}
		else{
			char buffer[19]; 
			bzero(buffer, 19);
			read(sockfd, buffer, 18);
			printf("From server: %s\n", buffer);
		}
	}

	else if(strcmp(argv[1], "rollback") == 0){
		char confirm[2]; 
		bzero(confirm, 2);
		read(sockfd, confirm, 1);

		if(strcmp(confirm, "0") == 0){
			printf("Error: Project does not exist on server.\n");
		}
		else{
			char vSize[strlen(argv[3])+1];
			bzero(vSize, strlen(argv[3])+1);
			sprintf(vSize, "%d", strlen(argv[3]));

			int k = 0;
			char send[2];
			bzero(send, 2);
			while(k < strlen(vSize)){
				send[0] = vSize[0];
				write(sockfd, send, 1);
				bzero(send, 2);
				k++;
			}
			write(sockfd, ":", 1);
			write(sockfd, argv[3], strlen(argv[3]));

			bzero(confirm, 2);
			read(sockfd, confirm, 1);
			if(strcmp(confirm, "0") == 0){
				printf("Error: Invalid version number.\n");
			}
			else{
				printf("Rollback successful!\n");
			}
		}
	}

	else if(strcmp(argv[1], "history") == 0){
		char confirm[2]; 
		bzero(confirm, 2);
		read(sockfd, confirm, 1);

		if(strcmp(confirm, "0") == 0){
			printf("Error: Project does not exist on server.\n");
		}
		else{
			char get[2];
			bzero(get, 2);
			int fileLen = 0;
			while(1){
				read(sockfd, get, 1);
				if(get[0] == ':'){
					bzero(get, 2);
					break;
				}
				fileLen = fileLen*10 + atoi(get);
				bzero(get, 2);
			}
			char fileContents[fileLen+1];
			bzero(fileContents, fileLen+1);
			read(sockfd, fileContents, fileLen);

			char message[22];
			bzero(message, 22);
			read(sockfd, message, 21);
			printf("From server: %s\n", message);

			printf("%s\n", fileContents);

			printf("History successful!\n");
		}
	}

	free(host);
	printf("Client disconnected.\n");
	close(sockfd);

	return 0;
}
