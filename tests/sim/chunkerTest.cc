#include "../catch.hpp"
#include "chunker.hpp"
extern Configure config;


TEST_CASE("chunker.hpp")
{
    SECTION("attach")
    {
        int indexflag=config.getIndexflag();
        Chunker test("c",indexflag);
        test.chunking();
      
        REQUIRE( test.returnindex().chunknumber== 1254);

    }

}