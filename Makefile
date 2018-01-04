CXX = g++
ARCHIVE = ar

all: libs app
locations:
	mkdir -p ./build/lib
	mkdir -p ./build/bin
libs: dynamic static
dynamic: locations src/*.cpp
	$(CXX) -fPIC -c src/*.cpp -I./ -I./nvidia/include -std=c++11 -lboost_system -lboost_filesystem -DUSE_BOOST_FILESYSTEM_3 $(shell pkg-config --libs --cflags opencv) -L /usr/lib/aarch64-linux-gnu/tegra/ -largus -lnvbuf_utils
	$(CXX) -shared -o build/lib/libarguscam.so *.o
	rm -rf *.o
	cp build/lib/libarguscam.so build/bin/

static: locations src/*.cpp
	$(CXX) -c src/*.cpp  -I./ -I ./nvidia/include -std=c++11 -lboost_system -lboost_filesystem -DUSE_BOOST_FILESYSTEM_3 $(shell pkg-config --libs --cflags opencv) -L /usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils -largus
	$(ARCHIVE) rs build/lib/libarguscam.a *.o
	rm -rf *.o

app: locations arguscamapp
arguscamapp: app/main.cpp
	$(CXX) app/main.cpp -o build/bin/$@ -L./build/lib/ -larguscam -I./ -I ./nvidia/include -I./ -std=c++11 $(shell pkg-config --libs --cflags opencv) -lboost_system -lboost_filesystem -lboost_program_options -lboost_date_time -L /usr/lib/aarch64-linux-gnu/tegra/ -largus -lnvbuf_utils

clean: 
	rm -rf tags *.o *.so *.a arguscamapp build/
