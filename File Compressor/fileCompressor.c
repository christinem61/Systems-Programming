#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include "test.h"

Node* root = NULL;
int count = 0;

void freeNodes(Node* tempRoot){

	if(tempRoot == NULL){
		return;
	}

	freeNodes(tempRoot->left);
	freeNodes(tempRoot->right);

	free(tempRoot);

}

void freeCodes(Node* tempRoot){

	if(tempRoot == NULL){
		return;
	}

	freeCodes(tempRoot->left);
	freeCodes(tempRoot->right);

	if(tempRoot->code != NULL){
		free(tempRoot->code);
	}
}

void freeStrs(Node* tempRoot){

	if(tempRoot == NULL){
		return;
	}

	freeStrs(tempRoot->left);
	freeStrs(tempRoot->right);
	
	if(tempRoot->str != NULL){
		free(tempRoot->str);
	}
}

void buildAVL(char* fileName){
	
	int file = open(fileName, O_RDONLY);

	struct stat st;
	stat(fileName, &st);
	int size = st.st_size;
	char buffer[size+1];
	memset(buffer, '\0', size+1);
	int status = 1;
	int readIn = 0;
	
	do{
		status = read(file, buffer+readIn, size-readIn);
		readIn += status;
	}
	while(status > 0 && readIn < size);

	int i, j = 0;
	char temp[size+1];
	memset(temp, '\0', size+1);
	int search;

	for(i=0; i < sizeof(buffer); i++){

		if((buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\t' || i == (sizeof(buffer) - 1)) && temp[0] != '\0'){
			if(root == NULL){
				root = (Node*)malloc(sizeof(Node));	
				root->str = (char*)malloc(strlen(temp));
				strcpy(root->str, temp);
				root->freq = 1;
				root->bf = '-';
				root->left = NULL;
				root->right = NULL;
				root->parent = NULL;
				count++;
			}
			else{
				search = searchAVL(root, temp);
				if(search == -1){
					root = insert(root, temp);
					count++;
				}
			}
			j = 0;
			memset(temp, '\0', size+1);
		}
		if((buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\t') && temp[0] == '\0'){
			if(buffer[i] == ' '){
				if(root == NULL){
					root = (Node*)malloc(sizeof(Node));
					root->str = (char*)malloc(4);
					strcpy(root->str, "_\\");
					root->freq = 1;
					root->bf = '-';
					root->left = NULL;
					root->right = NULL;
					root->parent = NULL;
					count++;
				}
				else{
					search = searchAVL(root, "_\\");
					if(search == -1){
						root = insert(root, "_\\");
						count++;
					}
				}
			}
			else if(buffer[i] == '\n'){
				if(root == NULL){
					root = (Node*)malloc(sizeof(Node));
					root->str = (char*)malloc(4);
					strcpy(root->str, "_\\n");
					root->freq = 1;
					root->bf = '-';
					root->left = NULL;
					root->right = NULL;
					root->parent = NULL;
					count++;
				}
				else{
					search = searchAVL(root, "_\\n");
					if(search == -1){
						root = insert(root, "_\\n");
						count++;
					}
				}
			}
			else if(buffer[i] == '\t'){
				if(root == NULL){
					root = (Node*)malloc(sizeof(Node));
					root->str = (char*)malloc(4);
					strcpy(root->str, "_\\t");
					root->freq = 1;
					root->bf = '-';
					root->left = NULL;
					root->right = NULL;
					root->parent = NULL;
					count++;
				}
				else{
					search = searchAVL(root, "_\\t");
					if(search == -1){
						root = insert(root, "_\\t");
						count++;
					}
				}
			}
		}
		else{
			temp[j] = buffer[i];
			j++;
		}
	}	
	close(file);

}

void compress(Node* root, char* fileName, char* newFile){

	int file = open(fileName, O_RDONLY); 	

	struct stat st;
	stat(fileName, &st);
	int size = st.st_size;
	char buffer[size+1];
	memset(buffer, '\0', size+1);
	int status = 1;
	int readIn = 0;

	do{
		status = read(file, buffer+readIn, size-readIn);
		readIn += status;
	}
	while(status > 0 && readIn < size);
	
	close(file);

	int fd = open(newFile, O_RDWR | O_CREAT | O_TRUNC, 00777);

	int i, j = 0;
	char temp[size+1];
	memset(temp, '\0', size+1);
	char* add;
	Node* tempNode;

	for(i=0; i < sizeof(buffer); i++){

		if((buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\t' || i == (sizeof(buffer) - 1)) && temp[0] != '\0'){
			tempNode = getCode(root, temp);
			add = (char*)malloc(strlen(tempNode->code));
			strcpy(add, tempNode->code);
			write(fd, add, strlen(add));
			free(add);
			j = 0;
			memset(temp, '\0', size+1);
		}
		if((buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\t') && temp[0] == '\0'){
			if(buffer[i] == ' '){
				tempNode = getCode(root, "_\\");
				add = (char*)malloc(strlen(tempNode->code));
				strcpy(add, tempNode->code);
				write(fd, add, strlen(add));
				free(add);
				memset(temp, '\0', size+1);
			}
			else if(buffer[i] == '\n'){
				tempNode = getCode(root, "_\\n");
				add = (char*)malloc(strlen(tempNode->code));
				strcpy(add, tempNode->code);
				write(fd, add, strlen(add));
				free(add);
			}
			else if(buffer[i] == '\t'){
				tempNode = getCode(root, "_\\t");
				add = (char*)malloc(strlen(tempNode->code));
				strcpy(add, tempNode->code);
				write(fd, add, strlen(add));
				free(add);
			}
		}
		else{
			temp[j] = buffer[i];
			j++;
		}

	}	

	close(fd);

}

void decompress(Node* root, char* fileName, char* newFile){

	int file = open(fileName, O_RDONLY); 	

	struct stat st;
	stat(fileName, &st);
	int size = st.st_size;
	char buffer[size+1];
	memset(buffer, '\0', size+1);
	int status = 1;
	int readIn = 0;

	do{
		status = read(file, buffer+readIn, size-readIn);
		readIn += status;
	}
	while(status > 0 && readIn < size);
	
	close(file);

	int fd = open(newFile, O_RDWR | O_CREAT | O_TRUNC, 00777);

	int i, j = 0;
	char temp[size+1];
	memset(temp, '\0', size+1);
	char* add;
	Node* tempNode;
	int store = 0;

	for(i=0; i < sizeof(buffer); i++){

		if(buffer[i] == '\n'){
			break;
		}

		if(temp[0] == '\0'){
			temp[j] = buffer[i];
		}
		if(store == 1){
			temp[j] = buffer[i];
			store = 0;
		}

		tempNode = getStr(root, temp, 1);
		
		if(tempNode == NULL){
			store = 1;
			j++;
		}
		else{
			if(strcmp(tempNode->str, "_\\") == 0){
				write(fd, " ", 1);
			}
			else if(strcmp(tempNode->str, "_\\n") == 0){
				write(fd, "\n", 1);
			}
			else if(strcmp(tempNode->str, "_\\t") == 0){
				write(fd, "\t", 1);
			}
			else{
				add = (char*)malloc(strlen(tempNode->str));
				strcpy(add, tempNode->str);
				write(fd, add, strlen(add));
				free(add);
			}
			memset(temp, '\0', size+1);
			j = 0;
			tempNode = NULL;
		}
	}	

	close(fd);

}

void recursiveDir(char* d, char* flag){

	DIR* dptr = opendir(d);
	struct dirent* dir = readdir(dptr);

	while(dir != NULL){

		if(dir->d_type == DT_DIR){

			if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0){

				char* sub = malloc(strlen(d) + strlen(dir->d_name) + 2);
				strcpy(sub, d);
				strcat(sub, "/");
				strcat(sub, dir->d_name);

				recursiveDir(sub, flag);
				free(sub);
			}
		}
		else if(dir->d_type == DT_REG){

			char* fileName = malloc(strlen(d) + strlen(dir->d_name) + 2);
			strcpy(fileName, d);
			strcat(fileName,"/");
			strcat(fileName,dir->d_name);

			struct stat st;
			stat(fileName, &st);
			int size = st.st_size;


			if(strcmp(fileName, "./fileCompressor") == 0 && strcmp(fileName, "./test.o") == 0 || strcmp(fileName, ".//fileCompressor") == 0 || strcmp(fileName, ".//test.o") == 0){
				free(fileName);
				dir = readdir(dptr);
				continue;
			}
			
			if(strcmp(flag, "-b") == 0){

				if(size == 0){
					free(fileName);
					dir = readdir(dptr);
					continue;
				}

				buildAVL(fileName);
				free(fileName);
			}
			else if(strcmp(flag, "-c") == 0){
				int len = strlen(fileName);
				if(fileName[len-1] == 'z' && fileName[len-2] == 'c' && fileName[len-3] == 'h' && fileName[len-4] == '.'){}
				else{
					char* newFile = (char*)malloc(strlen(fileName) + 4);
					strcpy(newFile, fileName);
					strcat(newFile, ".hcz");

					if(size == 0){
						int fd = open(newFile, O_CREAT | O_RDWR | O_TRUNC, 00777);
						close(fd);
						free(newFile);
						free(fileName);
						dir = readdir(dptr);
						continue;
					}

					compress(root, fileName, newFile);
					free(newFile);	
				}
				free(fileName);
			}
			else if(strcmp(flag, "-d") == 0){
				int len = strlen(fileName);
				if(fileName[len-1] == 'z' && fileName[len-2] == 'c' && fileName[len-3] == 'h' && fileName[len-4] == '.'){
					char* newFile = (char*)malloc(len - 4);
					char* name = (char*)malloc(strlen(dir->d_name) - 4);
					int m = 0;
					while(m < (strlen(dir->d_name) - 4)){
						*(name + m) = *(dir->d_name + m);
						m++;
					}
					strcpy(newFile, d);
					strcat(newFile, "/");
					strcat(newFile, name);
		
					if(size == 0){
						int fd = open(newFile, O_CREAT | O_RDWR | O_TRUNC, 00777);
						close(fd);
						free(newFile);
						free(fileName);
						free(name);
						dir = readdir(dptr);
						continue;
					}

					decompress(root, fileName, newFile);	
					free(newFile);
					free(name);
				}
				free(fileName);
			}
		}

		dir = readdir(dptr);

	}

	closedir(dptr);	
}

void huffmanTraversal(Node* root, int fd){

	if(root == NULL){
		return;
	}

	huffmanTraversal(root->left, fd);
	if(root->str != NULL){
		write(fd, root->code, strlen(root->code));
		write(fd, "\t", 1);
		write(fd, root->str, strlen(root->str));
		write(fd, "\n", 1);
	}  
	huffmanTraversal(root->right, fd);
}

void createCodebook(Node** arr){

	int file = open("./HuffmanCodebook", O_CREAT | O_RDWR | O_TRUNC, 00777);
	huffmanTraversal(arr[count-1], file);

	close(file);
}

Node* codebookAVL(char* fileName){

	int file = open(fileName, O_RDONLY);
	
	struct stat st;
	stat(fileName, &st);
	int size = st.st_size;
	char buffer[size+1];
	memset(buffer, '\0', size+1);
	int status = 1;
	int readIn = 0;

	do{
		status = read(file, buffer+readIn, size-readIn);
		readIn += status;
	}
	while(status > 0 && readIn < size);

	int i, j = 0;
	char temp[size+1];
	memset(temp, '\0', size+1);
	char* codeStr;

	for(i=0; i < sizeof(buffer); i++){

		if(buffer[i] == '\t'){

			if(root == NULL){
				root = (Node*)malloc(sizeof(Node));
				root->code = (char*)malloc(strlen(temp));
				strcpy(root->code, temp);
				root->parent = NULL;
				root->left = NULL;
				root->right = NULL;
				codeStr = (char*)malloc(strlen(temp));
				strcpy(codeStr, temp);
			}
			else{
				root = insertCode(root, temp);
				codeStr = (char*)malloc(strlen(temp));
				strcpy(codeStr, temp);
			}
			memset(temp, '\0', size+1);
			j = 0;
		}
		else if(buffer[i] == '\n'){

			Node* tempNode = searchCodebookAVL(root, codeStr);
			tempNode->str = (char*)malloc(strlen(temp));
			strcpy(tempNode->str, temp);
			free(codeStr);
			memset(temp, '\0', size+1);
			j = 0;
		}
		else{
			temp[j] = buffer[i];
			j++;
		}
	}	

	close(file);
	return root;
}

int main(int argc, char** argv){

	if(argc > 5){
		printf("Error: Too many arguments!\n");
		return 0;
	}
	else if(argc < 3){
		printf("Error: Too few arguments!\n");
		return 0;
	}
	
	char* flag1;
	char* flag2;	
		
	if(strcmp(argv[1],"-R") == 0){
		flag1 = argv[1];
		if(strcmp(argv[2],"-b") == 0 || strcmp(argv[2],"-c") == 0 || strcmp(argv[2],"-d") == 0){
			flag2 = argv[2];
		}
		else{
			printf("Error: %s is not a valid flag.\n", argv[2]);
			return 0;
		}

		if(argv[3][0] == '-'){
			printf("Error: Cannot have more than 2 flags.\n");
			return 0;
		}
		else if(strcmp(argv[3], "./HuffmanCodebook") == 0 || strcmp(argv[3], "HuffmanCodebook") == 0){
			printf("Error: Codebook must come before the path and must be the last argument.\n");
			return 0;
		}

		if(strcmp(flag2, "-b") == 0 && argv[4] != NULL){
			printf("Error: Too many arguments. Only expected 4.\n");
			return 0;
		}
		else if(strcmp(flag2, "-c") == 0 || strcmp(flag2, "-d") == 0){
			if(argv[4] == NULL){
				printf("Error: Codebook must be included as the last argument.\n");
				return 0;
			}
			else if(strcmp(argv[4], "./HuffmanCodebook") != 0 && strcmp(argv[4], "HuffmanCodebook") != 0){
				printf("Error: Last argument must be codebook.\n");
				return 0;
			}
		}
	}
	else if(strcmp(argv[1],"-b") == 0 || strcmp(argv[1],"-c") == 0 || strcmp(argv[1],"-d") == 0){
		flag1 = argv[1];
		if(strcmp(argv[2],"-R") == 0){
			printf("Error: Flags are in the wrong order.\n");
			return 0;
		}
		else if(argv[2][0] == '-'){
			printf("Error: Cannot have more than 1 flag.\n");
			return 0;
		}
		if(strcmp(flag1, "-b") == 0 && argv[3] != NULL){
			printf("Error: Too many arguments. Only expected 3.\n");
			return 0;
		}
		else if(strcmp(flag1, "-c") == 0 || strcmp(flag1, "-d") == 0){
			if(strcmp(argv[2], "./HuffmanCodebook") == 0 || strcmp(argv[2], "HuffmanCodebook") == 0){
				printf("Error: Codebook must come before the path and must be the last argument.\n");
				return 0;
			}
			if(argv[3] == NULL){
				printf("Error: Codebook must be included as the last argument.\n");
				return 0;
			}
			else if(strcmp(argv[3], "./HuffmanCodebook") != 0 && strcmp(argv[3], "HuffmanCodebook") != 0){
				printf("Error: Last argument must be codebook.\n");
				return 0;
			}
		} 
	}
	else{
		printf("Error: %s is not a valid flag.\n", argv[1]);
		return 0;
	} 

	int size;

	if(strcmp(flag1, "-R") == 0){
		int errno;
		int fd = open(argv[3], O_RDWR);
		if(strcmp(strerror(errno), "Is a directory") == 0){}
		else{
			printf("Error: Must enter a valid directory.\n");
			close(fd);
			return 0;
		}
		close(fd);
	}
	else{
		int errno;
		int fd = open(argv[2], O_RDWR);
		if(strcmp(strerror(errno), "Is a directory") == 0){
			printf("Error: Must enter a file.\n");
			close(fd);
			return 0;
		}
		else if(strcmp(strerror(errno), "No such file or directory") == 0){
			printf("Error: File does not exist.\n");
			close(fd);
			return 0;
		}
		close(fd);

		if(strcmp(argv[2], "./fileCompressor") == 0 || strcmp(argv[2], "./test.o") == 0 || strcmp(argv[2], "fileCompressor") == 0 || strcmp(argv[2], "test.o") == 0){
			return 0;
		}
		
		struct stat st;
		stat(argv[2], &st);
		size = st.st_size;	
		
	}

	if(strcmp(flag1,"-b") == 0){
		if(size == 0){
			int file = open("./HuffmanCodebook", O_CREAT | O_RDWR | O_TRUNC, 00777);
			close(file);
			return 0;
		}
		
		buildAVL(argv[2]);

		if(count == 1){
			int file = open("./HuffmanCodebook", O_CREAT | O_RDWR | O_TRUNC, 00777);
			write(file, "0\t", 2);
			write(file, root->str, strlen(root->str));
			write(file, "\n", 1);
			close(file);
			free(root->str);
			free(root);
			return 0;
		}

		Node** arr = (Node**)malloc(count*sizeof(Node*));
		int index = 0;
		createArr(arr, root, index);
		minHeap(arr, 0, count-1);
		huffmanTree(arr, count);
		huffmanCodes(arr[count-1]->left, 'l');
		huffmanCodes(arr[count-1]->right, 'r');
		createCodebook(arr);		
		
		freeCodes(arr[count-1]);
		freeStrs(arr[count-1]);
		freeNodes(arr[count-1]);	
		free(arr);
	}
	else if(strcmp(flag1,"-R") == 0 && strcmp(flag2,"-b") == 0){
		recursiveDir(argv[3], flag2);

		if(root == NULL){
			int file = open("./HuffmanCodebook", O_CREAT | O_RDWR | O_TRUNC, 00777);
			close(file);
			return 0;
		}

		if(count == 1){
			int file = open("./HuffmanCodebook", O_CREAT | O_RDWR | O_TRUNC, 00777);
			write(file, "0\t", 2);
			write(file, root->str, strlen(root->str));
			write(file, "\n", 1);
			close(file);
			free(root->str);
			free(root);
			return 0;
		}

		Node** arr = (Node**)malloc(count*sizeof(Node*));
		int index = 0;
		createArr(arr, root, index);
		minHeap(arr, 0, count-1);
		huffmanTree(arr, count);
		huffmanCodes(arr[count-1]->left, 'l');
		huffmanCodes(arr[count-1]->right, 'r');
		createCodebook(arr);

		freeCodes(arr[count-1]);
		freeStrs(arr[count-1]);
		freeNodes(arr[count-1]);	
		free(arr);
	}
	else if(strcmp(flag1,"-c") == 0){
		int len = strlen(argv[2]);
		if(argv[2][len-1] == 'z' && argv[2][len-2] == 'c' && argv[2][len-3] == 'h' && argv[2][len-4] == '.'){}
		else{
			char* newFile = (char*)malloc(strlen(argv[2]) + 4);
			strcpy(newFile, argv[2]);
			strcat(newFile, ".hcz");

			if(size == 0){
				int file = open(newFile, O_CREAT | O_RDWR | O_TRUNC, 00777);
				close(file);
				free(newFile);
				return 0;
			}
			
			root = codebookAVL(argv[3]);
			compress(root, argv[2], newFile);		
			free(newFile);
		}

		freeCodes(root);
		freeStrs(root);
		freeNodes(root);
	
	}
	else if(strcmp(flag1,"-R") == 0 && strcmp(flag2,"-c") == 0){
		root = codebookAVL(argv[4]);	
		recursiveDir(argv[3], flag2);
		
		freeCodes(root);
		freeStrs(root);
		freeNodes(root);	
	}
	else if(strcmp(flag1,"-d") == 0){
		int len = strlen(argv[2]);
		if(argv[2][len-1] == 'z' && argv[2][len-2] == 'c' && argv[2][len-3] == 'h' && argv[2][len-4] == '.'){
			char* newFile = (char*)malloc(strlen(argv[2]) - 4);
			strncpy(newFile, argv[2], strlen(argv[2])-4);

			if(size == 0){
				int file = open(newFile, O_CREAT | O_RDWR | O_TRUNC, 00777);
				close(file);
				free(newFile);
				return 0;
			}

			root = codebookAVL(argv[3]);
			decompress(root, argv[2], newFile);
			free(newFile);
			
		}
		freeCodes(root);
		freeStrs(root);
		freeNodes(root);
	}
	else if(strcmp(flag1,"-R") == 0 && strcmp(flag2,"-d") == 0){
		root = codebookAVL(argv[4]);
		recursiveDir(argv[3], flag2);
	
		freeCodes(root);
		freeStrs(root);
		freeNodes(root);
	}

	return 0;

}
