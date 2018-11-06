/**
 * \file    ubloxconn.c
 * \brief   UBlox module connection
 * \author  Yunhui Fu (yhfudev@gmail.com)
 * \version 1.0
 * \date    2018-11-06
 * \copyright GPL/BSD
 */


#if defined(CIUT_ENABLED) && (CIUT_ENABLED == 1)
#include <ciut.h>

TEST_CASE( .name="test ublox inner function", .description="Test ublox inner functions." ) {

    SECTION("test ublox") {
        REQUIRE(0 == 0);
        CIUT_LOG ("ublox? %d", 0);
    }
}
#endif /* CIUT_ENABLED */


