#include "BufferedByte.h"
#include "LZW.h"
#include "Huffman.h"
#include "Display.h"
#include <sys/stat.h>  // fstat
#include <ctime>
#include <conio.h>
#include <sstream>
#include <vector>

//! @brief Código ASCII da tecla Enter.
#define ENTER 13
#define NUM_TEST_FILES 9
#define COMPRESS_OPT "-comprimir"
#define DECOMPRESS_OPT "-descomprimir"
#define LZW_OPT "-lzw"
#define HUFFMAN_OPT "-huffman"

struct TestFile
{
	string nameOriginal, nameCompressed, nameDecompressed;
	unsigned long time_comp_algorithm1, time_decomp_algorithm1, time_comp_algorithm2, time_decomp_algorithm2;
	float compressRatio_algorithm1, compressRatio_algorithm2;
	unsigned long sizeOriginal;
	FILE * original, * compressed, * decompressed;
};

vector<TestFile> testFiles(NUM_TEST_FILES);  // files used to do a comparative analysis between two algorithms
CompressionAlgorithm * algorithm1_comp, * algorithm1_decomp, * algorithm2_comp, * algorithm2_decomp;  // for comparative analysis

/*!
@brief Converte string para objecto de outra classe ou tipo (números (se inteiro, trunca), etc.), desde que exista overload do operador >> nessa classe.
@param str String a converter.
@param retorno Variável onde ficará o resultado da conversão.
@return Retorna true se atribuiu valor a retorno, false se não.
*/
template<class T>
bool str2temp(const string & str, T & retorno)
{
	istringstream tmp(str);

	tmp >> retorno;

	if (tmp.fail())  // se não conseguiu atribuir valor a retorno, retorna false.
		return false;
	
	return true;
}

unsigned int getOption(unsigned int maxOption)
{
	string input;
	unsigned int option;
	bool valid = false;

	do
	{
		cout << "\nOpcao: ";
		getline(cin, input); cin.clear(); if (cin.fail()) cin.ignore(1000, '\n');
		if (!str2temp(input, option) || option > maxOption)
			cout << "Opcao invalida.";
		else
			valid = true;
	} while (!valid);

	return option;
}

/*!
@brief Espera que o utilizador carregue em Enter, imprimindo, previamente, a string passada como argumento.
@param str String a apresentar antes de esperar por Enter.
*/
void pressEnter(string str)
{
	char ch;
	cout << str;

	do
	{
		ch = _getch();
	} while (ch != ENTER);

	cout << endl;
}

void compress(CompressionAlgorithm & algorithm, bool askFileNames, string nameOriginal, string nameCompressed)  // if askFileNames == true, then the program is using menu interface
{
	string pressEnter_str = askFileNames ? "Prima ENTER para voltar ao Menu Inicial." : "Prima ENTER para sair.";

	if (askFileNames)
		clearScreen();
	setcolor(GREEN);
	cout << "COMPRIMIR FICHEIRO (ALGORITMO " << algorithm.getAlgorithmName() << "):\n\n";
	setcolor(WHITE);

	if (askFileNames)
	{
		cout << "Nome do ficheiro a comprimir: ";
		getline(cin, nameOriginal); cin.clear(); if (cin.fail()) cin.ignore(1000, '\n');
	}

	FILE* ifile;
	if (fopen_s(&ifile, nameOriginal.c_str(), "rb") != 0)
	{
		cout << "Erro ao abrir o ficheiro a comprimir.\n\n";
		pressEnter(pressEnter_str);
		return;
	}

	if (askFileNames)
	{
		cout << "Nome do ficheiro comprimido a criar (sem extensao): ";
		getline(cin, nameCompressed); cin.clear(); if (cin.fail()) cin.ignore(1000, '\n');
	}

	nameCompressed += algorithm.getCompressedFilesExtension();
	FILE* ofile;
	if (fopen_s(&ofile, nameCompressed.c_str(), "wb") != 0)
	{
		cout << "Erro ao criar o ficheiro comprimido.\n\n";
		pressEnter(pressEnter_str);
		fclose(ifile);
		return;
	}

	if (askFileNames)
		cout << endl;

	// Get original file size
	struct stat file_stat;
    fstat(_fileno(ifile), &file_stat);
    unsigned long long ifile_size = file_stat.st_size;

	// Apply compression and measure time spent doing it
	time_t t1_compress = time(NULL);
	algorithm.apply(ifile, ofile);
	time_t t2_compress = time(NULL);

	// Get compressed file size
    fstat(_fileno(ofile), &file_stat);
    unsigned long long ofile_size = file_stat.st_size;

	fclose(ifile);
	fclose(ofile);

	cout << "\nTamanho original: " << ifile_size << " byte(s). Tamanho comprimido: " << ofile_size << " byte(s).\n";
	cout << "Percentagem de compressao (% de tamanho poupado): " << (ifile_size != 0 ? 100 - ((double) ofile_size*100)/ifile_size : 0) << "%\n";
	cout << "Tempo decorrido: " << t2_compress-t1_compress << " segundo(s).\n\n";

	pressEnter(pressEnter_str);
}

void menuCompression()
{
	const unsigned int maxOption = 2;

	clearScreen();
	cout << "+---------------------------+" << endl;
	cout << "|        ";
	setcolor(GREEN);
	cout << "COMPRIMIR:";
	setcolor(WHITE);
	cout << "         |" << endl;
	cout << "| 1 - Lempel-Ziv-Welch      |" << endl;
	cout << "| 2 - Huffman               |" << endl;
	cout << "|                           |" << endl;
	cout << "| 0 - Menu Inicial          |" << endl;
	cout << "+---------------------------+" << endl;

	unsigned int option = getOption(maxOption);

	switch(option)
	{
	case 0:
		return;
	case 1:
		compress(LZW_compressor(), true, "", "");
		return;
	case 2:
		compress(Huffman_compressor(), true, "", "");
		return;
	}
}

void menuCompareFiles(bool ask2Files, string name1, bool withMenuInterface)
{
	string pressEnter_str = withMenuInterface ? "Prima ENTER para voltar ao Menu Inicial." : "Prima ENTER para sair.";

	char c1, c2;
	int read1, read2;
	string name2;

	if (withMenuInterface)
		clearScreen();
	setcolor(GREEN);
	cout << "COMPARAR FICHEIROS:\n\n";
	setcolor(WHITE);

	if (ask2Files)
	{
		cout << "Nome do ficheiro 1: ";
		getline(cin, name1); cin.clear(); if (cin.fail()) cin.ignore(1000, '\n');
	}

	FILE* file1;
	if (fopen_s(&file1, name1.c_str(), "rb") != 0)
	{
		if (ask2Files)
			cout << "Erro ao abrir o ficheiro 1.\n\n";
		else
			cout << "Erro ao abrir o ficheiro descomprimido.\n\n";
		pressEnter(pressEnter_str);
		return;
	}

	if (ask2Files)
		cout << "Nome do ficheiro 2: ";
	else
		cout << "Nome do ficheiro original: ";
	getline(cin, name2); cin.clear(); if (cin.fail()) cin.ignore(1000, '\n');
	FILE* file2;
	if (fopen_s(&file2, name2.c_str(), "rb") != 0)
	{
		if (ask2Files)
			cout << "Erro ao abrir o ficheiro 2.\n\n";
		else
			cout << "Erro ao abrir o ficheiro original.\n\n";
		pressEnter(pressEnter_str);
		fclose(file1);
		return;
	}

	cout << "\nA comparar ficheiros...\n";

	// Check file sizes
	struct stat file_stat;
    fstat(_fileno(file1), &file_stat);
    unsigned long long file1_size = file_stat.st_size;
    fstat(_fileno(file2), &file_stat);
    unsigned long long file2_size = file_stat.st_size;
	if (file1_size != file2_size)
	{
		cout << "Resultado: DIFERENTES.\n\n";
		pressEnter(pressEnter_str);
		fclose(file1);
		fclose(file2);
		return;
	}

	// Check character by character
	while (!feof(file1))
	{
		read1 = fread(&c1, sizeof(char), 1, file1);
		read2 = fread(&c2, sizeof(char), 1, file2);
		if (read1 != read2)
		{
			cout << "Resultado: DIFERENTES.\n\n";
			pressEnter(pressEnter_str);
			fclose(file1);
			fclose(file2);
			return;
		}

		if (c1 != c2)
		{
			cout << "Resultado: DIFERENTES.\n\n";
			pressEnter(pressEnter_str);
			fclose(file1);
			fclose(file2);
			return;
		}
	}

	cout << "Resultado: IGUAIS.\n\n";
	pressEnter(pressEnter_str);
	fclose(file1);
	fclose(file2);
}

void decompress(CompressionAlgorithm & algorithm, bool askFileNames, string nameCompressed, string nameDecompressed)  // if askFileNames == true, then the program is using menu interface
{
	string pressEnter_str = askFileNames ? "Prima ENTER para voltar ao Menu Inicial." : "Prima ENTER para sair.";

	if (askFileNames)
		clearScreen();
	setcolor(GREEN);
	cout << "DESCOMPRIMIR FICHEIRO (ALGORITMO " << algorithm.getAlgorithmName() << "):\n\n";
	setcolor(WHITE);

	if (askFileNames)
	{
		cout << "Nome do ficheiro comprimido (sem extensao): ";
		getline(cin, nameCompressed); cin.clear(); if (cin.fail()) cin.ignore(1000, '\n');
	}

	nameCompressed += algorithm.getCompressedFilesExtension();
	FILE* ifile;
	if (fopen_s(&ifile, nameCompressed.c_str(), "rb") != 0)
	{
		cout << "Erro ao abrir o ficheiro comprimido.\n\n";
		pressEnter(pressEnter_str);
		return;
	}

	if (askFileNames)
	{
		cout << "Nome do ficheiro descomprimido a criar: ";
		getline(cin, nameDecompressed); cin.clear(); if (cin.fail()) cin.ignore(1000, '\n');
	}

	FILE* ofile;
	if (fopen_s(&ofile, nameDecompressed.c_str(), "wb") != 0)
	{
		cout << "Erro ao criar o ficheiro descomprimido.\n\n";
		pressEnter(pressEnter_str);
		fclose(ifile);
		return;
	}

	if (askFileNames)
		cout << endl;

	// Apply compression and measure time spent doing it
	time_t t1_compress = time(NULL);
	algorithm.apply(ifile, ofile);
	time_t t2_compress = time(NULL);

	fclose(ifile);
	fclose(ofile);

	cout << "\nTempo decorrido: " << t2_compress-t1_compress << " segundo(s).\n";
	
	// Ask user if he/she wants to check for equality between the original and the decompressed file
	string input;
	bool valid = false;
	do
	{
		cout << "\nVerificar igualdade do ficheiro descomprimido com o ficheiro original (S/N)? ";
		getline(cin, input); cin.clear(); if (cin.fail()) cin.ignore(1000, '\n');
		if (input == "N" || input == "n")
			valid = true;
		else
			if (input == "S" || input == "s")
			{
				if (!askFileNames)
					cout << endl << endl;
				menuCompareFiles(false, nameDecompressed, askFileNames);  // send user to compare files menu
				return;
			}
			else
				cout << "Opcao invalida.";
	} while (!valid);

	cout << endl;
	pressEnter(pressEnter_str);
}

void menuDecompression()
{
	const unsigned int maxOption = 2;

	clearScreen();
	cout << "+---------------------------+" << endl;
	cout << "|       ";
	setcolor(GREEN);
	cout << "DESCOMPRIMIR:";
	setcolor(WHITE);
	cout << "       |" << endl;
	cout << "| 1 - Lempel-Ziv-Welch      |" << endl;
	cout << "| 2 - Huffman               |" << endl;
	cout << "|                           |" << endl;
	cout << "| 0 - Menu Inicial          |" << endl;
	cout << "+---------------------------+" << endl;

	unsigned int option = getOption(maxOption);

	switch(option)
	{
	case 0:
		return;
	case 1:
		decompress(LZW_decompressor(), true, "", "");
		return;
	case 2:
		decompress(Huffman_decompressor(), true, "", "");
		return;
	}
}

bool menuCompareAlgorithms()  // returns false if the caller needs to call delete for each of the 4 CompressionAlgorithm pointers used in this comparative analysis
{
	//const unsigned int maxOption = 2;

	//clearScreen();
	//cout << "+---------------------------+" << endl;
	//cout << "|    ";
	//setcolor(GREEN);
	//cout << "PRIMEIRO ALGORITMO:";
	//setcolor(WHITE);
	//cout << "    |" << endl;
	//cout << "| 1 - Lempel-Ziv-Welch      |" << endl;
	//cout << "| 2 - Huffman               |" << endl;
	//cout << "|                           |" << endl;
	//cout << "| 0 - Menu Inicial          |" << endl;
	//cout << "+---------------------------+" << endl;

	//unsigned int option1 = getOption(maxOption);
	//if (option1 == 0)
	//{
	//	cout << endl;
	//	return true;
	//}

	//clearScreen();
	//cout << "+---------------------------+" << endl;
	//cout << "|    ";
	//setcolor(GREEN);
	//cout << "SEGUNDO ALGORITMO:";
	//setcolor(WHITE);
	//cout << "     |" << endl;
	//cout << "| 1 - Lempel-Ziv-Welch      |" << endl;
	//cout << "| 2 - Huffman               |" << endl;
	//cout << "|                           |" << endl;
	//cout << "| 0 - Menu Inicial          |" << endl;
	//cout << "+---------------------------+" << endl;

	//unsigned int option2 = getOption(maxOption);
	//if (option2 == 0)
	//{
	//	cout << endl;
	//	return true;
	//}
	//if (option1 == option2)
	//{
	//	cout << "\nTem de escolher algoritmos diferentes.\n\n";
	//	return true;
	//}

	//switch(option1)
	//{
	//case 1:
	//	algorithm1_comp = new LZW_compressor();
	//	algorithm1_decomp = new LZW_decompressor();
	//	break;
	//case 2:
	//	algorithm1_comp = new Huffman_compressor();
	//	algorithm1_decomp = new Huffman_decompressor();
	//	break;
	//}
	//switch(option2)
	//{
	//case 1:
	//	algorithm2_comp = new LZW_compressor();
	//	algorithm2_decomp = new LZW_decompressor();
	//	break;
	//case 2:
	//	algorithm2_comp = new Huffman_compressor();
	//	algorithm2_decomp = new Huffman_decompressor();
	//	break;
	//}

	clearScreen();

	algorithm1_comp = new LZW_compressor();
	algorithm1_decomp = new LZW_decompressor();
	algorithm2_comp = new Huffman_compressor();
	algorithm2_decomp = new Huffman_decompressor();

	setcolor(GREEN);
	cout << "ANALISE COMPARATIVA DOS ALGORITMOS " << algorithm1_comp->getAlgorithmName() << " E " << algorithm2_comp->getAlgorithmName() << ":\n\n";
	setcolor(WHITE);
	struct stat file_stat;

	// Apply compressions and decompressions, measure compression rates and measure time spent doing each compression/decompression
	for (int i = 0; i < NUM_TEST_FILES; i++)
	{
		// Open original (rb), compressed (wb) and decompressed files (wb)
		if (fopen_s(&testFiles.at(i).original, testFiles.at(i).nameOriginal.c_str(), "rb") != 0) { cout << "Erro ao abrir " << testFiles.at(i).nameOriginal << " para leitura.\n\n"; return false; }
		if (fopen_s(&testFiles.at(i).compressed, testFiles.at(i).nameCompressed.c_str(), "wb") != 0) { cout << "Erro ao abrir " << testFiles.at(i).nameCompressed << " para escrita.\n\n"; fclose(testFiles.at(i).original); return false; }
		if (fopen_s(&testFiles.at(i).decompressed, testFiles.at(i).nameDecompressed.c_str(), "wb") != 0) { cout << "Erro ao abrir " << testFiles.at(i).nameDecompressed << " para escrita.\n\n"; fclose(testFiles.at(i).original); fclose(testFiles.at(i).compressed); return false; }

		// Apply compression of algorithm 1 and measure time
		cout << algorithm1_comp->getAlgorithmName() << " - Compressao do ficheiro " << testFiles.at(i).nameOriginal << ":\n";
		time_t t1 = time(NULL);
		algorithm1_comp->apply(testFiles.at(i).original, testFiles.at(i).compressed);
		time_t t2 = time(NULL);
		cout << endl;
		testFiles.at(i).time_comp_algorithm1 = (unsigned long) (t2-t1);
		if (i < (NUM_TEST_FILES-1)) algorithm1_comp->reset();

		// Close compressed file, reopen it for reading and calculate compress ratio of algorithm 1
		fclose(testFiles.at(i).compressed);
		if (fopen_s(&testFiles.at(i).compressed, testFiles.at(i).nameCompressed.c_str(), "rb") != 0) { cout << "Erro ao abrir " << testFiles.at(i).nameCompressed << " para leitura.\n\n"; fclose(testFiles.at(i).original); fclose(testFiles.at(i).decompressed); return false; }
		fstat(_fileno(testFiles.at(i).original), &file_stat);
		testFiles.at(i).sizeOriginal = file_stat.st_size;
		fstat(_fileno(testFiles.at(i).compressed), &file_stat);
		testFiles.at(i).compressRatio_algorithm1 = testFiles.at(i).sizeOriginal != 0 ? 100 - ((float) file_stat.st_size*100)/testFiles.at(i).sizeOriginal : 0;

		// Apply decompression of algorithm 1 and measure time
		cout << algorithm1_decomp->getAlgorithmName() << " - Descompressao do ficheiro " << testFiles.at(i).nameCompressed << ":\n";
		t1 = time(NULL);
		algorithm1_decomp->apply(testFiles.at(i).compressed, testFiles.at(i).decompressed);
		t2 = time(NULL);
		cout << endl;
		testFiles.at(i).time_decomp_algorithm1 = (unsigned long) (t2-t1);
		if (i < (NUM_TEST_FILES-1)) algorithm1_decomp->reset();

		// Rewind original file, close and reopen compressed and decompressed files (both for writing)
		rewind(testFiles.at(i).original);
		fclose(testFiles.at(i).compressed);
		fclose(testFiles.at(i).decompressed);
		if (fopen_s(&testFiles.at(i).compressed, testFiles.at(i).nameCompressed.c_str(), "wb") != 0) { cout << "Erro ao abrir " << testFiles.at(i).nameCompressed << " para escrita.\n\n"; fclose(testFiles.at(i).original); return false; }
		if (fopen_s(&testFiles.at(i).decompressed, testFiles.at(i).nameDecompressed.c_str(), "wb") != 0) { cout << "Erro ao abrir " << testFiles.at(i).nameDecompressed << " para escrita.\n\n"; fclose(testFiles.at(i).original); fclose(testFiles.at(i).compressed); return false; }

		// Apply compression of algorithm 2 and measure time
		cout << algorithm2_comp->getAlgorithmName() << " - Compressao do ficheiro " << testFiles.at(i).nameOriginal << ":\n";
		t1 = time(NULL);
		algorithm2_comp->apply(testFiles.at(i).original, testFiles.at(i).compressed);
		t2 = time(NULL);
		cout << endl;
		testFiles.at(i).time_comp_algorithm2 = (unsigned long) (t2-t1);
		if (i < (NUM_TEST_FILES-1)) algorithm2_comp->reset();

		// Close compressed file, reopen it for reading and calculate compress ratio of algorithm 2
		fclose(testFiles.at(i).compressed);
		if (fopen_s(&testFiles.at(i).compressed, testFiles.at(i).nameCompressed.c_str(), "rb") != 0) { cout << "Erro ao abrir " << testFiles.at(i).nameCompressed << " para leitura.\n\n"; fclose(testFiles.at(i).original); fclose(testFiles.at(i).decompressed); return false; }
		fstat(_fileno(testFiles.at(i).compressed), &file_stat);
		testFiles.at(i).compressRatio_algorithm2 = testFiles.at(i).sizeOriginal != 0 ? 100 - ((float) file_stat.st_size*100)/testFiles.at(i).sizeOriginal : 0;

		// Apply decompression of algorithm 2 and measure time
		cout << algorithm2_decomp->getAlgorithmName() << " - Descompressao do ficheiro " << testFiles.at(i).nameCompressed << ":\n";
		t1 = time(NULL);
		algorithm2_decomp->apply(testFiles.at(i).compressed, testFiles.at(i).decompressed);
		t2 = time(NULL);
		cout << endl;
		testFiles.at(i).time_decomp_algorithm2 = (unsigned long) (t2-t1);
		if (i < (NUM_TEST_FILES-1)) algorithm2_decomp->reset();

		// Close original, compressed and decompressed files
		fclose(testFiles.at(i).original);
		fclose(testFiles.at(i).compressed);
		fclose(testFiles.at(i).decompressed);
	}

	delete algorithm1_decomp;
	delete algorithm2_decomp;

	// Show results in two tables (one for compression, the other for decompression)
	cout << endl << endl;
	cout << "+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+\n";
	cout << "|                       |     Ficheiro 100KB    |     Ficheiro 250KB    |     Ficheiro 400KB    |     Ficheiro 100KB    |     Ficheiro 250KB    |     Ficheiro 400KB    |     Ficheiro 100KB    |     Ficheiro 250KB    |     Ficheiro 400KB    |\n";
	cout << "|      COMPRESSAO       |          TXT          |          TXT          |          TXT          |          BMP          |          BMP          |          BMP          |          WAV          |          WAV          |          WAV          |\n";
	cout << "|                       | Racio (%) | Tempo (s) | Racio (%) | Tempo (s) | Racio (%) | Tempo (s) | Racio (%) | Tempo (s) | Racio (%) | Tempo (s) | Racio (%) | Tempo (s) | Racio (%) | Tempo (s) | Racio (%) | Tempo (s) | Racio (%) | Tempo (s) |\n";
	cout << "+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+\n";
	printf("| %-22.22s|%-11.2f|%-11lu|%-11.2f|%-11lu|%-11.2f|%-11lu|%-11.2f|%-11lu|%-11.2f|%-11lu|%-11.2f|%-11lu|%-11.2f|%-11lu|%-11.2f|%-11lu|%-11.2f|%-11lu|\n", algorithm1_comp->getAlgorithmName().c_str(),
		testFiles.at(0).compressRatio_algorithm1, testFiles.at(0).time_comp_algorithm1, testFiles.at(1).compressRatio_algorithm1, testFiles.at(1).time_comp_algorithm1,
		testFiles.at(2).compressRatio_algorithm1, testFiles.at(2).time_comp_algorithm1, testFiles.at(3).compressRatio_algorithm1, testFiles.at(3).time_comp_algorithm1,
		testFiles.at(4).compressRatio_algorithm1, testFiles.at(4).time_comp_algorithm1, testFiles.at(5).compressRatio_algorithm1, testFiles.at(5).time_comp_algorithm1,
		testFiles.at(6).compressRatio_algorithm1, testFiles.at(6).time_comp_algorithm1, testFiles.at(7).compressRatio_algorithm1, testFiles.at(7).time_comp_algorithm1,
		testFiles.at(8).compressRatio_algorithm1, testFiles.at(8).time_comp_algorithm1);
	cout << "+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+\n";
	printf("| %-22.22s|%-11.2f|%-11lu|%-11.2f|%-11lu|%-11.2f|%-11lu|%-11.2f|%-11lu|%-11.2f|%-11lu|%-11.2f|%-11lu|%-11.2f|%-11lu|%-11.2f|%-11lu|%-11.2f|%-11lu|\n", algorithm2_comp->getAlgorithmName().c_str(),
		testFiles.at(0).compressRatio_algorithm2, testFiles.at(0).time_comp_algorithm2, testFiles.at(1).compressRatio_algorithm2, testFiles.at(1).time_comp_algorithm2,
		testFiles.at(2).compressRatio_algorithm2, testFiles.at(2).time_comp_algorithm2, testFiles.at(3).compressRatio_algorithm2, testFiles.at(3).time_comp_algorithm2,
		testFiles.at(4).compressRatio_algorithm2, testFiles.at(4).time_comp_algorithm2, testFiles.at(5).compressRatio_algorithm2, testFiles.at(5).time_comp_algorithm2,
		testFiles.at(6).compressRatio_algorithm2, testFiles.at(6).time_comp_algorithm2, testFiles.at(7).compressRatio_algorithm2, testFiles.at(7).time_comp_algorithm2,
		testFiles.at(8).compressRatio_algorithm2, testFiles.at(8).time_comp_algorithm2);
	cout << "+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+\n";

	cout << endl << endl;
	cout << "+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+\n";
	cout << "|                       |     Ficheiro 100KB    |     Ficheiro 250KB    |     Ficheiro 400KB    |     Ficheiro 100KB    |     Ficheiro 250KB    |     Ficheiro 400KB    |     Ficheiro 100KB    |     Ficheiro 250KB    |     Ficheiro 400KB    |\n";
	cout << "|     DESCOMPRESSAO     |          TXT          |          TXT          |          TXT          |          BMP          |          BMP          |          BMP          |          WAV          |          WAV          |          WAV          |\n";
	cout << "|                       |       Tempo (s)       |       Tempo (s)       |       Tempo (s)       |       Tempo (s)       |       Tempo (s)       |       Tempo (s)       |       Tempo (s)       |       Tempo (s)       |       Tempo (s)       |\n";
	cout << "+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+\n";
	printf("| %-22.22s|%-23lu|%-23lu|%-23lu|%-23lu|%-23lu|%-23lu|%-23lu|%-23lu|%-23lu|\n", algorithm1_comp->getAlgorithmName().c_str(),
		testFiles.at(0).time_decomp_algorithm1, testFiles.at(1).time_decomp_algorithm1,
		testFiles.at(2).time_decomp_algorithm1, testFiles.at(3).time_decomp_algorithm1,
		testFiles.at(4).time_decomp_algorithm1, testFiles.at(5).time_decomp_algorithm1,
		testFiles.at(6).time_decomp_algorithm1, testFiles.at(7).time_decomp_algorithm1,
		testFiles.at(8).time_decomp_algorithm1);
	delete algorithm1_comp;
	cout << "+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+\n";
	printf("| %-22.22s|%-23lu|%-23lu|%-23lu|%-23lu|%-23lu|%-23lu|%-23lu|%-23lu|%-23lu|\n", +algorithm2_comp->getAlgorithmName().c_str(),
		testFiles.at(0).time_decomp_algorithm2, testFiles.at(1).time_decomp_algorithm2,
		testFiles.at(2).time_decomp_algorithm2, testFiles.at(3).time_decomp_algorithm2,
		testFiles.at(4).time_decomp_algorithm2, testFiles.at(5).time_decomp_algorithm2,
		testFiles.at(6).time_decomp_algorithm2, testFiles.at(7).time_decomp_algorithm2,
		testFiles.at(8).time_decomp_algorithm2);
		delete algorithm2_comp;
	cout << "+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+\n";

	cout << "\n(Racio e' a percentagem de tamanho poupado ao comprimir o ficheiro original. Se negativo, o ficheiro comprimido e' maior do que o original)" << endl << endl << endl;

	return true;
}

void mainMenu()
{
	const unsigned int maxOption = 4;

	while (true)
	{
		clearScreen();
		cout << "+---------------------------+" << endl;
		cout << "|       ";
		setcolor(GREEN);
		cout << "MENU INICIAL:";
		setcolor(WHITE);
		cout << "       |" << endl;
		cout << "| 1 - Comprimir ficheiro    |" << endl;
		cout << "| 2 - Descomprimir ficheiro |" << endl;
		cout << "| 3 - Comparar algoritmos   |" << endl;
		cout << "| 4 - Comparar ficheiros    |" << endl;
		cout << "|                           |" << endl;
		cout << "| 0 - Sair                  |" << endl;
		cout << "+---------------------------+" << endl;

		unsigned int option = getOption(maxOption);

		switch(option)
		{
		case 0:
			clearScreen();
			exit(0);
		case 1:
			menuCompression();
			break;
		case 2:
			menuDecompression();
			break;
		case 3:
			if (!menuCompareAlgorithms())
			{
				delete algorithm1_comp;
				delete algorithm2_comp;
				delete algorithm1_decomp;
				delete algorithm2_decomp;
			}

			pressEnter("Prima ENTER para voltar ao Menu Inicial.");

			for (int i = 0; i < NUM_TEST_FILES; i++)
			{
				remove(testFiles.at(i).nameCompressed.c_str());
				remove(testFiles.at(i).nameDecompressed.c_str());
			}

			break;
		case 4:
			menuCompareFiles(true, "", true);
			break;
		}
	}
}

void printUsage()
{
	cout << "Argumentos invalidos.\n";
	cout << "Uso: <Tipo de operacao> <Algoritmo> <Ficheiro de entrada> <Ficheiro de saida>\n";
	cout << "<Tipo de operacao>: " << COMPRESS_OPT << " ou " << DECOMPRESS_OPT << "\n";
	cout << "<Algoritmo>: " << LZW_OPT << " ou " << HUFFMAN_OPT << "\n";
	cout << "<Ficheiro de entrada>: ficheiro a ser comprimido (com extensao) ou descomprimido (sem extensao)\n";
	cout << "<Ficheiro de saida>: ficheiro resultante (possivelmente inexistente) comprimido (sem extensao) ou descomprimido (com extensao)\n";
	exit(-1);
}

int main(int argc, char ** argv)
{
	setcolor(WHITE);

	void (*operation)(CompressionAlgorithm &, bool, string, string);
	CompressionAlgorithm * algorithm;

	if (argc != 5 && argc != 1)
	{
		enlargeScreenBufferAndConsole();
		printUsage();
	}

	if (argc == 5)
	{
		enlargeScreenBufferAndConsole();

		operation = strcmp(argv[1], COMPRESS_OPT) == 0 ? &compress : (strcmp(argv[1], DECOMPRESS_OPT) == 0 ? &decompress : NULL);
		if (operation == NULL)
			printUsage();
		
		if (strcmp(argv[2], LZW_OPT) == 0)
			if (operation == &compress)
				algorithm = new LZW_compressor();
			else
				algorithm = new LZW_decompressor();
		else
		{
			if (strcmp(argv[2], HUFFMAN_OPT) == 0)
				if (operation == &compress)
					algorithm = new Huffman_compressor();
				else
					algorithm = new Huffman_decompressor();
			else
				printUsage();
		}

		operation(*algorithm, false, argv[3], argv[4]);
		delete algorithm;
		return 0;
	}

	maximizeScreenBufferAndConsole();
	setConsoleTitle("WinCAL");

	// Compute test files' names
	char * testFileSize = new char[4];
	string type[3] = { "txt", "bmp", "wav" };
	for (int i = 0; i < NUM_TEST_FILES; i++)
	{
		sprintf_s(testFileSize, 4, "%d", 150*(i%3)+100);
		testFiles.at(i).nameOriginal = "WinCAL_comparative_analysis_files\\test_file_" + (string) testFileSize + "KB." + type[i/3];
		testFiles.at(i).nameDecompressed = "WinCAL_comparative_analysis_files\\test_file_" + (string) testFileSize + "KB_decompressed." + type[i/3];
		testFiles.at(i).nameCompressed = "WinCAL_comparative_analysis_files\\test_file_" + (string) testFileSize + "KB_compressed_" + type[i/3];
	}

	mainMenu();
}