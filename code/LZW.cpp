#include "LZW.h"
#include "BufferedByte.h"
#include "Display.h"
#include <sys/stat.h>  // fstat

using namespace std;

const string LZW::algorithm_name = "LEMPEL-ZIV-WELCH";
const string LZW::extension = ".lzw";

LZW_compressor::LZW_compressor()
{
	it = dictionary.begin();
	for (unsigned int i = 0; i < NUM_VALUES_8BYTES; i++)
		it = dictionary.insert(it, pair<string,unsigned int>(string(1, i), i));
	lastCodeUsed = MAX_VALUE_UNSIGNED_8BYTES;
}

bool LZW_compressor::apply(FILE * ifile, FILE * ofile)
{
	char c;
	string i;
	string i_;
	unsigned int numBits = 8;  // last entry of initial dictionary (255) needs 8 bits to be represented
	BufferedByte byte;

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

		i_ = i+c;
		if (dictionary.find(i_) != dictionary.end())  // if i+c exists in the dictionary
			i = i_;
		else  // if i+c does not exist in the dictionary
		{
			byte = BufferedByte::writeNumber(ofile, byte, dictionary.find(i)->second, numBits);  // write code of i in ofile (i exists in the dictionary)
			it = dictionary.insert(it, pair<string,unsigned int>(i_, ++lastCodeUsed));  // insert i+c in the dictionary with the next available code
			if (BufferedByte::mininumBitsNeeded(lastCodeUsed) > numBits)  // if the new code needs one more bit to be represented when compared to the previous ones
				numBits++;
			i = c;  // next i will be the character read
		}
	}

	if (!i.empty())  // if i isn't empty, it's because the file ended and i was not written to file
		byte = BufferedByte::writeNumber(ofile, byte, dictionary.find(i)->second, numBits);

	if (!byte.isEmpty())  // if byte is not empty, it's because the last call to writeNumber() could not fill an entire byte to write to file
	{
		char lastIncompleteByte = byte.getBuffer();
		fwrite(&lastIncompleteByte, sizeof(char), 1, ofile);
	}

	cout << endl;
	fflush(ofile);
	switchCursorVisibility(true);
	return true;
}

void LZW_compressor::reset()
{
	dictionary.clear();
	it = dictionary.begin();
	for (unsigned int i = 0; i < NUM_VALUES_8BYTES; i++)
		it = dictionary.insert(it, pair<string,unsigned int>(string(1, i), i));
	lastCodeUsed = MAX_VALUE_UNSIGNED_8BYTES;
}

LZW_decompressor::LZW_decompressor()
{
	it = dictionary.begin();
	for (unsigned int i = 0; i < NUM_VALUES_8BYTES; i++)
		it = dictionary.insert(it, pair<unsigned int,string>(i, string(1, i)));
	lastCodeUsed = MAX_VALUE_UNSIGNED_8BYTES;
}

bool LZW_decompressor::apply(FILE * ifile, FILE * ofile)
{
	unsigned int code;
	string dict_entry_prev_code;  // X
	string dict_entry_curr_code;  // Z
	unsigned int numBits = 8;  // last entry of initial dictionary (255) needs 8 bits to be represented and the first data to be read from the compressed file is always in the initial dictionary
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
	
	try { byte = BufferedByte::readNumber(ifile, byte, code, numBits, bytes_read); }  // read first compressed character
	catch (FailedToRead &) { writeErrorMessageOnTopOfProgressBar("Erro ao ler o ficheiro comprimido.\n"); switchCursorVisibility(true); return false; }

	map<unsigned int,string>::iterator it = dictionary.find(code);
	if (it == dictionary.end()) { writeErrorMessageOnTopOfProgressBar("Erro ao ler o ficheiro comprimido.\n"); switchCursorVisibility(true); return false; }  // if code of character wasn't found

	dict_entry_prev_code = it->second;
	fwrite(dict_entry_prev_code.c_str(), sizeof(char), 1, ofile);  // write the first decompressed character
	last_percentage = updateProgressBar(bytes_read, ifile_size, last_percentage);

	while(!feof(ifile))
	{
		if (BufferedByte::mininumBitsNeeded(lastCodeUsed+1) > numBits)  // if the next code needs one more bit to be represented when compared to the previous ones
			numBits++;

		try { byte = BufferedByte::readNumber(ifile, byte, code, numBits, bytes_read); }  // read next code with numBits bits
		catch (FailedToRead &) { writeErrorMessageOnTopOfProgressBar("Erro ao ler o ficheiro comprimido.\n"); fflush(ofile); switchCursorVisibility(true); return false; }
		catch (FinishedReading &)
		{
			last_percentage = updateProgressBar(bytes_read, ifile_size, last_percentage);
			break;
		}

		last_percentage = updateProgressBar(bytes_read, ifile_size, last_percentage);

		it = dictionary.find(code);  // search code in dictionary (codes some string Z)
		if (it != dictionary.end())  // if code exists in the dictionary, the dictionary entry added by the compressor (W) when writing dict_entry_prev_code (X) to file is X + first character of the string (Z=dict_entry_curr_code) this code codes, which means W = X+Z[0].
		{
			dict_entry_curr_code = it->second;  // Z
			fwrite(dict_entry_curr_code.c_str(), sizeof(char), dict_entry_curr_code.length(), ofile);
			it = dictionary.insert(it, pair<unsigned int, string>(++lastCodeUsed, dict_entry_prev_code+dict_entry_curr_code.at(0)));  // W = X+? (conjecture) = X+Z[0]. Add it to the decoder's dictionary (just one step behind the encoder).
		}
		else  // if code (codes string Z) doesn't exist in the dictionary and knowing the decoder is only one step behind the encoder, then it means Z was added to the encoder's dictionary just before it wrote Z to file, so the next entry of the decoder's dictionary (which has to be the same than the encoder's last entry) is also Z.
		{  // since Z was added to the encoder's dictionary right before this point (right before it wrote Z to file), then it means the conjecture W = X+? has to be Z: Z=W=X+?. Because ? is always the first letter of the string coded by the code that follows the code of X, ? must be Z[0], which means ? = (X+?)[0] = X[0] = dict_entry_prev_code[0]. So Z = dict_entry_prev_code + dict_entry_prev_code[0].
			dict_entry_curr_code = dict_entry_prev_code + dict_entry_prev_code.at(0);  // Z
			fwrite(dict_entry_curr_code.c_str(), sizeof(char), dict_entry_curr_code.length(), ofile);
			it = dictionary.insert(it, pair<unsigned int, string>(++lastCodeUsed, dict_entry_curr_code));  // add W=Z to decoder's dictionary.
		}

		dict_entry_prev_code = dict_entry_curr_code;  // X <-- Z
	}

	cout << endl;
	fflush(ofile);
	switchCursorVisibility(true);
	return true;
}

void LZW_decompressor::reset()
{
	dictionary.clear();
	it = dictionary.begin();
	for (unsigned int i = 0; i < NUM_VALUES_8BYTES; i++)
		it = dictionary.insert(it, pair<unsigned int,string>(i, string(1, i)));
	lastCodeUsed = NUM_VALUES_8BYTES-1;
}
