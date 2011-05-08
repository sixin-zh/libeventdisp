CXX = g++
CPPFLAGS += -g -Wall -Wextra -I../../lib/inc
CXXFLAGS += -L../../build -lpthread -leventdisp -lrt #-DCHECK_CORRECTNESS

TARGET = conv count count_ns

all: $(TARGET)

%.o: %.cc %.h
	@$(CXX) $(CPPFLAGS) -c -o $@ $<

conv: conv.cc conv_helper.o
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^

count: char_count.cc char_count_helper.o
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^

count_ns: char_count_no_split.cc char_count_helper.o
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^

.phony: clean perf
clean:
	@rm $(TARGET) *.o

perf: CXXFLAGS += -lprofiler
perf: conv
