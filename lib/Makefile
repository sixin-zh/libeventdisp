CXX = g++
TEST = -DUSE_HASHING -DLIBEVENT_USE_SIG
CXXFLAGS = -Wall -Wextra -O2 -fPIC -lpthread -ltr -Iinc $(TEST)

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

