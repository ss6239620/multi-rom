#include "include/orm/MySQLAdapter.h"
#include "include/orm/ModelMacros.h"
#include <iostream>
#include "include/orm/utils.h"

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

int main()
{
    ORM::MySQLAdapter adapter;
    if (!adapter.connect("localhost", "testuser", "testpass", "testdb"))
    {
        std::cerr << "Connection failed: " << adapter.getLastError() << std::endl;
        return 1;
    }

    auto queryBuilder = adapter.createQueryBuilder();
    auto query = queryBuilder->count("*","total")
                     .from("profile")
                     .where("user_id=1")
                     .build();

    std::cout << query << std::endl;

    printRows(adapter.fetchAllFromQuery(query));

    adapter.disconnect();
    return 0;
}