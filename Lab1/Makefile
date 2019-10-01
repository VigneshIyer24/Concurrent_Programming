#compiler for C and C++
CC = gcc
CXX = g++

#Flags for compiler
CFLAGS = -g -fpermissive -pthread -O3 -Wall 

#Target Files
TARGET = mysort

all : $(TARGET)

$(TARGET) : $(TARGET).cpp
	$(CXX) $(CFLAGS) -o $(TARGET) $(TARGET).cpp

clean :
	$(RM) $(TARGET)
