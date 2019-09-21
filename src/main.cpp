#include <iostream>
#include "ksh/playable_chart.hpp"

int main(int argc, char *argv[])
{
    if (argc >= 2)
    {
        ksh::PlayableChart chart(argv[1]);

        // TODO: Convert to kson
        std::cout << chart.comboCount() << std::endl;
    }
    else
    {
        std::cerr <<
            "Usage:\n"
            "./ksh2kson [ksh file]" << std::endl;
    }
    return 0;
}
