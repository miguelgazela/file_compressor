#include "Huffman.h"
#include "Display.h"
#include <iostream>
#include <queue>
#include <stack>
#include <sys/stat.h>  // fstat
#include "BufferedByte.h"

#define BIT(n) (0x01<<n)

using namespace std;

const string Huffman::algorithm_name = "HUFFMAN";
const string Huffman::extension = ".huff";

_htNode::_htNode() 
{
	left = NULL;
	right = NULL;
}

_htTree::_htTree() 
{
	root = NULL;
}

_hlTable::_hlTable()
{
	first = NULL;
	last = NULL;
}

Huffman_compressor::Huffman_compressor() 
{
	//initialize the char frequency array 
	for(int i=0; i<NUM_VALUES_8BYTES; i++)
		char_Frequency[i]=0;

	huffmanTree = NULL;
	huffmanTable = NULL;
}

void Huffman_compressor::reset()
{
	//initialize the char frequency array
	for(int i=0; i<NUM_VALUES_8BYTES; i++)
		char_Frequency[i]=0;

	if (huffmanTree != NULL)
	{
		deleteTree(huffmanTree->root);
		delete huffmanTree;
		huffmanTree = NULL;
	}

	if (huffmanTable != NULL) {
		deleteTable(huffmanTable->first);
		delete huffmanTable;
		huffmanTable = NULL;
	}
}

void Huffman::deleteTree(_htNode* node) 
{
	if (node == NULL)
		return;
	deleteTree(node->left);
	deleteTree(node->right);
	delete node;
}

void Huffman_compressor::deleteTable(_hlNode* node)
{
	if (node == huffmanTable->last) {
		delete node;
		return;
	}
	deleteTable(node->next);
	delete node;
}

bool Huffman_compressor::apply(FILE *ifile, FILE *ofile)
{
	unsigned char c;
	_hlNode *traversal;
	BufferedByte byte;
	unsigned int numBits = 8, numNodes = 0;
	struct stat ifile_stat;
    fstat(_fileno(ifile), &ifile_stat);
	unsigned long long ifile_size = ifile_stat.st_size;

	setvbuf(ifile, NULL, _IOFBF, BUFFER_SIZE);
	setvbuf(ofile, NULL, _IOFBF, BUFFER_SIZE);

	if (ifile_size == 0)
	{
		writePercentageAndHyphens(100, 100);
		cout << endl;
		return true;
	}

	while(!feof(ifile))
	{
		if (fread(&c, sizeof(char), 1, ifile) != 1) // read one character
		{
			if (feof(ifile))
				break;
			cout << "Erro ao ler o ficheiro a comprimir.\n";
			return false;
		}

		//consider the symbol as an array index and increase the number of times the symbol appears
		char_Frequency[c]++;
	}

	// build a tree node for each character in the file
	priority_queue<_htNode*,vector<_htNode*>, CompareNode> htNodeQueue;

	for(unsigned int i = 0; i < NUM_VALUES_8BYTES; i++) {
		if (char_Frequency[i] != 0)
		{
			_htNode *aux = new _htNode();
			numNodes++;
			aux->symbol = (unsigned char) i;
			aux->frequency = char_Frequency[i];
			htNodeQueue.push(aux);
		}
	}

	// build the tree
	while(htNodeQueue.size() != 1)
	{
		_htNode *left = htNodeQueue.top(); htNodeQueue.pop();
		_htNode *right = htNodeQueue.top(); htNodeQueue.pop();

		_htNode *newNode = new _htNode;
		numNodes++;
		newNode->left = left;
		newNode->right = right;
		newNode->frequency = left->frequency + right->frequency;

		htNodeQueue.push(newNode);
	}

	huffmanTree = new _htTree();
	huffmanTree->root = htNodeQueue.top(); htNodeQueue.pop();
	huffmanTree->numNodes = numNodes;
	
	// build a table to improve encode
	buildTable();

	// initialize progress bar
	rewind(ifile);
	switchCursorVisibility(false);
	cout << "Progresso:\n|";
    unsigned long long bytes_read = 0;
	unsigned int last_percentage = 0;
	
	writePercentageAndHyphens(0, 0);

	// write the key to decompress later, the number of nodes in the tree, and the nodes
	byte = BufferedByte::writeNumber(ofile, byte, 0, 8);
	byte = BufferedByte::writeNumber(ofile, byte, huffmanTree->numNodes, 16);  // numNodes is in [0, 2*256-1=511]: 9 bits needed. Write 16 to align the bytes.
	postOrder(huffmanTree->root, ofile, byte);

	while(!feof(ifile))
	{
		if (fread(&c, sizeof(char), 1, ifile) != 1)  // read one character
		{
			if (feof(ifile))
				break;
			writeErrorMessageOnTopOfProgressBar("Erro ao ler o ficheiro a comprimir.\n");
			fflush(ofile);
			switchCursorVisibility(true);
			return false;
		}

		bytes_read++;
		last_percentage = updateProgressBar(bytes_read, ifile_size, last_percentage);

		//For each element of the file traverse the table
		//and once the symbol is found, output the code for it

		traversal = huffmanTable->first;
		while(traversal->symbol != c)
			traversal = traversal->next;

		if (traversal->symbol != c && traversal->symbol == huffmanTable->last->symbol)
		{
			cout << "Erro ao comprimir o ficheiro.\n";
			return false;
		}
		byte = BufferedByte::writeNumber(ofile, byte, traversal->value, traversal->numberBitsOfCode);
	}

	if (!byte.isEmpty())  // if byte is not empty, it's because the last call to writeNumber() could not fill an entire byte to write to file
	{
		unsigned char lastIncompleteByte = byte.getBuffer();
		fwrite(&lastIncompleteByte, sizeof(char), 1, ofile);
		rewind(ofile);
		unsigned char nBits = byte.nBits();
		fwrite(&nBits,sizeof(char),1,ofile);
	}

	cout << endl;
	fflush(ofile);
	switchCursorVisibility(true);
	return true;
}

void Huffman_compressor::traverseTree(_htNode *treeNode, _hlTable **table, int k, unsigned int value, unsigned int numberBits)
{
	//If the end is reached, introduce the code in the table
	if (treeNode->left == NULL && treeNode->right == NULL)
	{
		_hlNode *aux = new _hlNode();
		aux->symbol = treeNode->symbol;
		aux->next = NULL;
		aux->value = value;
		if (numberBits == 0) // if there's only 1 character
			aux->numberBitsOfCode = 1;
		else
			aux->numberBitsOfCode = numberBits;
		
		if((*table)->first == NULL)
		{
			(*table)->first = aux;
			(*table)->last = aux;
		}
		else
		{
			(*table)->last->next = aux;
			(*table)->last = aux;
		}
	}
	
	if(treeNode->left!=NULL)
		traverseTree(treeNode->left,table,k+1,(value << 1),numberBits+1);
	if(treeNode->right!=NULL)
		traverseTree(treeNode->right,table,k+1,(value << 1)+1,++numberBits);
}

void Huffman_compressor::buildTable()
{
	//initialize the table
	huffmanTable = new _hlTable();

	//Auxiliary variables
	int k=0; //k will memorize the level on which the traversal is

	//traverse the tree and calculate the codes
	traverseTree(huffmanTree->root,&huffmanTable,k,0,0);
}

void Huffman_compressor::postOrder(_htNode* node, FILE* ofile, BufferedByte &byte)
{
	if (node == NULL)
		return;
	postOrder(node->left, ofile, byte);
	postOrder(node->right, ofile, byte);
	if(node->left == NULL && node->right == NULL) { // it's a leaf
		byte = BufferedByte::writeNumber(ofile, byte, 1, 1);
		byte = BufferedByte::writeNumber(ofile, byte, (unsigned int)node->symbol, 8);
	}
	else  // it's not leaf
		byte = BufferedByte::writeNumber(ofile, byte, 0, 1);
}

Huffman_decompressor::Huffman_decompressor()
{
	huffmanTree = NULL;
}

void Huffman_decompressor::reset()
{
	if (huffmanTree != NULL)
	{
		deleteTree(huffmanTree->root);
		delete huffmanTree;
		huffmanTree = NULL;
	}
}

bool Huffman_decompressor::apply(FILE * ifile, FILE * ofile) 
{
	unsigned int code, numNodes, numNodesCpy, realBitsLastChar, usedRealBits = 0;
	bool inLastByte = false;
	BufferedByte byte;

	if (feof(ifile))
		return true;

	setvbuf(ifile, NULL, _IOFBF, BUFFER_SIZE);
	setvbuf(ofile, NULL, _IOFBF, BUFFER_SIZE);

	// Initialize progress bar
	switchCursorVisibility(false);
	cout << "Progresso:\n|";
	struct stat ifile_stat;
    fstat(_fileno(ifile), &ifile_stat);
    unsigned long long ifile_size = ifile_stat.st_size, bytes_read = 0;
	unsigned int last_percentage = 0;
	if (ifile_size == 0)
	{
		writePercentageAndHyphens(100, 100);
		cout << endl;
		return true;
	}

	writePercentageAndHyphens(0, 0);

	// read the key to create the huffman tree
	try { byte = BufferedByte::readNumber(ifile, byte, realBitsLastChar, 8, bytes_read); }
	catch (FailedToRead &) { writeErrorMessageOnTopOfProgressBar("Erro ao ler o ficheiro comprimido.\n"); switchCursorVisibility(true); return false; }

	try { byte = BufferedByte::readNumber(ifile, byte, numNodes, 16, bytes_read); }  // numNodes is in [0, 2*256-1=511]: 9 bits needed. Write 16 to align the bytes.
	catch (FailedToRead &) { writeErrorMessageOnTopOfProgressBar("Erro ao ler o ficheiro comprimido.\n"); switchCursorVisibility(true); return false; }

	unsigned char value = 0, bits2Read = 9;
	int i;
	stack<_htNode*> nodeStack;
	numNodesCpy = numNodes;

	while(numNodes != 0)
	{
		try { byte = BufferedByte::readNumber(ifile, byte, code, 8, bytes_read); }
		catch (FailedToRead &) { writeErrorMessageOnTopOfProgressBar("Erro ao ler o ficheiro comprimido.\n"); fflush(ofile); switchCursorVisibility(true); return false; }
		catch (FinishedReading &)
		{
			last_percentage = updateProgressBar(bytes_read, ifile_size, last_percentage);
			break;
		}
		
		if(bytes_read == ifile_size)
			inLastByte = true;

		for(i = 7; i >= 0; i--) 
		{
			if (inLastByte)
				usedRealBits++;
				
			if(bits2Read == 9) { // it's reading the node
				if ((BIT(i) & code) > 0) // it's a leaf
					bits2Read--;
				else {
					numNodes--;
					_htNode* notLeaf = new _htNode;
					notLeaf->right = nodeStack.top(); nodeStack.pop();
					notLeaf->left = nodeStack.top(); nodeStack.pop();
					nodeStack.push(notLeaf);
				}
			}
			else if(bits2Read == 1) { // it's the last bit to create a leaf
				if((BIT(i) & code) > 0)
					value = (value << 1) + 1;
				else
					value = value << 1;
				_htNode* leaf = new _htNode;
				leaf->symbol = value;
				value = 0;
				nodeStack.push(leaf);
				bits2Read = 9;
				numNodes--;
			}
			else {
				if((BIT(i) & code) > 0)
					value = (value << 1) + 1;
				else
					value = value << 1;
				bits2Read--;
			}

			if(numNodes == 0) // to save the current index in case the byte has bits left
				break;
		}
	}

	huffmanTree = new _htTree();
	huffmanTree->root = nodeStack.top(); nodeStack.pop();
	_htNode* atual = huffmanTree->root;

	if (i > 0)
	{
		i--;
		if(!inLastByte)
		{
			for(; i >= 0; i--) 
			{
				if(numNodesCpy == 1)  // tree has only one node (one character coded as 0)
				{
					if ((BIT(i) & code) != 0) { writeErrorMessageOnTopOfProgressBar("Erro ao ler o ficheiro comprimido.\n"); fflush(ofile); switchCursorVisibility(true); return false; }
					fwrite(&atual->symbol,sizeof(char),1,ofile);
				}
				else 
				{
					if((BIT(i) & code) != 0)
						atual = atual->right;
					else
						atual = atual->left;

					if(atual->left == NULL && atual->right == NULL)
					{
						fwrite(&atual->symbol,sizeof(char),1,ofile);
						atual = huffmanTree->root;
					}
				}
			}
		}
		else
		{
			for(; i >= 0 && usedRealBits < realBitsLastChar; i--, usedRealBits++)
				if(numNodesCpy == 1)
				{
					if ((BIT(i) & code) != 0) { writeErrorMessageOnTopOfProgressBar("Erro ao ler o ficheiro comprimido.\n"); fflush(ofile); switchCursorVisibility(true); return false; }
					fwrite(&atual->symbol,sizeof(char),1,ofile);
				}
				else 
				{
					if((BIT(i) & code) != 0)
						atual = atual->right;
					else
						atual = atual->left;

					if(atual->left == NULL && atual->right == NULL) 
					{
						fwrite(&atual->symbol,sizeof(char),1,ofile);
						atual = huffmanTree->root;
					}
				}
			last_percentage = updateProgressBar(bytes_read, ifile_size, last_percentage);
		}
	}

	if(!inLastByte) 
	{
		while(!feof(ifile))
		{
			int min = -1;

			try { byte = BufferedByte::readNumber(ifile, byte, code, 8, bytes_read); }
			catch (FailedToRead &) { writeErrorMessageOnTopOfProgressBar("Erro ao ler o ficheiro comprimido.\n"); fflush(ofile); switchCursorVisibility(true); return false; }
			catch (FinishedReading &)
			{
				last_percentage = updateProgressBar(bytes_read, ifile_size, last_percentage);
				break;
			}
			
			if(bytes_read == ifile_size)
				inLastByte = true;

			last_percentage = updateProgressBar(bytes_read, ifile_size, last_percentage);

			if(!inLastByte || realBitsLastChar == 0)
				min = 0;
			else
				min = (8 - realBitsLastChar);

			for(int i = 7; i >= min; i--)
				if(numNodesCpy == 1)
					fwrite(&atual->symbol,sizeof(char),1,ofile);
				else 
				{
					if((BIT(i) & code) != 0) {
						atual = atual->right;
					}
					else
						atual = atual->left;
	
					if(atual->left == NULL && atual->right == NULL) {
						fwrite(&atual->symbol,sizeof(char),1,ofile);
						atual = huffmanTree->root;
					}
				}
		}
	}
	
	cout << endl;
	fflush(ofile);
	switchCursorVisibility(true);
	return true;
}
