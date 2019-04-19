SRC = src/cpp
LIB = lib
CC = g++
FLAGS = -fPIC -pthread -std=c++14

# the python interface through swig
PYTHONI = `python3-config --cflags`
PYTHONL = `python3-config --ldflags`
BOOSTI = -I/usr/local/Cellar/boost/1.69.0/include


all:
	$(shell mkdir -p $(LIB))
	#$(#shell cp $(SRC)/bzpy.h $(LIB))
	$(shell mkdir -p $(LIB))
	swig -c++ -python -o $(LIB)/bzpy_wrap.cxx swig/py/bzpy.i
	#$(CC) $(FLAGS) $(BOOSTI) -c $(SRC)/bzpy.cpp -o $(LIB)/bzpy.o
	#$(CC) $(FLAGS) $(PYTHONI) $(BOOSTI) -c $(LIB)/bzpy_wrap.cxx -o $(LIB)/bzpy_wrap.o
	#$(CC) $(PYTHONL) $(LIBFLAGS) -shared $(LIB)/bzpy.o $(LIB)/bzpy_wrap.o -o $(LIB)/_bzpy.so

clean:
	-rm -rf $(LIB)
