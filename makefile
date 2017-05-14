CC		= g++
#CXXFLAGS	= -Wall -O2 -s -std=c++11 -no-pie
CXXFLAGS	= -Wall -pedantic -g -pg -std=c++11 -no-pie
DEPS		= libmdal/mdal.h libmdal/pugixml.hpp
OBJ		= mdalc.o libmdal/module.o libmdal/config.o libmdal/sequence.o libmdal/blocks.o\
		 libmdal/field.o libmdal/command.o libmdal/helper_functions.o libmdal/pugixml.o


mdalc: $(OBJ)
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(DEPS)
	$(CC) -c $(CXXFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm *.o libmdal/*.o
