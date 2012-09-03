#include "Display.h"

#define MAX_COLUMN 250

void writeErrorMessageOnTopOfProgressBar(string errmsg)
{
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO  csbi;
	GetConsoleScreenBufferInfo(hCon, &csbi);
	clrln(csbi.dwCursorPosition.Y);
	gotoxy(0, csbi.dwCursorPosition.Y);
	cout << errmsg;
}

unsigned int updateProgressBar(unsigned long long bytes_read, unsigned long long file_size, unsigned int last_percentage)
{
	unsigned int new_percentage = (unsigned int) ((bytes_read*100)/file_size);
	if (new_percentage > last_percentage)
		writePercentageAndHyphens(new_percentage, new_percentage-last_percentage);
	return new_percentage;
}

void writePercentageAndHyphens(unsigned int percentage, unsigned int numHyphens)
{
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO  csbi;
	GetConsoleScreenBufferInfo(hCon, &csbi);
	SHORT old_x = csbi.dwCursorPosition.X;
	gotoxy(101, csbi.dwCursorPosition.Y);
	printf("| %d%%", percentage);
	gotoxy(old_x, csbi.dwCursorPosition.Y);

	for (unsigned int i = 0; i < numHyphens; i++)
		cout << "-";
}

void gotoxy(int x, int y)
{
	COORD coord;
	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void clrln(int y)
{
	for (int x = 0; x < MAX_COLUMN; x++)
	{ gotoxy(x,y); cout << ' '; }
}

void clearScreen(void)
{
	COORD upperLeftCorner = {0,0};
	DWORD charsWritten;
	DWORD conSize;
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO  csbi;

	GetConsoleScreenBufferInfo(hCon, &csbi);
	conSize = csbi.dwSize.X * csbi.dwSize.Y;

	// fill with spaces
	FillConsoleOutputCharacter(hCon, TEXT(' '), conSize, upperLeftCorner, &charsWritten);
	GetConsoleScreenBufferInfo(hCon, &csbi);
	FillConsoleOutputAttribute(hCon, csbi.wAttributes, conSize, upperLeftCorner, &charsWritten);

	// cursor to upper left corner
	SetConsoleCursorPosition(hCon, upperLeftCorner);
}

void setcolor(unsigned int color)
{
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hCon,color);
}

void enlargeScreenBufferAndConsole()
{
	HANDLE hOut;
	COORD NewSize;
	SMALL_RECT DisplayArea = {0, 0, 0, 0};

	hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	// Buffer
	NewSize.X = 127;
	NewSize.Y = 200;
	SetConsoleScreenBufferSize(hOut, NewSize);

	// Console
	DisplayArea.Right = 126;
	DisplayArea.Bottom = 49;

	SetConsoleWindowInfo(hOut, TRUE, &DisplayArea);
	ShowWindow(GetConsoleWindow(), SW_NORMAL);
}

void maximizeScreenBufferAndConsole()
{
	HANDLE hOut;
	COORD NewSize;
	SMALL_RECT DisplayArea = {0, 0, 0, 0};

	hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	// Buffer
	NewSize.X = MAX_COLUMN;
	NewSize.Y = 400;
	SetConsoleScreenBufferSize(hOut, NewSize);

	// Console
	NewSize = GetLargestConsoleWindowSize(hOut);
	DisplayArea.Right = NewSize.X - 1;
	DisplayArea.Bottom = NewSize.Y - 1;

	SetConsoleWindowInfo(hOut, TRUE, &DisplayArea);
	ShowWindow(GetConsoleWindow(), SW_MAXIMIZE);
}

void setConsoleTitle(char title[])
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTitleA(title);
}

void switchCursorVisibility(bool visible)
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	_CONSOLE_CURSOR_INFO cursorInfo;
	cursorInfo.bVisible = visible;
	cursorInfo.dwSize = 1;
	PCONSOLE_CURSOR_INFO pCursorInfo = &cursorInfo;
	SetConsoleCursorInfo(hOut, pCursorInfo);
}
