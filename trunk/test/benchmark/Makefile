CXX = g++
CPPFLAGS += -g -Wall -Wextra -I../../lib/inc
CXXFLAGS += -L../../build -lpthread -leventdisp -lrt

TARGET = conv

all: $(TARGET)

%.o: %.cc %.h
	@$(CXX) $(CPPFLAGS) -c -o $@ $<

conv: conv.cc $(OBJ)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

.phony: clean
clean:
	@rm $(TARGET) $(OBJ)
