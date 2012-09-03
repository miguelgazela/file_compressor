#ifndef LZW_H_
#define LZW_H_

#include "CompressionAlgorithm.h"
#include <map>

class LZW: public CompressionAlgorithm
{
	static const string algorithm_name;
	static const string extension;
protected:
	unsigned int lastCodeUsed;
public:
	string getAlgorithmName() const { return algorithm_name; }
	string getCompressedFilesExtension() const { return extension; }
	virtual ~LZW() {}
};

class LZW_compressor: public LZW
{
	map<string, unsigned int> dictionary;
	map<string, unsigned int>::iterator it;
public:
	LZW_compressor();  // initializes dictionary with the 256 ASCII characters
	~LZW_compressor() {}
	bool apply(FILE * ifile, FILE * ofile);
	void reset();
};

class LZW_decompressor: public LZW
{
	map<unsigned int, string> dictionary;
	map<unsigned int, string>::iterator it;
public:
	LZW_decompressor();  // initializes dictionary with the 256 ASCII characters
	~LZW_decompressor() {}
	bool apply(FILE * ifile, FILE * ofile);
	void reset();
};
#endif