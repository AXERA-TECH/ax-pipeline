#include "common_check.h"
#include "../utilities/ax_version_check.h"
#include "stdio.h"
void SAMPLE_Print_Logo()
{
    const char *logo = "\033[0m\033[1;32m"
                       "                                                                                    \n"
                       "           __   __       _____  _____  _____   ______  _       _____  _   _  ______ \n"
                       "     /\\    \\ \\ / /      |  __ \\|_   _||  __ \\ |  ____|| |     |_   _|| \\ | ||  ____|\n"
                       "    /  \\    \\ V /______ | |__) | | |  | |__) || |__   | |       | |  |  \\| || |__   \n"
                       "   / /\\ \\    > <|______||  ___/  | |  |  ___/ |  __|  | |       | |  | . ` ||  __|  \n"
                       "  / ____ \\  / . \\       | |     _| |_ | |     | |____ | |____  _| |_ | |\\  || |____ \n"
                       " /_/    \\_\\/_/ \\_\\      |_|    |_____||_|     |______||______||_____||_| \\_||______|\n"
                       "                                                                                    \n"
                       "                                                                                    \n\033[0m";
    printf("%s\n", logo);
}

int SAMPLE_Check_Bsp_Version()
{
    SAMPLE_Print_Logo();
#ifdef AXERA_TARGET_CHIP_AX620
    return 0;
#else
    return ax_version_check();
#endif
}
