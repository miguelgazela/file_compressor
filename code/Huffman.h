#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include "CompressionAlgorithm.h"

class BufferedByte;

class _htNode {
public:
	unsigned char symbol;
	unsigned int frequency;
	_htNode *left, *right;
	_htNode();
};

struct CompareNode : public std::binary_function<_htNode*, _htNode*, bool>                                                                                     
{
  bool operator()(const _htNode* lhs, const _htNode* rhs) const
  {
    return lhs->frequency > rhs->frequency;
  }
};

class _htTree {
public:
	_htNode *root;
	unsigned int numNodes;
	_htTree();
};

class _hlNode {
public:
	unsigned char symbol;
	unsigned int value;
	unsigned int numberBitsOfCode;
	_hlNode *next;
};

class _hlTable {
public:
	_hlNode *first;
	_hlNode *last;
	_hlTable();
};


class Huffman: public CompressionAlgorithm
{
	static const string algorithm_name;
	static const string extension;

protected:
	_htTree* huffmanTree;
	void deleteTree(_htNode* node);

public:
	string getAlgorithmName() const { return algorithm_name; }
	string getCompressedFilesExtension() const { return extension; }
	virtual ~Huffman() {}
};

class Huffman_compressor: public Huffman
{
	int char_Frequency[NUM_VALUES_8BYTES];
	_hlTable* huffmanTable;
	void buildTable();
	void traverseTree(_htNode *treeNode, _hlTable **table, int k,unsigned int value, unsigned int numberBits);
	void postOrder(_htNode* node, FILE* ofile, BufferedByte &byte);
	void deleteTable(_hlNode* node);

public:
	Huffman_compressor();
	~Huffman_compressor() { reset(); }
	bool apply(FILE *ifile, FILE *ofile);
	void reset();
};


class Huffman_decompressor: public Huffman
{
public:
	Huffman_decompressor();
	~Huffman_decompressor() { reset(); }
	bool apply(FILE *ifile, FILE *ofile);
	void reset();
};

#endif
