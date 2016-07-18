for Debian:

$ sudo apt-get install git
$ sudo apt-get install cmake
$ sudo apt-get install protobuf-compiler
$ sudo apt-get install libprotoc-dev
$ sudo apt-get install g++
$ sudo apt-get install libboost-all-dev

$ mkdir github
$ cd github
$ git clone https://github.com/newenclave/ferro-remote.git
$ cd ferro-remote

$ git submodule init        # for lua_client
$ git submodule update lua  # for lua_client
$ cd ../                    # for lua_client

$ mkdir build
$ cd build
$ cmake ../
.....
PROFIT

