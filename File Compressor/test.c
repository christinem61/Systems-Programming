#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef struct Node{

	char* str;
	char* code;
	int freq;
	int height;
	char bf;
	struct Node* parent;
	struct Node* left;
	struct Node* right;

} Node;

int createArr(Node** arr, Node* input, int index){

	if(input == NULL){
		return index;
	}

	Node* temp = NULL;
	*(&temp) = input;
	arr[index++] = input;
	
	index = createArr(arr, input->left, index);
	index = createArr(arr, input->right, index);

}

void fill(Node* root){
	
	if(root == NULL){
		return;
	}
	
	fill(root->left);
	fill(root->right);
	
	root->height = -1;
	
	if(root->left != NULL){
		root->height = root->left->height;
	}
	if(root->right != NULL){
		if(root->height < root->right->height){
			root->height = root->right->height;
		}
	}

	root->height++;

	if(root->right != NULL && root->left != NULL){
		int balance = root->right->height - root->left->height;
		if(balance < 0){
			root->bf = '/';
		}
		else if(balance > 0){
			root->bf = '\\';
		}
		else{
			root->bf = '-';
		}
	}
	else if(root->right != NULL && root->left == NULL){
		root->bf = '\\';
	}
	else if(root->right == NULL && root->left != NULL){
		root->bf = '/';
	}
	else{
		root->bf = '-';
	}
	
}


Node* balance1(Node* root, Node* x, Node* r){

	if(x->parent == NULL){

		r->parent = NULL;
		root = r;

		if(x->bf == '\\'){

			if(r->left != NULL){
				Node* t = r->left;
				r->left = x;
				x->parent = r;
				x->right = t;
				x->right->parent = x;
			}
			else{
				r->left = x;
				x->parent = r;
				x->right = NULL;
			}
		}
		else{
			if(r->right != NULL){
				Node* t = r->right;
				r->right = x;
				x->parent = r;
				x->left = t;
				x->left->parent = x;
			}
			else{
				r->right = x;
				x->parent = r;
				x->left = NULL;
			}
		}	
	}
	else{

		if(x->parent->left == x){
			x->parent->left = r;
		}
		else{
			x->parent->right = r;
		}
		r->parent = x->parent;

		if(x->bf == '\\'){
			if(r->left != NULL){
				Node* t = r->left;
				r->left = x;
				x->parent = r;
				x->right = t;
				x->right->parent = x;
			}
			else{
				r->left = x;
				x->parent = r;
				x->right = NULL;
			}
		}
		else{
			if(r->right != NULL){
				Node* t = r->right;
				r->right = x;
				x->parent = r;
				x->left = t;
				x->left->parent = x;
			}
			else{
				r->right = x;
				x->parent = r;
				x->left = NULL;
			}
		}
	}

	r->bf = '-';

	if(x->left == NULL && x->right == NULL){
		x->height = 0;
		x->bf = '-';
	}
	else if(x->left == NULL && x->right != NULL){
		x->height = x->right->height + 1;
		x->bf = '\\';
	}
	else if(x->left != NULL && x->right == NULL){
		x->height = x->left->height + 1;
		x->bf = '/';
	}
	else{
		x->height = x->left->height + 1;
		x->bf = '-';
	}

	return root;
}


Node* balance2(Node* root, Node* x, Node* r){

	Node* q;	

	if(r->bf == '\\'){
		q = r->right;
	}
	else if(r->bf == '/'){
		q = r->left;
	}
	
	if(r->parent->left == r){
		r->parent->left = q;
	}
	else{
		r->parent->right = q;
	}
	
	q->parent = r->parent;

	if(r->bf == '\\'){
		if(q->left != NULL){
			Node* t = q->left;
			q->left = r;
			r->parent = q;
			r->right = t;
			r->right->parent = r;
		}
		else{
			q->left = r;
			r->parent = q;
			r->right = NULL;
		}
	}
	else{
		if(q->right != NULL){
			Node* t = q->right;
			q->right = r;
			r->parent = q;
			r->left = t;
			r->left->parent = r;
		}
		else{
			q->right = r;
			r->parent = q;
			r->left = NULL;
		}
	}

	fill(root);
	root = balance1(root, x, q);
	return root;
}


int isBalanced(Node* root){

	if(root == NULL){
		return 1;
	}

	int balance;

	if(root->right != NULL && root->left != NULL){
		if(root->bf == '\\'){
			balance = root->height - (root->left->height + 1);
		}
		else if(root->bf == '/'){
			balance = root->height - (root->right->height + 1);
		}
		else{
			balance = 0;
		}		
	}
	else if(root->right != NULL && root->left == NULL){
		balance = root->height;
	}
	else if(root->right == NULL && root->left != NULL){
		balance = root->height;
	}
	else{
		return 1;
	}

	if(balance <= 1 && isBalanced(root->right) && isBalanced(root->left)){
		return 1;
	}

	return 0;

}

Node* insert(Node* root, char* word){

	Node* ptr = root;
	Node* parent;
	char dir = 'l';

	while(ptr!=NULL){
		if(strcmp(ptr->str,word) > 0){
			parent = ptr;
			ptr = ptr->left;
			dir = 'l';
		}
		else if(strcmp(ptr->str,word) < 0){
			parent = ptr;
			ptr = ptr->right;
			dir = 'r';
		}
	}

	Node* tempN = (Node*)malloc(sizeof(Node));
	tempN->str = (char*)malloc(strlen(word));
	strcpy(tempN->str, word);
	tempN->freq = 1;
	tempN->left = NULL;
	tempN->right = NULL;
	tempN->parent = parent;
	
	if(dir == 'l'){
		parent->left = tempN;
	}
	else if(dir == 'r'){
		parent->right = tempN;
	}

	fill(root);

	if(isBalanced(root) == 0){
		Node* ptr = tempN->parent;

		while(isBalanced(ptr) == 1){
			ptr = ptr->parent;
		}

		if(ptr->bf == '\\'){
			if(ptr->right->bf == '\\'){
				root = balance1(root,ptr,ptr->right);
				fill(root);
			}
			else if(ptr->right->bf == '/'){
				root = balance2(root,ptr,ptr->right);
				fill(root);
			}
		}
		else if(ptr->bf == '/'){
			if(ptr->left->bf == '/'){
				root = balance1(root,ptr,ptr->left);
				fill(root);
			}
			else if(ptr->left->bf == '\\'){				
				root = balance2(root,ptr,ptr->left);
				fill(root);
			}
		}
	}

	return root;	
}

int searchAVL(Node* root, char* word){

	Node* ptr = root;
	
	while(ptr != NULL){
		if(strcmp(word, ptr->str) == 0){
			ptr->freq++;
			return 1;
		}
		else if(strcmp(word, ptr->str) > 0){
			ptr = ptr->right;
		}
		else if(strcmp(word, ptr->str) < 0){
			ptr = ptr->left;
		}
	}

	return -1;

}

void minHeap(Node** arr, int start, int end){

	int i;

	for(i = ((start+end-1)/2); i >= start; i--){
		int k = i;
		int l = 2*k+1-start;
	
		while(l <= end){
			int min = l;
			int r = l+1;
			if(r <= end){
				if(arr[r]->freq < arr[l]->freq){
					min++;
				}
			}
			if(arr[min]->freq < arr[k]->freq){
				Node* temp = arr[k];
				arr[k] = arr[min];
				arr[min] = temp;
				k = min;
				l = 2*k+1-start;
			}
			else{
				break;
			}
		}
	}

}

void huffmanTree(Node** arr, int ct){

	int i;
	for(i=0; i<ct; i++){
		arr[i]->left = NULL;
		arr[i]->right = NULL;
		arr[i]->parent = NULL;
	}

	Node* one;
	Node* two;
	int j = 0;

	for(i=ct; i>1; i--){
		one = arr[j];
		j++;
		minHeap(arr, j, ct-1);
		two = arr[j];
		Node* head = (Node*)malloc(sizeof(Node));
		head->freq = one->freq + two->freq;
		head->left = one;
		head->right = two;
		head->str = NULL;
		one->parent = head;
		two->parent = head;
		one->code = NULL;
		two->code = NULL;
		head->code = NULL;
		arr[j] = head;
		minHeap(arr, j, ct-1);
	}
}

void huffmanCodes(Node* rootLeaf, char dir){

	if(rootLeaf == NULL){
		return;
	}

	char* codeStr;

	if(dir == 'l'){
		if(rootLeaf->parent->code == NULL){
			rootLeaf->code = (char*)malloc(1);
			strcpy(rootLeaf->code, "0");
		}
		else{
		 	codeStr = (char*)malloc(strlen(rootLeaf->parent->code));
			strcpy(codeStr, rootLeaf->parent->code);
			strcat(codeStr, "0");
			rootLeaf->code = (char*)malloc(strlen(codeStr));
			strcpy(rootLeaf->code, codeStr);		
			free(codeStr);
		}
	}
	else if(dir == 'r'){
		if(rootLeaf->parent->code == NULL){
			rootLeaf->code = (char*)malloc(1);
			strcpy(rootLeaf->code, "1");
		}
		else{

			codeStr = (char*)malloc(strlen(rootLeaf->parent->code));
			strcpy(codeStr, rootLeaf->parent->code);			
			strcat(codeStr, "1");
			rootLeaf->code = (char*)malloc(strlen(codeStr));
			strcpy(rootLeaf->code, codeStr);
			free(codeStr);		
		}
	}

	huffmanCodes(rootLeaf->left, 'l');
	huffmanCodes(rootLeaf->right, 'r');

}

Node* insertCode(Node* root, char* word){

	Node* ptr = root;
	Node* parent;
	char dir = 'l';

	while(ptr!=NULL){
		if(strcmp(ptr->code,word) > 0){
			parent = ptr;
			ptr = ptr->left;
			dir = 'l';
		}
		else if(strcmp(ptr->code,word) < 0){
			parent = ptr;
			ptr = ptr->right;
			dir = 'r';
		}
	}

	Node* tempN = (Node*)malloc(sizeof(Node));
	tempN->code = (char*)malloc(strlen(word));
	strcpy(tempN->code, word);
	tempN->left = NULL;
	tempN->right = NULL;
	tempN->parent = parent;
	
	if(dir == 'l'){
		parent->left = tempN;
	}
	else if(dir == 'r'){
		parent->right = tempN;
	}

	fill(root);

	if(isBalanced(root) == 0){
		Node* ptr = tempN->parent;

		while(isBalanced(ptr) == 1){
			ptr = ptr->parent;
		}

		if(ptr->bf == '\\'){
			if(ptr->right->bf == '\\'){
				root = balance1(root,ptr,ptr->right);
				fill(root);
			}
			else if(ptr->right->bf == '/'){
				root = balance2(root,ptr,ptr->right);
				fill(root);
			}
		}
		else if(ptr->bf == '/'){
			if(ptr->left->bf == '/'){
				root = balance1(root,ptr,ptr->left);
				fill(root);
			}
			else if(ptr->left->bf == '\\'){
				root = balance2(root,ptr,ptr->left);
				fill(root);
			}
		}
	}

	return root;
	
}

Node* searchCodebookAVL(Node* root, char* codeWord){

	Node* ptr = root;
	
	while(ptr != NULL){
		if(strcmp(codeWord, ptr->code) == 0){
			return ptr;
		}
		else if(strcmp(codeWord, ptr->code) > 0){
			ptr = ptr->right;
		}
		else if(strcmp(codeWord, ptr->code) < 0){
			ptr = ptr->left;
		}
	}

}

Node* temp;

Node* getCode(Node* root, char* word){

	if(strcmp(word, root->str) == 0){
		temp = root;
	}
	else{
		if(root->left != NULL){
			getCode(root->left, word);
		}
		if(root->right != NULL){
			getCode(root->right, word);
		}
	}
	
	return temp;

}

Node* temp1 = NULL;

Node* getStr(Node* root, char* word, int ct){

	if(ct == 1){
		temp1 = NULL;
		ct++;
	}

	if(strcmp(word, root->code) == 0){
		temp1 = root;
	}
	else{
		if(root->left != NULL){
			getStr(root->left, word, ct);
		}
		if(root->right != NULL){
			getStr(root->right, word, ct);
		}
	}
	
	return temp1;

}

