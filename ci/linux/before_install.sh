export DEBIAN_FRONTEND=noninteractive
#sudo add-apt-repository ppa:apokluda/boost1.53 --yes
sudo add-apt-repository ppa:boost-latest/ppa --yes
sudo add-apt-repository ppa:kalakris/cmake --yes # CMAKE 2.8.11
sudo add-apt-repository ppa:ubuntu-toolchain-r/test --yes # g++
sudo apt-get update -qq
sudo apt-get install --force-yes \
    g++-4.8 \
    cmake libboost-chrono1.55-dev libboost-program-options1.55-dev libboost-timer1.55-dev \
    libboost-test1.55-dev libboost-date-time1.55-dev libboost-thread1.55-dev \
    libboost-system1.55-dev libboost-serialization1.55-dev \
    libshp1 libshp-dev libfcgi-dev pyqt4-dev-tools libprotobuf-dev libosmpbf-dev
    
export CXX="g++-4.8"
