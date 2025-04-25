// include/orm/MySQLAdapter.cpp
#include "MySQLAdapter.h"
#include <stdexcept>
#include <iostream>

namespace ORM
{
    MySQLAdapter::MySQLAdapter() : connection_(nullptr) {}

    MySQLAdapter::~MySQLAdapter()
    {
        disconnect();
    }

    bool MySQLAdapter::connect(const std::string &host, const std::string &user, const std::string &password, const std::string &dbname)
    {
        connection_ = mysql_init(nullptr);
        if (!connection_)
        {
            lastError_ = "MySQL initialization failed";
            return false;
        }

        if (!mysql_real_connect(connection_, host.c_str(), user.c_str(),
                                password.c_str(), dbname.c_str(), 0, nullptr, 0))
        {
            lastError_ = mysql_error(connection_);
            mysql_close(connection_);
            connection_ = nullptr;
            return false;
        }

        return true;
    }

    void MySQLAdapter::disconnect()
    {
        if (connection_)
        {
            mysql_close(connection_);
            connection_ = nullptr;
        }
    }

    std::string MySQLAdapter::getTypeString(FieldType type, const FieldOptions &options) const
    {
        switch (type)
        {
        case FieldType::INTEGER:
            return options.auto_increment ? "INT AUTO_INCREMENT" : "INT";
        case FieldType::FLOAT:
            return "FLOAT";
        case FieldType::DOUBLE:
            return "DOUBLE";
        case FieldType::BOOLEAN:
            return "BOOLEAN";
        case FieldType::DATETIME:
            return "DATETIME";
        case FieldType::TEXT:
            return "TEXT";
        case FieldType::BLOB:
            return "BLOB";
        case FieldType::STRING:
            if (options.max_length > 0)
            {
                return "VARCHAR(" + std::to_string(options.max_length) + ")";
            }
            return "TEXT";
        default:
            return "TEXT";
        }
    }

    std::string MySQLAdapter::escapeString(const std::string &input) const
    {
        if (!connection_)
            return input;

        char *escaped = new char[input.length() * 2 + 1];
        mysql_real_escape_string(connection_, escaped, input.c_str(), input.length());
        std::string result(escaped);
        delete[] escaped;
        return result;
    }

    bool MySQLAdapter::createTable(const Model &model)
    {
        if (!connection_)
        {
            lastError_ = "Not connected to database";
            return false;
        }

        std::string query = "CREATE TABLE IF NOT EXISTS " + model.getTableName() + " (";
        bool first = true;

        for (const auto &field : model.getFields())
        {
            if (!first)
                query += ", ";
            first = false;

            query += field->getName() + " " + getTypeString(field->getType(), field->getOptions());

            if (field->getOptions().primary_key)
            {
                query += " PRIMARY KEY";
            }
            if (field->getOptions().unique)
            {
                query += " UNIQUE";
            }
            if (!field->getOptions().nullable)
            {
                query += " NOT NULL";
            }
            if (!field->getOptions().default_value.empty())
            {
                query += " DEFAULT '" + escapeString(field->getOptions().default_value) + "'";
            }
        }

        query += ")";

        if (mysql_query(connection_, query.c_str()))
        {
            lastError_ = mysql_error(connection_);
            return false;
        }

        return true;
    }

    bool MySQLAdapter::insertRecord(const Model &model)
    {
        if (!connection_)
        {
            lastError_ = "Not connected to database";
            return false;
        }

        std::string query = "INSERT INTO " + model.getTableName() + " (";
        std::string values = "VALUES (";
        bool first = true;

        for (const auto &field : model.getFields())
        {
            if (field->getOptions().auto_increment)
                continue;

            std::string value = model.getFieldValue(field->getName());

            if (value.empty() && !field->getOptions().nullable)
            {
                if (!field->getOptions().default_value.empty())
                {
                    value = field->getOptions().default_value;
                }
                else
                {
                    lastError_ = "Field '" + field->getName() + "' cannot be NULL";
                    return false;
                }
            }

            if (!first)
            {
                query += ", ";
                values += ", ";
            }
            first = false;

            query += field->getName();

            if (value.empty())
            {
                values += "NULL";
            }
            else
            {
                switch (field->getType())
                {
                case FieldType::INTEGER:
                case FieldType::FLOAT:
                case FieldType::DOUBLE:
                case FieldType::BOOLEAN:
                    values += value;
                    break;
                default:
                    values += "'" + escapeString(value) + "'";
                }
            }
        }

        query += ") " + values + ")";
        if (mysql_query(connection_, query.c_str()))
        {
            lastError_ = mysql_error(connection_);
            return false;
        }

        return true;
    }

    bool MySQLAdapter::executeQuery(const std::string &query, MYSQL_RES *&result)
    {
        if (!connection_)
        {
            lastError_ = "Not connected to database";
            return false;
        }
        if (mysql_query(connection_, query.c_str()))
        {
            lastError_ = mysql_error(connection_);
            return false;
        }
        result = mysql_store_result(connection_);
        if (!result && mysql_field_count(connection_) > 0)
        {
            lastError_ = mysql_error(connection_);
            return false;
        }

        return true;
    }

    std::vector<std::unordered_map<std::string, std::string>> MySQLAdapter::fetchAllFromQuery(const std::string &query)
    {
        std::vector<std::unordered_map<std::string, std::string>> rows;
        MYSQL_RES *result = nullptr;

        if (!executeQuery(query, result))
        {
            return rows; // unsuccsessfull
        }
        int num_fields = mysql_num_fields(result);       // total rows
        MYSQL_FIELD *fields = mysql_fetch_field(result); // all field with name
        MYSQL_ROW row;

        while (row = mysql_fetch_row(result))
        {
            std::unordered_map<std::string, std::string> row_map;
            unsigned long *lengths = mysql_fetch_lengths(result);

            for (int i = 0; i < num_fields; i++)
            {
                std::string key = fields[i].name;
                std::string value = row[i] ? std::string(row[i], lengths[i]) : "NULL";
                row_map[key] = value;
            }
            rows.push_back(row_map);
        }
        mysql_free_result(result);
        return rows;
    }
}