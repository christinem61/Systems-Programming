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
#include <errno.h>
#include <dirent.h>
#include <openssl/md5.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

int port;
struct sockaddr_in serverAddress, cli;
int sockfd;
int newSfd;

typedef struct Node{
	
	char* projectName;
	pthread_mutex_t mutex;
	struct Node* next;
	
} Node;

Node* head = NULL;
Node* ptr;
int lock = 0;

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

void deleteCom(char* proj, int len, char* activeCom){

	DIR* dptr = opendir(proj);
	struct dirent* dir = readdir(dptr);

	while(dir != NULL){
		if(dir->d_type == DT_REG){
			char* fileName = (char*)malloc(len + strlen(dir->d_name) + 2);
			strcpy(fileName, proj);
			strcat(fileName, "/");
			strcat(fileName, dir->d_name);
			strcat(fileName, "\0");
			
			if(strcmp(fileName, activeCom) != 0){
				char* com = (char*)malloc(8);
				strcpy(com, ".Commit");
				strcat(com, "\0");
				char* found = strstr(fileName, com);
				if(found){
					remove(fileName);
				}
				free(com);
			}
			free(fileName);
		}
		dir = readdir(dptr);
	}
	closedir(dptr);
}

void deleteProj(char* d){

	DIR* dptr = opendir(d);
	struct dirent* dir = readdir(dptr);

	while(dir != NULL){
		if(dir->d_type == DT_DIR){
			if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0){
				char* sub = malloc(strlen(d) + strlen(dir->d_name) + 2);
				strcpy(sub, d);
				strcat(sub, "/");
				strcat(sub, dir->d_name);
				strcat(sub, "\0");

				deleteProj(sub);
				remove(sub);
				free(sub);	
			}
		}
		else if(dir->d_type == DT_REG){
			char* fileName = malloc(strlen(d) + strlen(dir->d_name) + 2);
			strcpy(fileName, d);
			strcat(fileName, "/");
			strcat(fileName, dir->d_name);
			strcat(fileName, "\0");
		
			remove(fileName);
			free(fileName);
		}
		dir = readdir(dptr);
	}
	closedir(dptr);
}

char* oldManVer;
void moveOldProj(char* proj, int len, char* comFile){	

	char* projMan = (char*)malloc(len + 11);
	strcpy(projMan, proj);
	strcat(projMan, "/.Manifest");
	strcat(projMan, "\0");

	int fd = open(projMan, O_RDONLY);

	struct stat st;
	stat(projMan, &st);
	int size = st.st_size;
	char man[size+1];
	bzero(man, size+1);
	int status = 1;
	int readIn = 0;

	do{
		status = read(fd, man+readIn, sizeof(man)-readIn);
		readIn += status;
	}
	while(status > 0 && readIn < size);

	close(fd);

	int i = 0;
	while(i < size){
		if(man[i] == '\n'){
			oldManVer = substring(man, 0, i);
			break;
		}
		i++;
	}

	char* newProjName = (char*)malloc(len + 2 + strlen(oldManVer));
	strcpy(newProjName, proj);
	strcat(newProjName, "_");
	strcat(newProjName, oldManVer);
	strcat(newProjName, "\0");

	rename(proj, newProjName);

	char* archive = (char*)malloc(len + 10);
	strcpy(archive, ".");
	strcat(archive, proj);
	strcat(archive, "_Archive");
	strcat(archive, "\0");

	char* command = (char*)malloc(8 + strlen(newProjName) + strlen(archive));
	strcpy(command, "cp -R ");
	strcat(command, newProjName);
	strcat(command, " ");
	strcat(command, archive);
	strcat(command, "\0");
	system(command);
	free(command);	

	rename(newProjName, proj);
	char* newPath = (char*)malloc(strlen(archive) + strlen(newProjName) + 2);
	strcpy(newPath, archive);
	strcat(newPath, "/");
	strcat(newPath, newProjName);
	strcat(newPath, "\0");


	char* delete = (char*)malloc(strlen(newPath) + strlen(comFile) + 1);
	strcpy(delete, newPath);
	strcat(delete, comFile);
	strcat(delete, "\0");
	remove(delete);
	free(delete);

	char* compressOld = (char*)malloc(strlen(newPath) + strlen(newPath) + 18);
	strcpy(compressOld, "tar -czf ");
	strcat(compressOld, newPath);
	strcat(compressOld, ".tar.gz ");
	strcat(compressOld, newPath);	
	strcat(compressOld, "\0");
	system(compressOld);
	deleteProj(newPath);
	remove(newPath);
	free(compressOld);
	free(newPath);

	remove(projMan);

	char* end = substring(man, i, size);
	int manifestVer = atoi(oldManVer) + 1;
	char incVer[manifestVer+1];
	bzero(incVer, manifestVer+1);
	sprintf(incVer, "%d", manifestVer);
	int file = open(projMan, O_CREAT | O_RDWR | O_TRUNC, 00777);
	write(file, incVer, strlen(incVer));
	write(file, end, strlen(end));
	close(file);

	free(end);
	free(projMan);
	free(newProjName);
	free(archive);
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

int fileI;
int fileE;
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
	bzero(buffer, size+1);
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
					fileE = i+strlen(filePath)+36;
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
				i = i+34;
			}
		}
		if(buffer[i] == '\n'){
			n++;
			fileI = i+1;
		}
		i++;
	}
	close(fd);
	free(projMan);
	return 0;
}

void addFile(char* projName, char* insert){

	char* projMan = (char*)malloc(strlen(projName) + 11);
	strcpy(projMan, projName);
	strcat(projMan, "/.Manifest");
	strcat(projMan, "\0");

	int fd = open(projMan, O_RDWR | O_APPEND, 00777);
	write(fd, insert, strlen(insert));
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

void pushFunc(char* proj, int len){

	int found = mkdir(proj, 00777);
	if(found != -1){
		rmdir(proj);
		write(newSfd, "0", 1);
		printf("Error: Project name doesn't exist on server.\n");
	}
	else if(strcmp(strerror(errno), "File exists") == 0){
		write(newSfd, "1", 1);

		ptr = head;
		while(ptr != NULL){
			if(strcmp(ptr->projectName,proj) == 0){
				break;
			}
			ptr = ptr->next;
		}
		lock = 1;
		pthread_mutex_lock(&(ptr->mutex));

		char confirm[2];
		bzero(confirm, 2);
		read(newSfd, confirm, 1);
		if(strcmp(confirm, "0") == 0){
			printf("Error: Need to run commit before pushing.\n");
			pthread_mutex_unlock(&(ptr->mutex));
			return;
		}
		
		char get[2];
		bzero(get, 2);
		int IPSize = 0;
		while(1){
			read(newSfd, get, 1);
			if(get[0] == ':'){
				bzero(get, 2);
				break;
			}
			IPSize = IPSize*10 + atoi(get);
			bzero(get, 2);
		}
		char IPBuf[IPSize+1];
		bzero(IPBuf, IPSize+1);
		read(newSfd, IPBuf, IPSize);

		char* serComName = (char*)malloc(len + IPSize + 10);
		strcpy(serComName, proj);
		strcat(serComName, "/.Commit_");
		strcat(serComName, IPBuf);
		strcat(serComName, "\0");

		if(access(serComName, F_OK) == -1){
			write(newSfd, "0", 1);
			printf("Error: Commit not found.\n");
			free(serComName);
			pthread_mutex_unlock(&(ptr->mutex));
			return;
		}
		write(newSfd, "1", 1);

		int fd = open(serComName, O_RDONLY);

		struct stat st;
		stat(serComName, &st);
		int size = st.st_size;
		char serCom[size+1];
		bzero(serCom, size+1);
		int status = 1;
		int readIn = 0;

		do{
			status = read(fd, serCom+readIn, sizeof(serCom)-readIn);
			readIn += status;
		}
		while(status > 0 && readIn < size);
		close(fd);		

		int cliComSize = 0;
		while(1){
			read(newSfd, get, 1);
			if(get[0] == ':'){
				bzero(get, 2);
				break;
			}
			cliComSize = cliComSize*10 + atoi(get);
			bzero(get, 2);
		}
		char cliCom[cliComSize+1];
		bzero(cliCom, cliComSize+1);
		read(newSfd, cliCom, cliComSize);
		if(strcmp(serCom, cliCom) != 0){
			write(newSfd, "0", 1);
			free(serComName);
			printf("Error: Server and client commits do not match.\n");
			pthread_mutex_unlock(&(ptr->mutex));
			return;
		}
	
		write(newSfd, "1", 1);					
		
		deleteCom(proj, len, serComName);

		char* commitFile = (char*)malloc(IPSize + 10);
		strcpy(commitFile, "/.Commit_");
		strcat(commitFile, IPBuf);
		strcat(commitFile, "\0");

		moveOldProj(proj, len, commitFile);
		free(commitFile);	

		int k = 0;
		int s = 0;
		int start = 0;
		int end = 0;
		int sVer, eHash;
		char func[2];
		bzero(func, 2);
		char send[2];
		bzero(send, 2);
		
		while(k < size){
			if(serCom[k] == ' '){
				s++;
				if(s==1){
					sVer = k+1;
				}
				else if(s==2){
					start = k+1;
				}
				else if(s==3){
					end = k;
					k = k+32;
					eHash = k+1;
				}
			}
			if(s==0){
				func[0] = serCom[k];
			}
			if(serCom[k] == '\n'){
				if(func[0] == 'A'){
					write(newSfd, "0", 1);
					char* fileN = substring(serCom, start, end);
	
					int i = 0;
					char fSize[end-start+1];
					bzero(fSize, end-start+1);
					char send[2];
					bzero(send, 2);
					sprintf(fSize, "%d", end-start);
					while(i < strlen(fSize)){
						send[0] = fSize[i];
						write(newSfd, send, 1);
						bzero(send, 2);
						i++;
					}
					write(newSfd, ":", 1);
					write(newSfd, fileN, end-start);

					char get[2];
					bzero(get, 2);
					int fLen = 0;
					while(1){
						read(newSfd, get, 1);
						if(get[0] == ':'){
							bzero(get,2);
							break;
						}
						fLen = fLen*10 + atoi(get);
						bzero(get, 2);
					}
					char fileCon[fLen+1];
					bzero(fileCon, fLen+1);
					read(newSfd, fileCon, fLen);

					if((k+1) == size){
						write(newSfd, "1", 1);
					}
					else{
						write(newSfd, "0", 1);
					}

					createSubDir(fileN);
					int tempfd = open(fileN, O_CREAT | O_RDWR | O_TRUNC, 00777);
					write(tempfd, fileCon, fLen);
					close(tempfd);

					char* manLine = substring(serCom, sVer, eHash);
					addFile(proj, manLine);
					free(fileN);
					free(manLine);
				}
				else if(func[0] == 'D'){
					write(newSfd, "1", 1);
					
					if((k+1) == size){
						write(newSfd, "1", 1);
					}
					else{
						write(newSfd, "0", 1);
					}
					
					char* fileN = substring(serCom, start, end);
					remove(fileN);
					findFile(proj, fileN);
					removeFile(proj);
					free(fileN);
				}
				else if(func[0] == 'M'){
					write(newSfd, "0", 1);
					char* fileN = substring(serCom, start, end);				
					int i = 0;
					char fSize[end-start+1];
					bzero(fSize, end-start+1);
					char send[2];
					bzero(send, 2);
					sprintf(fSize, "%d", end-start);
					while(i < strlen(fSize)){
						send[0] = fSize[i];
						write(newSfd, send, 1);
						bzero(send, 2);
						i++;
					}
					write(newSfd, ":", 1);
					write(newSfd, fileN, end-start);

					char get[2];
					bzero(get, 2);
					int fLen = 0;
					while(1){
						read(newSfd, get, 1);
						if(get[0] == ':'){
							bzero(get,2);
							break;
						}
						fLen = fLen*10 + atoi(get);
						bzero(get, 2);
					}
					char fileCon[fLen+1];
					bzero(fileCon, fLen+1);
					read(newSfd, fileCon, fLen);

					if((k+1) == size){
						write(newSfd, "1", 1);
					}
					else{
						write(newSfd, "0", 1);
					}
					
					remove(fileN);
					int tempfd = open(fileN, O_CREAT | O_RDWR | O_TRUNC, 00777);
					write(tempfd, fileCon, fLen);
					close(tempfd);
					char* manLine = substring(serCom, sVer, eHash);
					findFile(proj, fileN);
					removeFile(proj);
					addFile(proj, manLine);
					
					free(manLine);
					free(fileN);
				}
				s = 0;
				bzero(func, 2);
			}
			k++;
		}

		char* hist = (char*)malloc(len + 19);
		strcpy(hist, ".");
		strcat(hist, proj);
		strcat(hist, "_Archive/.History");
		strcat(hist, "\0");

		int histFD = open(hist, O_RDWR | O_APPEND, 00777);
		write(histFD, oldManVer, strlen(oldManVer));
		write(histFD, "\n", 1);
		write(histFD, serCom, size);
		write(histFD, "\n", 1);
		close(histFD);
		
		remove(serComName);
		write(newSfd, "Push successful!", 16);
		free(serComName);
		free(hist);
		free(oldManVer);

		pthread_mutex_unlock(&(ptr->mutex));
		lock = 0;
	}
}

void cvFunc(char* proj, int len){

	int found = mkdir(proj, 00777);
	if(found != -1){
		rmdir(proj);
		write(newSfd, "0", 1);
		printf("Error: Project name doesn't exist on server.\n");
	}
	else if(strcmp(strerror(errno), "File exists") == 0){
		write(newSfd, "1", 1);

		char* projMan = (char*)malloc(len + 11);
		strcpy(projMan, proj);
		strcat(projMan, "/.Manifest");
		strcat(projMan, "\0");

		int fd =  open(projMan, O_RDONLY);
	
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
		char bufSize[size+1];
		bzero(bufSize, size+1);
		char send[2];
		bzero(send, 2);
		sprintf(bufSize, "%d", size);
		while(i<strlen(bufSize)){
			send[0] = bufSize[i];
			write(newSfd, send, 1);
			bzero(send, 2);
			i++;
		}
		write(newSfd, ":", 1);
		write(newSfd, buffer, size);
		close(fd);
		free(projMan);
		write(newSfd, "Project manifest sent!", 22);
	}
}

char* manVer;
char* findVer(char* proj, char* filePath, int len){

	int sVer = 0;
	int eVer = 0;
	int mVer = 0;

	char* projMan = (char*)malloc(len+11);
	strcpy(projMan, proj);
	strcat(projMan, "/.Manifest");
	strcat(projMan, "\0");

	int fd = open(projMan, O_RDONLY);

	struct stat st;
	stat(projMan, &st);
	int size = st.st_size;
	char buffer[size+1];
	bzero(buffer, size+1);
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
	int firstLine = 0;
	while(i < size){
		if(s == 2 && n == 1){
			char* dest = substring(buffer, i, i+len);
			if(strcmp(dest, filePath) == 0){
				if(buffer[i+len] == ' '){
					free(dest);
					return substring(buffer, sVer, eVer);
				}
			}
			free(dest);
			s = 0;
			n = 0;
		}
		if(buffer[i] == ' '){
			s++;
			if(s == 1){
				i = i+34;
			}
			else if(s == 2){
				eVer = i;
			}
		}
		if(buffer[i] == '\n'){
			n++;
			if(firstLine == 0){
				manVer = substring(buffer, 0, i);
				firstLine++;
			}
			sVer = i+1;
		}
		i++;
	}
	close(fd);
	free(projMan);
}

void upFunc(char* proj){
	int found = mkdir(proj, 00777);
	if(found != -1){
		rmdir(proj);
		write(newSfd, "0", 1);
		printf("Error: Project name doesn't exist on server.\n");
	}
	else if(strcmp(strerror(errno), "File exists") == 0){
		write(newSfd, "1", 1);
		char go[2];
		bzero(go, 2);
		read(newSfd, go, 1);
		if(go[0] == '0'){
			bzero(go, 2);
			return;
		}	

		bzero(go, 2);
		char empty[2];
		bzero(empty, 2);
		char stop[2];
		bzero(stop, 2);
		char delete[2];
		bzero(delete, 2);
		char sendV[2];
		bzero(sendV, 2);

		read(newSfd, empty, 1);		
		if(empty[0] == '1'){
			bzero(empty, 2);
			return;
		}
		bzero(empty, 2);

		while(1){			
			read(newSfd, delete, 1);

			if(delete[0] == '1'){
				bzero(delete, 2);
				read(newSfd, stop, 1);
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
					read(newSfd, get, 1);
					if(get[0] == ':'){
						bzero(get, 2);
						break;
					}
					fileNLen = fileNLen*10 + atoi(get);
					bzero(get, 2);
				}
				char fName[fileNLen+1];
				memset(fName, '\0',fileNLen+1);
				read(newSfd, fName, fileNLen);

				int fd = open(fName, O_RDONLY);
			
				struct stat st;
				stat(fName, &st);
				int size = st.st_size;
				char contents[size+1];
				memset(contents, '\0', size+1);
				int status = 1;
				int readIn = 0;

				do{
					status = read(fd, contents+readIn, sizeof(contents)-readIn);
					readIn += status;
				}
				while(status > 0 && readIn < size);

				close(fd);

				int i = 0;
				char fContentSize[size+1];
				bzero(fContentSize, size+1);
				char send[2];
				bzero(send, 2);
				sprintf(fContentSize, "%d", size);
				while(i < strlen(fContentSize)){
					send[0] = fContentSize[i];
					write(newSfd, send, 1);
					bzero(send, 2);
					i++;
				}
				write(newSfd, ":", 1);
				write(newSfd, contents, size);

				read(newSfd, sendV, 1);
				if(sendV[0] == '1'){
					char* version = findVer(proj, fName, fileNLen);
					i = 0;
					char vSize[strlen(version)+1];
					bzero(vSize, strlen(version)+1);
					sprintf(vSize, "%d", strlen(version));
					while(i < strlen(vSize)){
						send[0] = vSize[i];
						write(newSfd, send, 1);
						bzero(send, 2);
						i++;
					}
					write(newSfd, ":", 1);
					write(newSfd, version, strlen(version));
					free(version);
				}
				bzero(sendV, 2);

				read(newSfd, stop, 1);
				if(stop[0] == '1'){
					bzero(stop, 2);
					break;
				}
				bzero(stop, 2);
			}
		}
		
		int i = 0;
		char mvSize[strlen(manVer)+1];
		bzero(mvSize, strlen(manVer)+1);
		char sendM[2];
		bzero(sendM, 2);
		sprintf(mvSize, "%d", strlen(manVer));
		while(i < strlen(mvSize)){
			sendM[0] = mvSize[i];
			write(newSfd, sendM, 1);
			bzero(sendM, 2);
			i++;
		}
		write(newSfd, ":", 1);
		write(newSfd, manVer, strlen(manVer));
		free(manVer);
	}
}

void createFunc(char* proj, int len){

	int found = mkdir(proj, 00777);
	if(strcmp(strerror(errno), "File exists") == 0){
		write(newSfd, "1", 1);
		printf("Error: Project name already exists on server.\n");
	}
	else{
		char* man = (char*)malloc(len + 11);
		strcpy(man, proj);
		strcat(man, "/.Manifest");
		strcat(man, "\0");
		int file = open(man, O_CREAT | O_RDWR | O_TRUNC, 00777);
		write(file, "0\n", 2);
		close(file);
		free(man);

		char* archive = (char*)malloc(len + 10);
		strcpy(archive, ".");
		strcat(archive, proj);
		strcat(archive, "_Archive");
		strcat(archive, "\0");
		mkdir(archive, 00777);

		char* hist = (char*)malloc(strlen(archive) + 10);
		strcpy(hist, archive);
		strcat(hist, "/.History");
		strcat(hist, "\0");

		int histFD = open(hist, O_CREAT | O_RDWR | O_TRUNC, 00777);
		close(histFD);

		free(archive);
		free(hist);		


		if(head == NULL){
			head = (Node*)malloc(sizeof(Node));
			head->projectName = (char*)malloc(len + 1);
			strcpy(head->projectName, proj); 
			strcat(head->projectName, "\0");
			pthread_mutex_init(&(head->mutex), NULL);
			head->next = NULL;
		}
		else{
			ptr = head;
			while(ptr->next != NULL){
				ptr = ptr->next;
			}
			Node* temp = (Node*)malloc(sizeof(Node));
			temp->projectName = (char*)malloc(len + 1);
			strcpy(temp->projectName, proj);
			strcat(temp->projectName, "\0");
			pthread_mutex_init(&(temp->mutex), NULL);
			ptr->next = temp;
			temp->next = NULL;
		}


		write(newSfd, "New project created!", 20);
	}
}

char* str;
char* dirStr;
void createStr(char* d){

	DIR* dptr = opendir(d);
	struct dirent* dir = readdir(dptr);

	while(dir != NULL){
		if(dir->d_type == DT_DIR){
			if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0){
				char* sub = (char*)malloc(strlen(d) + strlen(dir->d_name) + 2);
				strcpy(sub, d);
				strcat(sub, "/");
				strcat(sub, dir->d_name);
				strcat(sub, "\0");

				char dirLen[strlen(sub)+1];
				memset(dirLen, '\0', strlen(sub)+1);
				sprintf(dirLen, "%d", strlen(sub));

				char* tempDir = (char*)malloc(strlen(dirStr)+1);
				strcpy(tempDir, dirStr);
				strcat(tempDir, "\0");
				free(dirStr);

				int dirSize = strlen(dirLen)+strlen(sub)+strlen(tempDir)+3;
			
				dirStr = (char*)malloc(dirSize);
				strcpy(dirStr, tempDir);
				strcat(dirStr, dirLen);
				strcat(dirStr, ":");
				strcat(dirStr, sub);
				strcat(dirStr, ":");
				strcat(dirStr, "\0");

				free(tempDir);
				createStr(sub);
				free(sub);
			}
		}
		else if(dir->d_type == DT_REG){
			char* fileName = (char*)malloc(strlen(d) + strlen(dir->d_name) + 2);
			strcpy(fileName, d);
			strcat(fileName, "/");
			strcat(fileName, dir->d_name);
			strcat(fileName, "\0");

			int fd = open(fileName, O_RDONLY);

			struct stat st;
			stat(fileName, &st);
			int size = st.st_size;
			char contents[size+1];
			memset(contents, '\0', size+1);
			int status = 1;
			int readIn = 0;

			do{
				status = read(fd, contents+readIn, sizeof(contents)-readIn);
				readIn += status;
			}
			while(status > 0 && readIn < sizeof(contents));

			close(fd);

			char len[strlen(fileName)+1];
			memset(len, '\0', strlen(fileName)+1);
			sprintf(len, "%d", strlen(fileName));
			char fileLen[size+1];
			memset(fileLen, '\0', size+1);
			sprintf(fileLen, "%d", size);

			char* tempStr = (char*)malloc(strlen(str) + 1);
			strcpy(tempStr, str);
			strcat(tempStr, "\0");
			free(str);
			int strSize = strlen(len)+strlen(fileName)+strlen(tempStr)+strlen(contents)+strlen(fileLen)+5;
			
			str = (char*)malloc(strSize);
			strcpy(str, tempStr);
			strcat(str, len);
			strcat(str, ":");
			strcat(str, fileName);
			strcat(str, ":");
			strcat(str, fileLen);
			strcat(str, ":");
			strcat(str, contents);
			strcat(str, ":");
			strcat(str, "\0");

			free(fileName);
			free(tempStr);
		}
		dir = readdir(dptr);
	}
	closedir(dptr);
}

void checkoutFunc(char* proj, int len){

	int stop = 0;
	char confirm[2];
	bzero(confirm, 2);
	read(newSfd, confirm, 1);
	if(strcmp(confirm, "1") == 0){
		printf("Error: Project name already exists on client.\n");
		stop = 1;
	}

	int found = mkdir(proj, 00777);
	if(found != -1){
		rmdir(proj);
		printf("Error: Project name does not exist on server.\n");
		write(newSfd, "0", 1);
	}
	else if(stop == 1){
		write(newSfd, "1", 1);
	}
	else if(strcmp(strerror(errno), "File exists") == 0){
		write(newSfd, "1", 1);

		str = (char*)malloc(2);
		strcpy(str, ":");
		strcat(str, "\0");
		dirStr = (char*)malloc(len + 2);
		strcpy(dirStr, proj);
		strcat(dirStr, ":");
		strcat(dirStr, "\0");
		createStr(proj);

		char mainDir[len+1];
		memset(mainDir, '\0', len+1);
		sprintf(mainDir, "%d", len);

		int i = 0;
		char send[2];
		bzero(send, 2);
		
		write(newSfd, "1", 1);

		while(i < strlen(mainDir)){
			send[0] = mainDir[i];
			write(newSfd, send, 1);
			bzero(send, 2);
			i++;			
		}
		write(newSfd, ":", 1);
		char* dirName = substring(dirStr, 0, len);

		write(newSfd, dirName, len);
		free(dirName);

		i = len+1;
		int dirNameLen = 0;
		while(i < strlen(dirStr)){

			write(newSfd, "1", 1);

			while(*(dirStr+i) != ':'){
				send[0] = *(dirStr+i);
				dirNameLen = dirNameLen*10 + atoi(send);
				write(newSfd, send, 1);
				bzero(send, 2);
				i++;
			}

			write(newSfd, ":", 1);
			i++;

			dirName = substring(dirStr, i, dirNameLen+i);
	
			write(newSfd, dirName, dirNameLen);
			i = i + dirNameLen + 1;
			dirNameLen = 0;
			free(dirName);
		}
		write(newSfd, "0", 1);

		i = 1;
		int fileNameLen = 0;
		int fileLen = 0;
		while(i < strlen(str)){

			write(newSfd, "1", 1);

			while(*(str+i) != ':'){
				send[0] = *(str+i);
				fileNameLen = fileNameLen*10 + atoi(send);
				write(newSfd, send, 1);
				bzero(send, 2);
				i++;
			}

			write(newSfd, ":", 1);
			i++;

			char* fileName = substring(str, i, fileNameLen+i);

			write(newSfd, fileName, fileNameLen);
			i = i + fileNameLen + 1;
			fileNameLen = 0;
			free(fileName);

			while(*(str+i) != ':'){
				send[0] = *(str+i);
				fileLen = fileLen*10 + atoi(send);
				write(newSfd, send, 1);
				bzero(send, 2);
				i++;
			}

			write(newSfd, ":", 1);
			i++;

			char* fileContents = substring(str, i, fileLen+i);
	
			write(newSfd, fileContents, fileLen);
			i = i + fileLen + 1;
			fileLen = 0;
			free(fileContents);

		}
		write(newSfd, "0", 1);

		free(str);
		free(dirStr);
	}
}

void destroyFunc(char* proj, int len){

	int found = mkdir(proj, 00777);
	if(found != -1){
		rmdir(proj);
		write(newSfd, "0", 1);
		printf("Error: Project does not exist on server.\n");
	}
	else if(strcmp(strerror(errno), "File exists") == 0){
		write(newSfd, "1", 1);

		ptr = head;
		while(ptr != NULL){
			if(strcmp(ptr->projectName,proj) == 0){
				break;
			}
			ptr = ptr->next;
		}
		lock = 1;
		pthread_mutex_lock(&(ptr->mutex));

		deleteProj(proj);
		remove(proj);

		char* archive = (char*)malloc(len + 10);
		strcpy(archive, ".");
		strcat(archive, proj);
		strcat(archive, "_Archive");
		strcat(archive, "\0");

		deleteProj(archive);
		rmdir(archive);

		write(newSfd, "Project destroyed!", 18);
		printf("Project destroyed!\n");
		free(archive);

		pthread_mutex_unlock(&(ptr->mutex));
		lock = 0;
	}
}

void getComFile(){

	char get[2];
	bzero(get, 2);
	int comLen = 0;
	while(1){
		read(newSfd, get, 1);
		if(get[0] == ':'){
			bzero(get, 2);
			break;
		}
		comLen = comLen*10 + atoi(get);
		bzero(get, 2);
	}
	char comFile[comLen+1];
	memset(comFile, '\0',comLen+1);
	read(newSfd, comFile, comLen);

	int fileSize = 0;
	while(1){
		read(newSfd, get, 1);
		if(get[0] == ':'){
			bzero(get, 2);
			break;
		}
		fileSize = fileSize*10 + atoi(get);
		bzero(get, 2);
	}
	char contents[fileSize+1];
	memset(contents, '\0',fileSize+1);
	read(newSfd, contents, fileSize);

	int fd = open(comFile, O_CREAT | O_RDWR | O_TRUNC, 00777);
	write(fd, contents, fileSize);
	close(fd);

	write(newSfd, "Commit file received!", 21);
}

char* getManVer(char* proj, int len){

	char* projMan = (char*)malloc(len + 11);
	strcpy(projMan, proj);
	strcat(projMan, "/.Manifest");
	strcat(projMan, "\0");

	int fd = open(projMan, O_RDONLY);
	
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

	close(fd);

	int i = 0;
	while(i < size){
		if(buffer[i] == '\n'){
			return substring(buffer, 0, i);
		}
		i++;
	}
	free(projMan);
}

void rollbackFunc(char* proj, int len, char* ver, char* mVer){

	int i = atoi(mVer) - 1;

	while(i > atoi(ver)){
		char num[i];
		bzero(num, i);
		sprintf(num, "%d", i);

		char* delete = (char*)malloc(len + strlen(num) + len + 19);
		strcpy(delete, ".");
		strcat(delete, proj);
		strcat(delete, "_Archive/");
		strcat(delete, proj);
		strcat(delete, "_");
		strcat(delete, num);
		strcat(delete, ".tar.gz");
		strcat(delete, "\0");

		remove(delete);
		free(delete);
		i--;
	}

	char num[i];
	bzero(num, i);
	sprintf(num, "%d", i);

	char* curProj = (char*)malloc(len + 19 + strlen(num) + len);
	strcpy(curProj, ".");
	strcat(curProj, proj);
	strcat(curProj, "_Archive/");
	strcat(curProj, proj);
	strcat(curProj, "_");
	strcat(curProj, num); 
	strcat(curProj, ".tar.gz");
	strcat(curProj, "\0");

	char* command2 = (char*)malloc(strlen(curProj) + 10);
	strcpy(command2, "tar -xzf ");
	strcat(command2, curProj);
	strcat(command2, "\0");
	system(command2);
	free(command2);
	remove(curProj);

	char* oldName = (char*)malloc(len + strlen(num) + 2);
	strcpy(oldName, proj);
	strcat(oldName, "_");
	strcat(oldName, num);
	strcat(oldName, "\0");

	char* keep = (char*)malloc(len + strlen(oldName) + 11);
	strcpy(keep, ".");
	strcat(keep, proj);
	strcat(keep, "_Archive/");
	strcat(keep, oldName);
	strcat(keep, "\0");

	char* command = (char*)malloc(strlen(keep) + 7);
	strcpy(command, "mv ");
	strcat(command, keep);
	strcat(command, " ./");
	strcat(command, "\0");
	system(command);
	free(command);

	deleteProj(proj);
	remove(proj);
	rename(oldName, proj);

	free(keep);
	free(curProj);
	free(oldName);

}

void* handleClients(){

	int funcLen = 0;
	char get[2];
	bzero(get, 2);
	while(1){
		read(newSfd, get, 1);
		if(get[0] == ':'){
			bzero(get, 2);
			break;
		}
		funcLen = funcLen*10 + atoi(get);
		bzero(get, 2);
	}
	char func[funcLen+1];
	bzero(func, funcLen+1);
	read(newSfd, func, funcLen);

	int len = 0;
	char temp[2];
	bzero(temp, 2);
	while(1){
		read(newSfd, temp, 1);
		if(temp[0] == ':'){
			bzero(temp, 2);
			break;
		}

		len = len*10 + atoi(temp);
		bzero(temp, 2);
	}

	char proj[len+1];
	bzero(proj, len+1);
	read(newSfd, proj, len);


	if(strcmp(func, "update") == 0){
		cvFunc(proj, len);
	}

	else if(strcmp(func, "upgrade") == 0){
		upFunc(proj);
	}

	else if(strcmp(func, "commit") == 0){
		cvFunc(proj, len);
		getComFile();
	}

	else if(strcmp(func, "push") == 0){
		pushFunc(proj, len);
	}
	
	else if(strcmp(func, "create") == 0){
		createFunc(proj, len);
	}

	else if(strcmp(func, "currentversion") == 0){
		cvFunc(proj, len);
	}

	else if(strcmp(func, "checkout") == 0){
		checkoutFunc(proj, len);
	}
	
	else if(strcmp(func, "destroy") == 0){
		destroyFunc(proj, len);
	}

	else if(strcmp(func, "rollback") == 0){
		int found = mkdir(proj, 00777);
		if(found != -1){
			rmdir(proj);
			write(newSfd, "0", 1);
			printf("Error: Project does not exist on server.\n");
		}
		else if(strcmp(strerror(errno), "File exists") == 0){
			write(newSfd, "1", 1);
			
			ptr = head;
			while(ptr != NULL){
				if(strcmp(ptr->projectName,proj) == 0){
					break;
				}
				ptr = ptr->next;
			}
			lock = 1;
			pthread_mutex_lock(&(ptr->mutex));

			char get[2];
			bzero(get, 2);
			int verLen = 0;
			while(1){
				read(newSfd, get, 1);
				if(get[0] == ':'){
					bzero(get, 2);
					break;
				}
				verLen = verLen*10 + atoi(get);
				bzero(get, 2);
			}
			char version[verLen+1];
			bzero(version, verLen+1);
			read(newSfd, version, verLen);
			
			char* manV = getManVer(proj, len);
			if(atoi(version) >= atoi(manV)){
				write(newSfd, "0", 1);
				free(manV);
				printf("Error: Invalid version number.\n");
			}
			else{
				write(newSfd, "1", 1);
				rollbackFunc(proj, len, version, manV);

				char* hist = (char*)malloc(len + 19);
				strcpy(hist, ".");
				strcat(hist, proj);
				strcat(hist, "_Archive/.History");
				strcat(hist, "\0");

				int histFD = open(hist, O_RDWR | O_APPEND);
				write(histFD, "Rollback to project version ", 28);
				write(histFD, version, verLen);
				write(histFD, ".\n\n", 3); 
				close(histFD);

				free(hist);
				free(manV);

				printf("Successful rollback!\n");
			}

			pthread_mutex_unlock(&(ptr->mutex));
			lock = 0;
		}
	}

	else if(strcmp(func, "history") == 0){
		int found = mkdir(proj, 00777);
		if(found != -1){
			rmdir(proj);
			write(newSfd, "0", 1);
			printf("Error: Project does not exist on server.\n");
		}
		else if(strcmp(strerror(errno), "File exists") == 0){
			write(newSfd, "1", 1);
			char* hist = (char*)malloc(len + 19);
			strcpy(hist, ".");
			strcat(hist, proj);
			strcat(hist, "_Archive/.History");
			strcat(hist, "\0");
			int histFD = open(hist, O_RDONLY);

			struct stat st;
			stat(hist, &st);
			int size = st.st_size;
			char histContents[size+1];
			bzero(histContents, size+1);
			int status = 1;
			int readIn = 0;

			do{
				status = read(histFD, histContents+readIn, sizeof(histContents)-readIn);
				readIn += status;
			}
			while(status > 0 && readIn < size);

			char histSize[size+1];
			memset(histSize, '\0', size+1);
			sprintf(histSize, "%d", size);

			int k = 0;
			char send[2];
			bzero(send, 2);
			while(k < strlen(histSize)){
				send[0] = histSize[k];
				write(newSfd, send, 1);
				bzero(send, 2);
				k++;			
			}
			write(newSfd, ":", 1);
			write(newSfd, histContents, size);
			
			write(newSfd, "Project history sent!", 21);
		
			free(hist);
		}
	}

	return NULL;

}

void sigHandler(int signum){

	printf("\nServer disconnected.\n");
	close(newSfd);
	exit(signum);
}

int main(int argc, char** argv){

	if(argc != 2){
		printf("Error: Incorrect number of arguments.\n");
		return 0;
	}

	port = atoi(argv[1]);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		printf("Socket creation failed.\n");
		return 0;
	}
	else{
		printf("Socket created!\n");
	}

	bzero(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);

	int b = bind(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)); 
	if(b != 0){
		printf("Socket bind failed.\n");
		return 0;
	}
	else{
		printf("Socket binded!\n");
	}

	int l = listen(sockfd, 5);
	if(l != 0){
		printf("Listen failed.\n");
		return 0;
	}
	else{
		printf("Server listening!\n");
	}

	signal(SIGINT, sigHandler);

	pthread_t t[100];
	int threadI = 0;

	while(1){

		printf("Waiting for connections...\n");

		int length = sizeof(cli);
		newSfd = accept(sockfd, (struct sockaddr*)&cli, &length);
		if(newSfd < 0){
			printf("Server accept failed.\n");
			close(newSfd);
			return 0;
		}
		printf("Server accepted the client!\n");
		if(lock == 0){
			pthread_create(&t[threadI], NULL, handleClients, (void*)&newSfd);
			threadI++;
		}
		else{
			close(newSfd);
			printf("Error: Respository is locked.\n");
		}
		
	}	

	printf("Server disconnected.\n");
	close(newSfd);

	return 0;

}

