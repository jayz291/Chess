#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#ifndef UCI_H
#define UCI_H

/**
 * @brief runs the chess engine so that it communicates using UCI (Universal Chess Interface)
 */
void run_uci_loop(); 

/**
 * @brief formats the evaluation so that it is displayed in centipawns, or mate in [int]
 * @param score the evaluation
 * @return a string representing the formatted evaluation to be printed out
 */
std::string format_score(int score);
#endif