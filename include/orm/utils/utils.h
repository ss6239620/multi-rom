#pragma once
#include <string>
#include <iomanip> // for std::setw
#include <ctime>
#include "serializer/jsonparser.h"

std::string getCurrentDateTime()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    return buf;
}

void printRows(const std::vector<std::map<std::string, std::string>> &rows)
{
    if (rows.empty())
    {
        std::cout << "No rows found." << std::endl;
        return;
    }
    // Get headers from first row
    std::vector<std::string> headers;
    for (const auto &pair : rows[0])
    {
        headers.push_back(pair.first);
    }
    // print headers
    for (const auto &header : headers)
    {
        std::cout << std::setw(15) << std::left << header;
    }
    std::cout << "\n";

    // print seperator
    for (size_t i = 0; i < headers.size(); i++)
    {
        std::cout << std::string(15, '-');
    }
    std::cout << "\n";

    // print rows
    for (const auto &row : rows)
    {
        for (const auto &header : headers)
        {
            auto it = row.find(header);
            std::string value = (it != row.end()) ? it->second : "";
            std::cout << std::setw(15) << std::left << value;
        }
        std::cout << "\n";
    }
}

JSON serializationTOJSONNode(std::vector<std::map<std::string, std::string>> &rows)
{
    JSON jsonArray(JSONType::ARRAY);

    for (const auto &row : rows)
    {
        JSON obj(JSONType::OBJECT);
        for (const auto &pair : row)
        {
            obj[pair.first] = JSON(pair.second);
        }
        jsonArray.appendArray(obj);
    }
    return jsonArray;
}