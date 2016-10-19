CC		= g++
#CXXFLAGS	= -Wall -O2 -s -std=c++11
CXXFLAGS	= -Wall -pedantic -g -std=c++11
DEPS		= mdalc.h
OBJ		= mdalc.o module.o config.o sequence.o pattern.o table.o field.o command.o


mdalc: $(OBJ)
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(DEPS)
	$(CC) -c $(CXXFLAGS) -o $@ $< $(LDFLAGS)

.PHONY: clean
clean:
	rm *o
