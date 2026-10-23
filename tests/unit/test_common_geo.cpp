#include <cassert>
#include <iostream>
#include "aegis/common/types.hpp"
#include "../../src/common/geo_utils.hpp"

int main() {
    std::cout << "[RUN] Unit Test: test_common_geo" << std::endl;
    aegis::common::GeoCoordinate a{12.971598, 77.594562, 0.0};
    aegis::common::GeoCoordinate b{12.973412, 77.597120, 0.0};
    double dist = aegis::common::CalculateHaversineDistance(a, b);
    assert(dist >= 0.0);
    std::cout << "[PASS] Unit Test: test_common_geo" << std::endl;
    return 0;
}
