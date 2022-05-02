#include "pg/pgfwd.h"

#undef PGZXB_DEBUG_INFO_HEADER
#define PGZXB_DEBUG_INFO_HEADER "[YunGameServer-LOG] "

int main() {
    PGZXB_DEBUG_Print("Starting YunGameServer ---");

    PGZXB_DEBUG_Print("Exiting YunGameServer  ---");
    return 0;
}