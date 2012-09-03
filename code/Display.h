#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <Windows.h>
#include <iostream>
#include <string>

using namespace std;

#define WHITE 0x07
#define GREEN FOREGROUND_GREEN

void writeErrorMessageOnTopOfProgressBar(string errmsg);

unsigned int updateProgressBar(unsigned long long bytes_read, unsigned long long file_size, unsigned int last_percentage);

void writePercentageAndHyphens(unsigned int percentage, unsigned int numHyphens);

/**
 * @brief Desloca cursor da consola
 * Recebe 2 inteiros, que servem de coordenadas para
 * a movimentação do cursor da janela de comando.
 * @param x Coordenada onde ficará no eixo dos xx
 * @param y Coordenada onde ficará no eixo dos yy
 */
void gotoxy(int x, int y);

/**
 * @brief Limpa uma determinada linha do ecrã
 * Recebe uma coordenada y e substitui todos os 
 * caracteres dessa linha por caracteres vazios
 * @param y Coordenada da linha a ser limpa
 */
void clrln(int y);

/**
 * @brief Limpa uma área da consola
 * Insere umas certas linhas de caracteres whitespace na 
 * consola para dar o efeito que "limpou" o ecrã.
 */
void clearScreen(void);

/**
 * @brief Altera a cor dos caracteres imprimidos no ecrã
 * @param color Valor da cor a utilizar
 */
void setcolor(unsigned int color);

/**
 * @brief Aumenta o tamanho do buffer do ecrã
 */
void enlargeScreenBufferAndConsole();

/**
 * @brief Aumenta o tamanho do buffer e da consola
 */
void maximizeScreenBufferAndConsole();

/**
 * @brief Define um título na janela da consola
 * @param title Array contendo os caracteres a serem apresentados na barra
 */
void setConsoleTitle(char title[]);

/**
 * @brief Coloca ou não visível o cursor na consola
 * @param visible Boleano que define se o cursor deve ou não ser visível
 */
void switchCursorVisibility(bool visible);

#endif /* DISPLAY_H_ */
