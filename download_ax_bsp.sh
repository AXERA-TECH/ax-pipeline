#!/bin/sh
echo "\e[32mTips: \n   follow docs/how_to_speed_up_submodule_init.md if you this process too slow\e[0m\n"
chip=$1

if [ "$chip" = "ax620" ]
then
    echo "clone ax620 bsp to axpi_bsp_sdk, please wait..."
    git clone https://github.com/sipeed/axpi_bsp_sdk.git
elif [ "$chip" = "ax650" ]
then
    echo "clone ax650 bsp to ax650n_bsp_sdk, please wait..."
    git clone https://github.com/AXERA-TECH/ax650n_bsp_sdk.git
else
    echo "supported chips: \n   \e[34m[ax620, ax650]\e[0m\n"
    echo "for example: \n   \e[34m$ ./download_ax_bsp.sh ax620\e[0m"
fi