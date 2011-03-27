CXX = g++
CPPFLAGS += -g -Wall -Wextra -I../../lib/inc
CXXFLAGS += -L../../build -lpthread -leventdisp -lrt

TARGET = conv

all: $(TARGET)

%.o: %.cc %.h
	@$(CXX) $(CPPFLAGS) -c -o $@ $<

conv: conv.cc $(OBJ)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

.phony: clean perf
clean:
	@rm $(TARGET) $(OBJ)

perf: CXXFLAGS += -lprofiler
perf: conv
