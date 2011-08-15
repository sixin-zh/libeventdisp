CXX = g++
TEST = -DUSE_HASHING -DLIBEVENT_USE_SIG -g -std=c++0x -O2
RUN = -DUSE_HASHING -g -O2
CXXFLAGS = -Wall -Wextra -fPIC -lpthread -ltr -Iinc $(RUN)

OBJ = event_dispatcher.o mp_event_dispatcher.o thread.o dispatcher.o aio_wrapper.o \
  unit_task.o delayed_task.o
TARGET = libeventdisp.so

$(TARGET): $(OBJ)
	@echo $@
	@$(CXX) -shared -Wl,-soname,$@ -o $@ $^
	mv $@ ../build/

%.o: %.cc %.h
	@echo $@
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

.phony: clean

clean:
	rm *.o ../build/$(TARGET)

