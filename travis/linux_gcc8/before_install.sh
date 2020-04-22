#!/bin/bash

set -e

# Remove pgdg.list (to avoid weird conflicts when installing i386 packages)
sudo rm -f /etc/apt/sources.list.d/pgdg.list

./travis/before_install_apt.sh
./travis/before_install_pip.sh

sudo apt-get install \
        libsqlite3-dev:$ARCH \
        libtiff-dev:$ARCH libcurl4-openssl-dev:$ARCH

sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install  g++-5
sudo apt-get install  g++-8-multilib 
sudo add-apt-repository -y --remove ppa:ubuntu-toolchain-r/test


sudo apt-get install \
        sqlite3 
sudo apt-get install make autoconf automake lcov \
        doxygen graphviz

#scripts/cppcheck.sh
#scripts/doxygen.sh

#pip install --user sphinxcontrib-bibtex
#pip install --user cpp-coveralls

#./travis/docker.sh
