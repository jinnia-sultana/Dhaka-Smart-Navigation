#ifndef CSV_H
#define CSV_H

#include "mode.h"
#include <vector>

#define MAX_LINE   200000
#define MAX_TOKENS 5000

void trim_in_place(char *s);
int  split_csv(char *line, char **tokens, int maxTokens);
int  is_number_token(const char *s);

void parseRoadmapCSV(const char *filename);
void parseMetroCSV(const char *filename);
void parseBusCSV(const char *filename, Mode busMode);

void exportPathToKML(const std::vector<int>& path, const char *filename);

#endif