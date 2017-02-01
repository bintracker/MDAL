CC		= g++
#CC		= clang++
#CXXFLAGS	= -Wall -O2 -s -std=c++11
CXXFLAGS	= -Wall -pedantic -g -std=c++11 -no-pie
#CXXFLAGS	= -Wall -pedantic -g -std=c++11 -libstd=libc++ -lc++abi  #-Weverything # still need to configure linker to actually use libc++
#LDFLAGS        = -libstd=libc++ -lc++abi
DEPS		= mdalc.h
OBJ		= mdalc.o blockList.o module.o config.o sequence.o block.o field.o command.o


mdalc: $(OBJ)
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(DEPS)
	$(CC) -c $(CXXFLAGS) -o $@ $< $(LDFLAGS)

.PHONY: clean
clean:
	rm *o
