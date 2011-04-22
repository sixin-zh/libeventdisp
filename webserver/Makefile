CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -fPIC -lpthread -ltr -Iinc

TARGET: cache.o

%.o: %.cc %.h
	@echo $@
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

.phony: clean

clean:
	rm *.o