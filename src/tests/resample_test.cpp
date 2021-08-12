#include <doctest/doctest.h>

#include "../resampler.h"

TEST_CASE("resample")
{

    Resampler<16384> res;
    res.set_hz(22050);
    std::array<float, 800> data{};
    res.write(data.data(), data.size());
    res.write(data.data(), data.size());
    //REQUIRE(res.size() == 2);
}
