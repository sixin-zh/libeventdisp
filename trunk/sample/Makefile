CXX = g++
CPPFLAGS += -I../lib/inc
CXXFLAGS += -g -Wall -Wextra -L../build -lpthread -leventdisp -lrt

TARGET = copy
OBJ = copy.o

all: $(TARGET)

%.o: %.cc %.h
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

copy: copy.cc $(OBJ)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

.phony: clean
clean:
	@rm $(TARGET) $(OBJ)
