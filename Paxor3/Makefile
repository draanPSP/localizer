# Project: Paxor
# Makefile created by Dev-C++ 4.9.9.2

CPP  = g++.exe -D__DEBUG__
CC   = gcc.exe -D__DEBUG__
WINDRES = windres.exe
RES  = 
OBJ  = main.o md5.o parse.o utils.o $(RES)
LINKOBJ  = main.o md5.o parse.o utils.o $(RES)
LIBS =  -L"C:/Dev-Cpp/lib"   -g3 
INCS =  -I"C:/Dev-Cpp/include" 
CXXINCS =  -I"C:/Dev-Cpp/lib/gcc/mingw32/3.4.2/include"  -I"C:/Dev-Cpp/include/c++/3.4.2/backward"  -I"C:/Dev-Cpp/include/c++/3.4.2/mingw32"  -I"C:/Dev-Cpp/include/c++/3.4.2"  -I"C:/Dev-Cpp/include" 
BIN  = Paxor.exe
CXXFLAGS = $(CXXINCS) -fshort-wchar   -g3
CFLAGS = $(INCS) -fshort-wchar   -g3
RM = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before Paxor.exe all-after


clean: clean-custom
	${RM} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LINKOBJ) -o "Paxor.exe" $(LIBS)

main.o: main.cpp
	$(CC) -c main.cpp -o main.o $(CFLAGS)

md5.o: md5.c
	$(CC) -c md5.c -o md5.o $(CFLAGS)

parse.o: parse.cpp
	$(CC) -c parse.cpp -o parse.o $(CFLAGS)

utils.o: utils.cpp
	$(CC) -c utils.cpp -o utils.o $(CFLAGS)
