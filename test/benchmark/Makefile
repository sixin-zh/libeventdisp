CXX = g++
CPPFLAGS += -g -Wall -Wextra -I../../lib/inc
CXXFLAGS += -L../../build -lpthread -leventdisp -lrt

TARGET = conv conv_test
OBJ = conv_helper.o

all: $(TARGET)

%.o: %.cc %.h
	@$(CXX) $(CPPFLAGS) -c -o $@ $<

conv: conv.cc $(OBJ)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(OBJ) -o $@ $<

conv_test: conv_test.cc $(OBJ)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(OBJ) -o $@ $<


.phony: clean perf
clean:
	@rm $(TARGET) $(OBJ)

perf: CXXFLAGS += -lprofiler
perf: conv
