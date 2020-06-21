#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

typedef struct Node{

	int val;
	char* str;
	int inPlace;
	struct Node* next;
	struct Node* prev;

} Node;



int insertionSort(void* toSort, int (*comparator)(void*,void*)){


	Node* head = (Node*)toSort;
	Node* ptr1;
	Node* ptr2;
	Node* temp;

	if(head == NULL || head->next == NULL){
		return 0;
	}

	ptr1 = head;
	ptr2 = head->next;


	while(ptr2->next != NULL){
	
		if(comparator(ptr2,ptr1) == 1 || comparator(ptr2,ptr1) == 0){
			ptr2 = ptr2->next;
			ptr1 = ptr1->next;
		}
		else if(comparator(ptr2,ptr1) == -1){
			while(comparator(ptr2,ptr1) == -1 && ptr1->prev!=NULL){
				ptr1 = ptr1->prev;
			}
			if(ptr1->prev == NULL && comparator(ptr2,ptr1) == -1){
				temp = ptr2;
				ptr2 = ptr2->next;
				temp->prev->next = ptr2;
				ptr2->prev = temp->prev;
				temp->next = ptr1;
				ptr1->prev = temp;
				temp->prev = NULL;
				head = temp;
			}
			else{
				temp = ptr2;
				ptr2 = ptr2->next;
				temp->prev->next = ptr2;
				ptr2->prev = temp->prev;
				temp->next = ptr1->next;
				ptr1->next->prev = temp;
				ptr1->next = temp;
				temp->prev = ptr1;
			}
			ptr1 = ptr2->prev;
		}
		
	}

	if(comparator(ptr2,ptr1) == 1 || comparator(ptr2,ptr1) == 0){
		return 0;
	}
	else if(comparator(ptr2,ptr1) == -1){

		while(comparator(ptr2,ptr1) == -1 && ptr1->prev!=NULL){
			ptr1 = ptr1->prev;
		}
		if(ptr1->prev == NULL && comparator(ptr2,ptr1) == -1){	
			ptr2->prev->next = NULL;	
			ptr2->next = ptr1;
			ptr1->prev = ptr2;
			ptr2->prev = NULL;
			head = ptr2;
		}
		else{			
			ptr2->prev->next = NULL;
			ptr2->next = ptr1->next;
			ptr1->next->prev = ptr2;
			ptr1->next = ptr2;
			ptr2->prev = ptr1;
		}
	}

	return 0;	
}


Node* swap(Node* head, Node* n1, Node* n2){

	Node* n1prev;
	Node* n1next;
	Node* n2prev;
	Node* n2next;
	int tog = 0;

	if(n1->next == n2){
		tog = 1;
	}

	if(n1->prev == NULL && n2->next == NULL){
		n1next = n1->next;
		n2prev = n2->prev;

		if(tog==1){
			n2->next = n1;
			n1->next = n2;
			n2->prev = NULL;
			n1->next = NULL;
			head = n2;
		}
		else{
			n2->prev = NULL;
			n1->next = NULL;
			n1next->prev = n2;
			n2->next = n1next;
			n2prev->next = n1;
			n1->prev = n2prev;
			head = n2;
		}
	}
	else if(n1->prev == NULL){
		n1next = n1->next;
		n2prev = n2->prev;
		n2next = n2->next;


		if(tog==1){
			n2->next = n1;
			n1->prev = n2;
			n2->prev = NULL;
			n1->next = n2next;
			n2next->prev = n1;
			head = n2;
		}
		else{
			n2prev->next = n1;
			n1->prev = n2prev;
			n1->next = n2next;
			n2next->prev = n1;
			n1next->prev = n2;
			n2->prev = NULL;
			n2->next = n1next;
			head = n2;
		}

	}
	else if(n2->next == NULL){
		n1prev = n1->prev;
		n1next = n1->next;
		n2prev = n2->prev;


		if(tog==1){
			n2->next = n1;
			n1->prev = n2;
			n1->next = NULL;
			n1prev->next = n2;
			n2->prev = n1prev;
		}
		else{
			n1prev->next = n2;
			n2->prev = n1prev;
			n2->next = n1next;
			n1next->prev = n2;
			n2prev->next = n1;
			n1->prev = n2prev;
			n1->next = NULL;
		}
	}
	else{
		n1prev = n1->prev;
		n1next = n1->next;
		n2prev = n2->prev;
		n2next = n2->next;	

		if(tog==1){
			n2->next = n1;
			n1->prev = n2;
			n1prev->next = n2;
			n2->prev = n1prev;
			n1->next = n2next;
			n2next->prev = n1;
		}
		else{
			n1prev->next = n2;
			n2->prev = n1prev;
			n2->next = n1next;
			n1next->prev = n2;
			n2prev->next = n1;
			n1->prev = n2prev;
			n1->next = n2next;
			n2next->prev = n1;
		}
	}

	return head;

}


Node* qSort(Node* head, Node* start, Node* tail, int (*comparator)(void*,void*)){

	if(start == NULL || start->next == NULL){
		return head;
	}

	Node* begin = head;
	Node* pivot = start;
	Node* left = start->next;
	Node* right = tail;
	Node* tempN;

	if(begin->prev != NULL){
		while(begin->prev != NULL){
			begin = begin->prev;
		}
	}


	int crossed = 0;
	int twoNode = 0;
	int lastStep = 0;

	if(start->next->inPlace == 1){
		crossed = 1;
		lastStep = 1;
	}

	if(lastStep != 1 && left == right && comparator(pivot,pivot->next) == 1){

		begin = swap(begin,pivot,left);
		start = left;

		crossed = 1;
		twoNode = 1;

	}

	while(crossed != 1 && left != right){
	

		while((comparator(left,pivot) == -1 || comparator(left,pivot) == 0) && left->next != NULL){
		

			left = left->next;
			if(left == right){
				crossed = 1;
				break;
			}
		}
		while((comparator(right,pivot) == 1 || comparator(right,pivot) == 0) && right->prev != pivot){

			right = right->prev;
			if(left == right){
				crossed = 1;
				break;
			}
		}
		if(crossed == 1){

			if(left->prev == pivot){

			}
			else if(left->next == NULL || left->next->inPlace == 1){

				if(comparator(pivot,left) == 1){
					begin = swap(begin,pivot,left);
					start = left;
				}
				else{
					tempN = left->prev;
					begin = swap(begin,pivot,left->prev);
					start = tempN;
				}

			}
			else{

				if(comparator(pivot,left) == 1){
					begin = swap(begin,pivot,left);
					start = left;
				}
				else{
					tempN = left->prev;
					begin = swap(begin, pivot, left->prev);
					start = tempN;
				}		

			}
		}

		if(crossed == 0){

			Node* temp = left;
			left = right;
			right = temp;

			begin = swap(begin, right, left);

			left = left->next;
			right = right->prev;
		
			if(left == right || left->prev == right){
			
				if(comparator(pivot,left) == 1 && left == right){
					begin = swap(begin, pivot, left);
					start = left;
				}
				else{ 
					tempN = left->prev;
					begin = swap(begin, pivot, left->prev);
					start = tempN;
				}

				crossed = 1;
			}
		}

	}


	pivot->inPlace = 1;


	tail = pivot;
	while(tail->next != NULL){
		tail = tail->next;
		if(tail->inPlace == 1){
			tail = tail->prev;
			break;
		}
	}

	if((twoNode == 1 || left == right) && left->next == NULL && left->prev == NULL){
		lastStep = 1;
	}
	else if((twoNode == 1 || left == right) && left->next == NULL && left->prev->inPlace == 1){
		lastStep = 1;
	}
	else if((twoNode == 1 || left == right) && left->next->inPlace == 1 && left->prev == NULL){
		lastStep = 1;
	}
	else if((twoNode == 1 || left == right) && left->next->inPlace == 1 && left->prev->inPlace == 1){
		lastStep = 1;
	}

	if(lastStep == 1){

		return begin;

	}

	if(pivot->prev != NULL && pivot->prev->inPlace != 1){
		qSort(begin, start, pivot->prev, comparator);
	}

	if(pivot->next != NULL && pivot->next->inPlace != 1){
		qSort(begin, pivot->next, tail, comparator);
	}

}


int quickSort(void* toSort, int (*comparator)(void*,void*)){

	Node* head = (Node*)toSort;

	if(head == NULL || head->next == NULL){
		return 0;
	}

	Node* tail = head;
	while(tail->next != NULL){
		tail = tail->next;
	}

	head = qSort(head,head,tail,comparator); 

	return 0;
}


int intComparator(void* data1, void* data2){

	Node* t1 = (Node*)data1;
	Node* t2 = (Node*)data2;

	if((t1->val) > (t2->val)){
		return 1;
	}
	else if((t1->val) < (t2->val)){
		return -1;
	}
	else{
		return 0;
	}
	
}


int strComparator(void* data1, void* data2){

	Node* t1 = (Node*)data1;
	Node* t2 = (Node*)data2;

	int i;
	int len;

	if(strlen(t1->str) > strlen(t2->str)){
		len = strlen(t2->str);
	}
	else{
		len = strlen(t1->str);
	}		

	for(i=0;i<len;i++){

		if(t1->str[i] > t2->str[i]){
			return 1;
		}
		else if(t1->str[i] < t2->str[i]){
			return -1;
		}
		else{
			continue;
		}	

	}

	if(strlen(t1->str) < strlen(t2->str)){
		return -1;
	}
	else if(strlen(t1->str) > strlen(t2->str)){
		return 1;
	}
	else{
		return 0;
	}
	
}



int main(int argc, char** argv){

	if(argc != 3){
		printf("Fatal Error: expected two arguments, had one.\n");
		return 0;
	}

	if(argv[1][0] == '-'){
		if((argv[1][1] != 'i' && argv[1][1] != 'q') || argv[1][2] != '\0'){
			printf("Fatal Error: %s is not a valid sort flag.\n", argv[1]);
			return 0;
		}
	}
	else{
		printf("Fatal Error: %s is not a valid sort flag.\n", argv[1]);
		return 0;
	}

	
	int file = open(argv[2], O_RDONLY);

	if(file < 0){
		printf("Error: file does not exist or could not open file.\n");
		close(file);
		return 0;
	}
	
	struct stat st;
	stat(argv[2], &st);
	int size = st.st_size;

	char flag = argv[1][1];

	if(size == 0){
		printf("Warning: file is empty.\n");
		close(file);
		return 0;
	}

	char buffer[size+1];
	memset(buffer,'\0',size);
	int status = 1;
	int readIn = 0;

	do{
		status = read(file, buffer+readIn, size-readIn);
		
		if(status == -1){
			printf("Error: problem with reading the file.\n");
			close(file);
			return 0;
		}

		readIn += status;
	} 
	while(status > 0 && readIn < size);

	int isInt = 0;
	int isStr = 0;
	int i;

	for(i=0; i<sizeof(buffer); i++){
		isInt = isdigit(buffer[i]);
		isStr = isalpha(buffer[i]);

		if(isInt != 0 || isStr !=0){
			break;
		}
	}		

	int blank = 0;

	if(isInt == 0 && isStr == 0){
		isStr = 1;
		blank = 1;
	}

	Node* head = NULL;
	Node* ptr = NULL;
	char temp[size+1];
	int j = 0;
	int commas = 0;
	

	for(i=0; i<sizeof(buffer); i++){

		if(buffer[i] == ','){
			commas = 1;
		}

		if(i == (sizeof(buffer) - 1) && temp[0] == '\0'){
			break;
		}

		if(buffer[i] == ',' || i == (sizeof(buffer) - 1)){
			if(isInt != 0){
				if(head == NULL){
					head = (Node*)malloc(sizeof(Node));
					if(head == NULL){
						printf("Error: malloc() is null; memory cannot be allocated.\n");
						break;
					}
					head->next = NULL;
					head->prev = NULL;
					head->val = atoi(temp);
					ptr = head;
					memset(temp,0,sizeof(buffer));
					j = 0;
				}
				else{
					Node* tempNode = (Node*)malloc(sizeof(Node));
					if(tempNode == NULL){
						printf("Error: malloc() is null; memory cannot be allocated.\n");
						break;
					}
					ptr->next = tempNode;
					tempNode->prev = ptr;
					tempNode->val = atoi(temp);
					tempNode->next = NULL;
					ptr = tempNode;
					memset(temp,0,sizeof(buffer));
					j = 0;
				}
			}
			else if(isStr != 0){

				if(head == NULL){
					head = (Node*)malloc(sizeof(Node));
					if(head == NULL){
						printf("Error: malloc() is null; memory cannot be allocated.\n");
						break;
					}
					head->next = NULL;
					head->prev = NULL;
					head->str = (char*)malloc(strlen(temp));
					if(head->str == NULL){
						printf("Error: malloc() is null; memory cannot be allocated.\n");
						break;
					}
					strcpy(head->str, temp);
					ptr = head;
					memset(temp,0,sizeof(buffer));
					j = 0;
				}
				else{		
					Node* tempNode = (Node*)malloc(sizeof(Node));
					if(tempNode == NULL){
						printf("Error: malloc() is null; memory cannot be allocated.\n");
						break;
					}
					ptr->next = tempNode;
					tempNode->prev = ptr;
					tempNode->str = (char*)malloc(strlen(temp));
					if(tempNode->str == NULL){
						printf("Error: malloc() is null; memory cannot be allocated.\n");
						break;
					}
					strcpy(tempNode->str, temp);
					tempNode->next = NULL;
					ptr = tempNode;
					memset(temp,0,sizeof(buffer));
					j = 0;
				}
			}
				
			continue;		
		}

		if((buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\t')){
			continue;
		}
		
		else{
			temp[j]=buffer[i];
			j++;
		}
		
	}

	if(commas == 0 && blank == 1){
		printf("Warning: file is empty.\n");
		close(file);
		return 0;
	}

	Node* tempptr = head;

	while(tempptr != NULL){
		tempptr->inPlace = 0;
		tempptr = tempptr->next;
	}
	

	int (*intComp)(void*,void*) = intComparator;
	int (*strComp)(void*,void*) = strComparator;

	if(flag == 'i' && isInt != 0){
		insertionSort(head, intComp);
	}
	else if(flag == 'i' && isStr != 0){
		insertionSort(head, strComp);
	}
	else if(flag == 'q' && isInt != 0){
		quickSort(head, intComp);
	}
	else if(flag == 'q' && isStr != 0){
		quickSort(head, strComp);
	}

	while(head->prev != NULL){
		head = head->prev;
	}

	Node* curr = head;

	while(curr != NULL){
		if(isInt != 0){
			printf("%d\n",curr->val);
		}
		else if(isStr != 0){
			printf("%s\n",curr->str);
		}
		curr = curr->next;
	}

	curr = head;

	while(curr != NULL){
		curr = head->next;

		if(isStr != 0){
			free(head->str);
		}
		free(head);
		head = curr;
	}

	close(file);

	return 0;
}
