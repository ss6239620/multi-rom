// include/orm/MySQLAdapter.h
#pragma once
#include "DatabaseTypes.h"
#include "MySQLQueryBuilder.h"
#include <mysql/mysql.h>
#include <map>

namespace ORM
{
    class MySQLAdapter;

    template <typename ModelType>
    class ModelQuery
    {
    public:
        ModelQuery(MySQLAdapter *adapter, const std::string &operation);
        ModelQuery &where(const std::string &condition);
        ModelQuery &join(const std::string &table, const std::string &condition, const std::string &type = "INNER");
        ModelQuery &set(const std::map<std::string, std::string> &fields);

        bool execute();

    private:
        MySQLAdapter *adapter_;
        MySQLQueryBuilder builder_;
        std::string operation_;
        std::vector<std::string> setClauses_;
    };

    class MySQLAdapter : public DatabaseAdapter
    {
    public:
        MySQLAdapter();
        ~MySQLAdapter() override;

        bool connect(const std::string &host, const std::string &user,
                     const std::string &password, const std::string &dbname) override;
        bool createTable(const Model &model) override;
        std::string getCreateTableSTring(const Model &model) override;
        void disconnect() override;
        std::string getLastError() const override { return lastError_; }

        bool executeQuery(const std::string &query, MYSQL_RES *&result);

        std::vector<std::map<std::string, std::string>> executeQuery(
            const std::string &query, const std::vector<std::string> &params) override;

        bool executeRawQuery(
            const std::string &query, const std::vector<std::string> &params) override;

        std::vector<std::map<std::string, std::string>> fetchAllFromQuery(const std::string &query) override;

        std::unique_ptr<QueryBuilder> createQueryBuilder() override
        {
            return std::make_unique<MySQLQueryBuilder>(connection_);
        }

        template <typename ModelType>
        bool insert(const std::map<std::string, std::string> &fields);

        template <typename ModelType>
        ModelQuery<ModelType> update()
        {
            return ModelQuery<ModelType>(this, "UPDATE");
        }

        template <typename ModelType>
        ModelQuery<ModelType> delete_()
        {
            return ModelQuery<ModelType>(this, "DELETE");
        }

        MYSQL *getConnection() const { return connection_; }
        std::string escapeString(const std::string &input) const override;

    private:
        MYSQL *connection_;
        std::string lastError_;

        std::string getTypeString(FieldType type, const FieldOptions &options) const;
        bool insertRecord(const Model &model) override;
    };
}

// Include the template implementation at the end of the header
#include "MySQLAdapter.tpp"