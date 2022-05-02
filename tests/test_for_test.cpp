#include <iostream>
#include "pg/pgtest/pgtest.h"

PGTEST_CASE(test_for_test) {
    PGTEST_EXPECT(1 == 1);
    return true;
}