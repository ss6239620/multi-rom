#include "MySQLQueryBuilder.h"
#include <iostream>

namespace ORM
{
    MySQLQueryBuilder::MySQLQueryBuilder(MYSQL *connection) : connection_(connection) {}

    QueryBuilder &MySQLQueryBuilder::select(const std::vector<std::string> &columns)
    {
        for (const auto &col : columns)
        {
            if (col.find('.') == std::string::npos && !lastAlias_.empty())
                selectColumns_.push_back(lastAlias_ + "." + col);
            else
                selectColumns_.push_back(col);
        }
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::alias(const std::string &table, const std::string &alias)
    {
        aliasMap_[table] = alias;
        lastAlias_ = alias;
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::from(const std::string &table)
    {
        if (aliasMap_.count(table))
        {
            fromTable_ = table + " AS " + aliasMap_[table];
            lastAlias_ = aliasMap_[table];
        }
        else
        {
            fromTable_ = table;
            lastAlias_ = table;
        }
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::count(const std::string &column, const std::string &alias)
    {
        std::string countExpr = "COUNT(";
        if (column.find('.') == std::string::npos && !lastAlias_.empty())
        {
            countExpr += lastAlias_ + "." + column;
        }
        else
        {
            countExpr += column;
        }
        countExpr += ")";
        if (!alias.empty())
        {
            countExpr += " AS " + alias;
        }
        selectColumns_.push_back(countExpr);
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::average(const std::string &column, const std::string &alias)
    {
        std::string str = "AVG(";
        if (column.find('.') == std::string::npos && !lastAlias_.empty())
        {
            str += lastAlias_ + "." + column;
        }
        else
        {
            str += column;
        }
        str += ")";
        if (!alias.empty())
        {
            str += " AS " + alias;
        }
        selectColumns_.push_back(str);
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::sum(const std::string &column, const std::string &alias)
    {
        std::string str = "SUM(";
        if (column.find('.') == std::string::npos && !lastAlias_.empty())
        {
            str += lastAlias_ + "." + column;
        }
        else
        {
            str += column;
        }
        str += ")";
        if (!alias.empty())
        {
            str += " AS " + alias;
        }
        selectColumns_.push_back(str);
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::min(const std::string &column, const std::string &alias)
    {
        std::string str = "MIN(";
        if (column.find('.') == std::string::npos && !lastAlias_.empty())
        {
            str += lastAlias_ + "." + column;
        }
        else
        {
            str += column;
        }
        str += ")";
        if (!alias.empty())
        {
            str += " AS " + alias;
        }
        selectColumns_.push_back(str);
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::max(const std::string &column, const std::string &alias)
    {
        std::string str = "MAX(";
        if (column.find('.') == std::string::npos && !lastAlias_.empty())
        {
            str += lastAlias_ + "." + column;
        }
        else
        {
            str += column;
        }
        str += ")";
        if (!alias.empty())
        {
            str += " AS " + alias;
        }
        selectColumns_.push_back(str);
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::join(const std::string &table, const std::string &condition, const std::string &type)
    {
        if (aliasMap_.count(table))
        {
            joinClauses_.push_back(type + " JOIN " + table + " AS " + aliasMap_[table] + " ON " + condition);
            lastAlias_ = aliasMap_[table];
        }
        else
        {
            joinClauses_.push_back(type + " JOIN " + table + " ON " + condition);
            lastAlias_ = table;
        }
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::leftJoin(const std::string &table, const std::string &condition)
    {
        return join(table, condition, "LEFT");
    }

    QueryBuilder &MySQLQueryBuilder::rightJoin(const std::string &table, const std::string &condition)
    {
        return join(table, condition, "RIGHT");
    }

    QueryBuilder &MySQLQueryBuilder::where(const std::string &condition)
    {
        whereClauses_.push_back(condition);
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::where(const std::string &column, const std::string &value)
    {
        std::string escapedValue = "'" + escapeString(value) + "'";
        whereClauses_.push_back(column + " = " + escapedValue);
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::groupBy(const std::vector<std::string> &columns)
    {
        groupByColumns_ = columns;
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::having(const std::string &condition)
    {
        havingclause_ = condition;
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::orderBy(const std::string &column, const std::string &direction)
    {
        orderByClauses_.push_back(column + " " + direction);
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::limit(int count)
    {
        limit_ = count;
        return *this;
    }

    QueryBuilder &MySQLQueryBuilder::offset(int count)
    {
        offset_ = count;
        return *this;
    }

    std::string MySQLQueryBuilder::build()
    {
        std::string query = "SELECT ";

        // ADD COLUMNS
        if (selectColumns_.empty())
        {
            query += "* ";
        }
        else
        {
            for (size_t i = 0; i < selectColumns_.size(); i++)
            {
                if (i > 0)
                    query += ", ";
                query += selectColumns_[i];
            }
        }

        // Add FROM (base table would need to be set)
        query += " FROM " + fromTable_;

        // Add JOINs
        for (const auto &join : joinClauses_)
        {
            query += " " + join;
        }

        // ADD where
        if (!whereClauses_.empty())
        {
            query += " WHERE ";
            for (size_t i = 0; i < whereClauses_.size(); i++)
            {
                if (i > 0)
                    query += " AND ";
                query += whereClauses_[i];
            }
        }

        // Add GROUP BY
        if (!groupByColumns_.empty())
        {
            query += " GROUP BY ";
            for (size_t i = 0; i < groupByColumns_.size(); i++)
            {
                if (i > 0)
                    query += ", ";
                query += groupByColumns_[i];
            }
        }

        // ADD having
        if (!havingclause_.empty())
        {
            query += " HAVING " + havingclause_;
        }

        // ADD ORDER BY
        if (!orderByClauses_.empty())
        {
            query += " ORDER BY ";
            for (size_t i = 0; i < orderByClauses_.size(); i++)
            {
                if (i > 0)
                    query += ", ";
                query += orderByClauses_[i];
            }
        }

        // Add LIMIT and OFFSET
        if (limit_ > 0)
        {
            query += " LIMIT " + std::to_string(limit_);
            if (offset_ > 0)
            {
                query += " OFFSET " + std::to_string(offset_);
            }
        }
        query += ";";
        return query;
    }

    std::string MySQLQueryBuilder::escapeString(const std::string &input) const
    {
        if (!connection_)
            return input;

        char *escaped = new char[input.length() * 2 + 1];
        mysql_real_escape_string(connection_, escaped, input.c_str(), input.length());
        std::string result(escaped);
        delete[] escaped;
        return result;
    }
}