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

        MYSQL *getConnection() const { return connection_; }
        std::string escapeString(const std::string &input) const override;

        // Insert operations
        template <typename ModelType>
        bool insert(const std::map<std::string, std::string> &fields);

        template <typename ModelType>
        bool bulkInsert(const std::vector<std::map<std::string, std::string>> &entities);

        // select operations
        template <typename ModelType>
        std::vector<std::map<std::string, std::string>> find();

        template <typename ModelType>
        std::map<std::string, std::string> findOne(const std::string &condition);

        template <typename ModelType>
        std::map<std::string, std::string> findById(const std::string &id);

        template <typename ModelType>
        std::vector<std::map<std::string, std::string>> findBy(const std::string &condition);

        template <typename ModelType>
        std::pair<std::vector<std::map<std::string, std::string>>, int> findAndCount();

        template <typename ModelType>
        bool exists(const std::string &condition);

        template <typename ModelType>
        int count(const std::string &condition);

        // Update Operations
        template <typename ModelType>
        bool update(const std::map<std::string, std::string> &critria, const std::map<std::string, std::string> &partialEntity);

        template <typename ModelType>
        bool updateById(const std::string &id, const std::map<std::string, std::string> &partialEntity);

        template <typename ModelType>
        ModelType create(const std::initializer_list<std::pair<std::string, std::string>> &fields);

        template <typename ModelType>
        bool save(const ModelType &entity);

        template <typename ModelType>
        bool bulkUpddate(const std::map<std::string, std::string> &critria, const std::map<std::string, std::string> &updates);

        template <typename ModelType>
        bool increment(const std::string &field, int value, const std::string &condition = "");

        template <typename ModelType>
        bool decrement(const std::string &field, int value, const std::string &condition = "");

        // Update Operations
        template <typename ModelType>
        bool delete_(const std::map<std::string, std::string> &critria);

        template <typename ModelType>
        bool deleteById(const std::string &id);

        template <typename ModelType>
        bool softDelete(const std::map<std::string, std::string> &critria, const std::string &deleteColumn = "is_deleted");

        template <typename ModelType>
        bool remove(const ModelType &entity);

    private:
        MYSQL *connection_;
        std::string lastError_;
        MySQLQueryBuilder queryBuilder_;

        std::string getTypeString(FieldType type, const FieldOptions &options) const;
        bool insertRecord(const Model &model) override;
    };
}

// Include the template implementation at the end of the header
#include "MySQLAdapter.tpp"