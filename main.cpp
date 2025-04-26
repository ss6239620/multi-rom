#include "include/orm/MY_SQL/MySQLAdapter.h"
#include "include/orm/ModelMacros.h"
#include <iostream>
#include "utils/utils.h"
#include "serializer/jsonparser.h"

BEGIN_MODEL_DEFINITION(User, "users")
FIELD(id, INTEGER, .primary_key = true, .auto_increment = true)
FIELD(username, STRING, .nullable = false, .unique = true, .max_length = 50)
FIELD(email, STRING, .nullable = false, .unique = false, .max_length = 100)
FIELD(created_at, DATETIME)
FIELD(is_active, BOOLEAN, .default_value = "1")
END_MODEL_DEFINITION()

BEGIN_MODEL_DEFINITION(Profile, "profile")
FIELD(id, INTEGER, .primary_key = true, .auto_increment = true)
FIELD(user_id, INTEGER, .nullable = false)
FIELD(fullname, STRING, .nullable = false, .unique = true, .max_length = 50)
FIELD(bio, STRING, .nullable = false, .unique = false, .max_length = 100)
FIELD(created_at, DATETIME)
END_MODEL_DEFINITION()

BEGIN_MODEL_DEFINITION(Account, "account")
FIELD(id, INTEGER, .primary_key = true, .auto_increment = true)
FIELD(user_id, INTEGER, .nullable = false)
FIELD(price, INTEGER, .nullable = false, .unique = true, .max_length = 50)
FIELD(created_at, DATETIME)
END_MODEL_DEFINITION()

int main()
{
    ORM::MySQLAdapter adapter;
    if (!adapter.connect("localhost", "testuser", "testpass", "testdb"))
    {
        std::cerr << "Connection failed: " << adapter.getLastError() << std::endl;
        return 1;
    }

    bool res = adapter.delete_<Account>()
                   .where("id = 1")
                   .execute();

    if (!res)
    {
        std::cout << "Insertion Failed: " << adapter.getLastError() << std::endl;
    }

    auto queryBuilder = adapter.createQueryBuilder();
    auto query = queryBuilder->select({})
                     .from("account")
                     .build();

    std::cout << query << std::endl;

    printRows(adapter.fetchAllFromQuery(query));

    adapter.disconnect();
    return 0;
}