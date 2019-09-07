#compiler for C and C++
CC = gcc
CXX = g++

#Flags for compiler
CFLAGS = -g -Wall 

#Target Files
TARGET = Lab0_sort

all : $(TARGET)

$(TARGET) : $(TARGET).cpp
	$(CXX) $(CFLAGS) -o $(TARGET) $(TARGET).cpp

clean : 
	$(RM) $(TARGET)
