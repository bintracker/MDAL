CC		= g++
#CXXFLAGS	= -Wall -O2 -s -std=c++11
CXXFLAGS	= -Wall -pedantic -g -pg -std=c++11 -no-pie
DEPS		= mdalc.h pugixml.hpp
OBJ		= mdalc.o module.o config.o sequence.o blocks.o field.o command.o pugixml.o


mdalc: $(OBJ)
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(DEPS)
	$(CC) -c $(CXXFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm *o
