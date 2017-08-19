#include <io.h>
#include <fcntl.h>

#include "binary_tests.h"

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int main(int argc, char* argv[])
{
    int previous_mode = _setmode(_fileno(stdout), _O_WTEXT);

    BinaryTests::DiscoverTests();

    _setmode(_fileno(stdout), previous_mode);

    int result = Catch::Session().run(argc, argv);
    return (result < 0xff ? result : 0xff);
}
