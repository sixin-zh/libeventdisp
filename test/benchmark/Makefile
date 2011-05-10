CXX = g++
CPPFLAGS += -g -Wall -Wextra -I../../lib/inc
CXXFLAGS += -L../../build -lpthread -leventdisp -lrt #-DCHECK_CORRECTNESS

TARGET = conv count

all: $(TARGET)

%.o: %.cc %.h
	@$(CXX) $(CPPFLAGS) -c -o $@ $<

conv: conv.cc conv_helper.o
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^

count: word_count.cc count_helper.o
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^

.phony: clean perf
clean:
	@rm $(TARGET) *.o

perf: CXXFLAGS += -lprofiler
perf: conv
