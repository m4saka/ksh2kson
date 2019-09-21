#include <iostream>
#include "ksh/playable_chart.hpp"

int main()
{
    ksh::PlayableChart chart("catsjumpanywhere_ex.ksh");
    std::cout << chart.comboCount() << std::endl;
    return 0;
}
