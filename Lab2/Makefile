#Target Files
TARGET1 = mysort
TARGET2 = counter

#compiler for C and C++
CC = gcc
CXX = g++
DEPS = mcslock

#Flags for compiler
CFLAGS = -g -fpermissive -pthread -O3 -Wall -std=c++11 -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable 



all : $(TARGET1) $(TARGET2)

$(TARGET1) : $(TARGET1).o $(TARGET1).cpp
	$(CXX) $(CFLAGS) -o $(TARGET1) $(TARGET1).cpp 

$(TARGET2) : $(TARGET2).o $(TARGET2).cpp
	$(CXX) $(CFLAGS) -o $(TARGET2) $(TARGET2).cpp

$(TARGET1).o : $(TARGET1).cpp 
	g++ $(CFLAGS) $(DEPS).hpp -c $(TARGET1).cpp

$(TARGET2).o : $(TARGET2).cpp 
	g++ $(CFLAGS) $(DEPS).hpp -c $(TARGET2).cpp

clean :
	$(RM) $(TARGET1) $(TARGET2) 
