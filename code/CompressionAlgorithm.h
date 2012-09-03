#ifndef COMPRESSIONALGORITHM_H_
#define COMPRESSIONALGORITHM_H_

#include <string>

#define BUFFER_SIZE 524288  // 512 KB
#define NUM_VALUES_8BYTES 256
#define MAX_VALUE_UNSIGNED_8BYTES (NUM_VALUES_8BYTES-1)

using namespace std;

#define COMPRESSION
#define DECOMPRESSION

class CompressionAlgorithm
{
public:
	virtual bool apply(FILE * ifile, FILE * ofile) = 0;
	virtual string getAlgorithmName() const = 0;
	virtual string getCompressedFilesExtension() const = 0;
	virtual void reset() = 0;
	virtual ~CompressionAlgorithm() {}
};

#endif