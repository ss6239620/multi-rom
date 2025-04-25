// include/orm/MySQLAdapter.h
#pragma once
#include "DatabaseTypes.h"
#include "MySQLQueryBuilder.h"
#include <mysql/mysql.h>
#include <map>

namespace ORM
{
    class MySQLAdapter : public DatabaseAdapter
    {
    public:
        MySQLAdapter();
        ~MySQLAdapter() override;

        bool connect(const std::string &host, const std::string &user,
                     const std::string &password, const std::string &dbname) override;
        bool createTable(const Model &model) override;
        bool insertRecord(const Model &model) override;
        void disconnect() override;
        std::string getLastError() const { return lastError_; }

        bool executeQuery(const std::string &query, MYSQL_RES *&result);

        std::vector<std::unordered_map<std::string, std::string>> fetchAllFromQuery(const std::string &query);

        std::unique_ptr<QueryBuilder> createQueryBuilder() override
        {
            return std::make_unique<MySQLQueryBuilder>(connection_);
        }

    private:
        MYSQL *connection_;
        std::string lastError_;

        std::string getTypeString(FieldType type, const FieldOptions &options) const;
        std::string escapeString(const std::string &input) const;
    };
}