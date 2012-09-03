#ifndef BUFFEREDBYTE_H_
#define BUFFEREDBYTE_H_

#include <iostream>

#define BYTE_SIZE 8
#define BUFFEREDBYTE_MAX_INDEX (BYTE_SIZE-1)

using namespace std;

class FinishedReading {};
class FailedToRead {};

class BufferedByte
{
	unsigned char buffer;
	char lastOccupiedBit;  // counting from the left

	static BufferedByte readNumber_aux(FILE * file, BufferedByte previousBB, unsigned int & number, unsigned int numBits, unsigned long long & bytes_read);

public:
	BufferedByte(): lastOccupiedBit(-1), buffer(0) {}
	bool isEmpty() const { return lastOccupiedBit == -1; }
	unsigned int nBits() const { return lastOccupiedBit+1; }  // Returns the number of filled bits in this BufferedByte
	unsigned char getBuffer() const { return buffer; }

	static unsigned int mininumBitsNeeded(unsigned int number);
	static BufferedByte writeNumber(FILE * file, BufferedByte previousBB, unsigned int number, unsigned int numBits);
	static BufferedByte readNumber(FILE * file, BufferedByte previousBB, unsigned int & number, unsigned int numBits, unsigned long long & bytes_read);
};

#endif
