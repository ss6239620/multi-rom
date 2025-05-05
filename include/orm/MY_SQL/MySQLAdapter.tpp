// include/orm/MySQLAdapter.tpp
#ifndef MYSQL_ADAPTER_TPP
#define MYSQL_ADAPTER_TPP

#include <map>
#include <stdexcept>
#include <iostream>

namespace ORM
{

    template <typename ModelType>
    bool MySQLAdapter::insert(const std::map<std::string, std::string> &fields)
    {
        if (!connection_)
        {
            lastError_ = "Not connected to database";
            return false;
        }

        ModelType model;
        for (const auto &[fieldName, value] : fields)
        {
            try
            {
                model.setFieldValue(fieldName, value);
            }
            catch (const std::runtime_error &e)
            {
                lastError_ = e.what();
                return false;
            }
        }
        return insertRecord(model);
    }

    template <typename ModelType>
    ModelQuery<ModelType>::ModelQuery(MySQLAdapter *adapter, const std::string &operation)
        : adapter_(adapter), operation_(operation), builder_(adapter_->getConnection()) {}

    template <typename ModelType>
    ModelQuery<ModelType> &ModelQuery<ModelType>::where(const std::string &condition)
    {
        builder_.where(condition);
        return *this;
    }

    template <typename ModelType>
    ModelQuery<ModelType> &ModelQuery<ModelType>::join(const std::string &table, const std::string &condition, const std::string &type)
    {
        builder_.join(table, condition, type);
        return *this;
    }

    template <typename ModelType>
    ModelQuery<ModelType> &ModelQuery<ModelType>::set(const std::map<std::string, std::string> &fields)
    {
        ModelType model;
        for (const auto &[field, value] : fields)
        {
            // Validate field exists for model fields (not joined table fields)
            if (field.find('.') == std::string::npos)
            {
                model.setFieldValue(field, ""); // Throws if field doesn't exist
            }

            bool isExpression = (value.find('(') != std::string::npos || // function call like COALESCE(), NOW()
                                 value.find('+') != std::string::npos || // math operation
                                 value.find('-') != std::string::npos ||
                                 value.find('*') != std::string::npos ||
                                 value.find('/') != std::string::npos ||
                                 isdigit(value[0])); // numeric literal (e.g., 0.9)

            if (isExpression)

                setClauses_.push_back(field + " = " + value);
            else
                setClauses_.push_back(field + " = '" + adapter_->escapeString(value) + "'");
        }
        return *this;
    }

    template <typename ModelType>
    bool ModelQuery<ModelType>::execute()
    {
        ModelType model;
        std::string query;

        if (operation_ == "UPDATE")
        {
            query = "UPDATE " + model.getTableName();
            // Add joins
            auto joins = builder_.getJoinClause();
            for (const auto &join : joins)
            {
                query += " " + join;
            }
            // Add SET clause
            query += " SET ";
            for (size_t i = 0; i < setClauses_.size(); i++)
            {
                if (i > 0)
                    query += ", ";
                query += setClauses_[i];
            }
        }
        else if (operation_ == "DELETE")
        {
            query += "DELETE ";

            // For DELETE with joins, MySQL requires specifying tables
            auto joins = builder_.getJoinClause();

            if (!joins.empty())
            {
                query += model.getTableName() + " FROM " + model.getTableName();
                for (const auto &join : joins)
                {
                    query += " " + join;
                }
            }
            else
            {
                query += "FROM " + model.getTableName();
            }
        }
        // where clause
        auto whereClauses = builder_.getWhereClause();
        if (!whereClauses.empty())
        {
            query += " WHERE ";
            for (size_t i = 0; i < whereClauses.size(); i++)
            {
                if (i > 0)
                    query += " AND ";
                query += whereClauses[i];
            }
        }

        // ORDER BY clause
        auto orderByCLause = builder_.getOrderByClause();
        if (!orderByCLause.empty())
        {
            query += " ORDER BY ";
            for (size_t i = 0; i < orderByCLause.size(); i++)
            {
                if (i > 0)
                    query += ", ";
                query += orderByCLause[i];
            }
        }
        if (builder_.getLimit() > 0)
        {
            query += " LIMIT " + std::to_string(builder_.getLimit());
        }

        MYSQL_RES *temp = nullptr;
        bool success = adapter_->executeQuery(query, temp);
        if (temp)
            mysql_free_result(temp);
        return success;
    }
}
#endif // MYSQL_ADAPTER_TPP