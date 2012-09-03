#include "BufferedByte.h"

BufferedByte BufferedByte::readNumber_aux(FILE * file, BufferedByte previousBB, unsigned int & number, unsigned int numBits, unsigned long long & bytes_read)
{
	if (numBits == 0)
		return previousBB;

	unsigned char c;
	char nBitsToBeReadFromBB = numBits < BYTE_SIZE ? numBits : BYTE_SIZE;  // number of bits from previousBB that are going to be inserted in number (and are a part of number)
	if (nBitsToBeReadFromBB > (previousBB.lastOccupiedBit+1))  // if number of bits to be read is greater than the number of bits in the buffered byte
	{
		if (fread(&c, sizeof(unsigned char), 1, file) != 1)  // read one more byte
			if (feof(file))
				throw FinishedReading();
			else
				throw FailedToRead();

		bytes_read++;
		previousBB.buffer |= c >> (previousBB.lastOccupiedBit+1);  // fill the buffered byte with as many bits from c as possible (starting at the left most bit of c): buffer will always be completed (8 bits) after this
	}

	number <<= nBitsToBeReadFromBB;  // create "space" for nBitsToBeReadFromBB bits
	number |= previousBB.buffer >> (BYTE_SIZE - nBitsToBeReadFromBB);  // insert nBitsToBeReadFromBB bits from buffer (starting at the left most bit of buffer)
	previousBB.buffer <<= nBitsToBeReadFromBB;  // erase those bits from buffer
	if (nBitsToBeReadFromBB > (previousBB.lastOccupiedBit+1))  // if a byte was read from the file in this call
	{
		previousBB.buffer |= ((unsigned char) (c << (BUFFEREDBYTE_MAX_INDEX-previousBB.lastOccupiedBit))) >> (BYTE_SIZE-nBitsToBeReadFromBB);  // store the rest of the bits of c (the ones not previously inserted into buffer) in the buffer
		previousBB.lastOccupiedBit = BYTE_SIZE - nBitsToBeReadFromBB + previousBB.lastOccupiedBit;  // update lastOccupiedBit correctly
	}
	else
		previousBB.lastOccupiedBit -= nBitsToBeReadFromBB;  // update lastOccupiedBit correctly

	return readNumber_aux(file, previousBB, number, numBits-nBitsToBeReadFromBB, bytes_read);  // number may not be fully read
}

unsigned int BufferedByte::mininumBitsNeeded(unsigned int number)
{
	if (number == 0)
		return 1;
	int count = 0;
	while (number != 0)
	{
		number >>= 1;
		count++;
	}
	return count;
}

BufferedByte BufferedByte::writeNumber(FILE * file, BufferedByte previousBB, unsigned int number, unsigned int numBits)
{
	if (numBits == 0)
		return previousBB;

	char nBitsAvailableInBB = BUFFEREDBYTE_MAX_INDEX - previousBB.lastOccupiedBit;  // number of free bits of previousBB
	char nBitsToBeInsertedInBB = numBits < (unsigned) nBitsAvailableInBB ? numBits : nBitsAvailableInBB;  // number of bits to be inserted in previousBB that are a part of the number that is being written

	previousBB.buffer |= (number >> (numBits-nBitsToBeInsertedInBB)) << (nBitsAvailableInBB-nBitsToBeInsertedInBB);  // insert nBitsToBeInsertedInBB bits of number (starting at the left most bit of number, considering it has numBits bits) in previousBB
	previousBB.lastOccupiedBit += nBitsToBeInsertedInBB;  // update lastOccupiedBit correctly

	BufferedByte nextBB;

	if (previousBB.lastOccupiedBit == BUFFEREDBYTE_MAX_INDEX)  // if buffered byte is complete (8 bits)
	{
		fwrite(&previousBB.buffer, sizeof(unsigned char), 1, file);  // write it
		nextBB = BufferedByte();  // a new BufferedByte has to be instantiated to accommodate more bits from this number or another number
	}
	else
		nextBB = previousBB;  // this BufferedByte can still take more bits

	numBits -= nBitsToBeInsertedInBB;  // update number of bits left

	// there is no need to "remove" (set to 0) the bits of number that were stored in the buffer (due to bit hacks while inserting bits in buffer):
	// number &= (int) pow(2.0, (double) numBits) - 1;
	// OR
	// int shift = sizeof(unsigned int)*BYTE_SIZE - numBits;
	// number = ((unsigned int) (number << shift)) >> shift;

	return writeNumber(file, nextBB, number, numBits);  // number may not be completely written to file
}

BufferedByte BufferedByte::readNumber(FILE * file, BufferedByte previousBB, unsigned int & number, unsigned int numBits, unsigned long long & bytes_read)
{
	number = 0;
	return readNumber_aux(file, previousBB, number, numBits, bytes_read);
}