#!/bin/sh
#Install all dependencies in Ubuntu-based linux implementations

sudo apt-get install -y libtbb-dev libevent-dev gcc g++ luarocks lua5.1 lua5.1-dev make

#comment if you don't want to plot graphs
sudo apt-get install -y graphviz graphviz-dev libgraph-easy-perl && sudo luarocks install luagraph

sudo luarocks install luasocket CC="gcc -fPIC"

#Don't need this
#sudo luarocks install leda

make
sudo make install

#Testing
lua examples/hello_world.lua 

#to uninstall
#sudo make uninstall
