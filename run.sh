#!/bin/bash
if [ -e /etc/debian_version ]
then
    echo "Debian based OS found, the required packages will be installed automatically"
        sudo apt install cmake python3 python3-virtualenv python3-pip python3-tk libssl-dev libgtk-3-dev build-essential libsdl2-dev socat at-spi2-core libncurses5-dev -y
else
    echo "Make sure the following packets are installed:"
    echo "cmake"
    echo "python3"
    echo "python3-virtualenv"
    echo "python3-pip"
    echo "python3-tk"
    echo "libssl-dev"
    echo "libgtk-3-dev" 
    echo "build-essential"
    echo "libsdl2-dev"
    echo "socat"
    echo "at-spi2-core"
    echo "libncurses5-dev"
fi

echo "Building penne_ecu project"
mkdir -p penne_ecu/build
cd penne_ecu/build
cmake ..
cmake --build .
cd ../..

echo "Creating virtual environment"
mkdir -p env
virtualenv -q env/
echo "Activating virtual environment"
source env/bin/activate
pip install --upgrade pip
echo "Checking pip requirements"
pip install -q -U -r requirements.txt
echo "Setting up CAN interface"
if ! grep -wEq "vcan" /proc/modules ; then
    sudo modprobe vcan
fi
if ! ip addr | grep -q "vcan0" ; then
    sudo ip link add dev vcan0 type vcan
    ip link set vcan0 mtu 72
fi
if ! ip addr | grep -q "vcan1" ; then
    sudo ip link add dev vcan1 type vcan
    ip link set vcan1 mtu 72
fi
if ip addr | grep vcan0 | grep -q "DOWN"; then
    sudo ip link set up vcan0
fi
if ip addr | grep vcan1 | grep -q "DOWN"; then
    sudo ip link set up vcan1
fi

echo "Starting GUI"
python3 pygame_gui/main.py

