CXX = g++
CPPFLAGS += -g -Wall -Wextra -I../lib/inc
CXXFLAGS += -L../build -lpthread -leventdisp -lrt

TARGET = copy
OBJ = copy.o

all: $(TARGET)

%.o: %.cc %.h
	@$(CXX) $(CPPFLAGS) -c -o $@ $<

copy: copy.cc $(OBJ)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

.phony: clean
clean:
	@rm $(TARGET) $(OBJ)
