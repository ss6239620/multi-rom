#pragma once
#include <string>
#include <iomanip> // for std::setw
#include <ctime>
#include <map>
#include "serializer/jsonparser.h"

std::string getCurrentDateTime();

void printRows(const std::vector<std::map<std::string, std::string>> &rows);

void printRow(const std::map<std::string, std::string> &row);

JSON serializationTOJSONNode(std::vector<std::map<std::string, std::string>> &rows);