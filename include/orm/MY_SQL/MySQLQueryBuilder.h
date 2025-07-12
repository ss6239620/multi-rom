#pragma once
#include "DatabaseTypes.h"
#include <mysql/mysql.h>

namespace ORM
{
    class MySQLQueryBuilder : public QueryBuilder
    {
    public:
        MySQLQueryBuilder(MYSQL *connection);

        QueryBuilder &select(const std::vector<std::string> &columns = {"*"}) override;
        QueryBuilder &from(const std::string &table) override;
        QueryBuilder &alias(const std::string &table, const std::string &alias) override;

        // Aggregate functions
        QueryBuilder &count(const std::string &column = "*", const std::string &alias = "") override;
        QueryBuilder &average(const std::string &column, const std::string &alias) override;
        QueryBuilder &sum(const std::string &column, const std::string &alias) override;
        QueryBuilder &min(const std::string &column, const std::string &alias) override;
        QueryBuilder &max(const std::string &column, const std::string &alias) override;

        QueryBuilder &join(const std::string &table, const std::string &condition, const std::string &type = "INNER") override;
        QueryBuilder &leftJoin(const std::string &table, const std::string &condition) override;
        QueryBuilder &rightJoin(const std::string &table, const std::string &condition) override;

        // WHERE clauses
        QueryBuilder &where(const std::string &condition) override;
        QueryBuilder &where(const std::string &column, const std::string &value) override;

        // GROUP BY and HAVING
        QueryBuilder &groupBy(const std::vector<std::string> &columns) override;
        QueryBuilder &having(const std::string &condition) override;

        // ORDER BY
        QueryBuilder &orderBy(const std::string &column, const std::string &direction = "ASC") override;

        // LIMIT and OFFSET
        QueryBuilder &limit(int count) override;
        QueryBuilder &offset(int count) override;

        std::string escapeString(const std::string &input) const;

        std::string build() override;

        void reset() override;

        const std::vector<std::string> &getWhereClause() const { return whereClauses_; }
        const std::vector<std::string> &getJoinClause() const { return joinClauses_; }
        const std::vector<std::string> &getOrderByClause() const { return orderByClauses_; }
        const int &getLimit() const { return limit_; }

    private:
        MYSQL *connection_;
        std::unordered_map<std::string, std::string> aliasMap_;
        std::string query_;
        std::string fromTable_;
        std::vector<std::string> selectColumns_;
        std::vector<std::string> joinClauses_;
        std::vector<std::string> whereClauses_;
        std::vector<std::string> groupByColumns_;
        std::string lastAlias_;
        std::string havingclause_;
        std::vector<std::string> orderByClauses_;
        int limit_ = -1;
        int offset_ = -1;
    };
}