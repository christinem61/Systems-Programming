#ifndef TEST_H
#define TEST_H

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

void createArr(Node**, Node*, int);

void fill(Node*);

Node* balance1(Node*, Node*, Node*);

Node* balance2(Node*, Node*, Node*);

int isBalanced(Node*);

Node* insert(Node*, char*);

int searchAVL(Node*, char*);

void minHeap(Node**, int, int);

void huffmanTree(Node**, int);

void huffmanCodes(Node*, char);

Node* insertCode(Node*, char*);

Node* searchCodebookAVL(Node*, char*);

Node* getCode(Node*, char*);

Node* getStr(Node*, char*, int);

#endif
