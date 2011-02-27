CXX = g++
CXXFLAGS = -Wall -O2 -fPIC -lpthread -Iinc

OBJ = event_dispatcher.o mp_event_dispatcher.o thread.o dispatcher.o aio_wrapper.o
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

