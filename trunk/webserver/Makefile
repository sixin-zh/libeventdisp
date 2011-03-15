#
#  Web Server
#

SVRD_INC_DIR = ./inc
SVRD_SRC_DIR = ./src
SVRD_BIN_DIR = .

LIBEVENTDISP_INC_DIR = ../lib/inc
LIBEVENTDISP_BIN_DIR = ../build

INCFLAGS += -I$(SVRD_INC_DIR) -I$(LIBEVENTDISP_INC_DIR)
LIBFLAGS += -L$(LIBEVENTDISP_BIN_DIR) -leventdisp -lpthread -lrt
CXFLAGS  += -g -Wall -Wextra


all: 	clean cli srv

clean:	
	-rm -f *.o

cli : 	client1.o

client1.o : $(SVRD_SRC_DIR)/client1.cc
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(LIBFLAGS) $< -o $(SVRD_BIN_DIR)/$@

srv : websvrd.o
	export LD_LIBRARY_PATH="$(LIBEVENTDISP_BIN_DIR)/"
#	$(SVRD_BIN_DIR)/websvrd.o &


	
websvrd.o : $(SVRD_SRC_DIR)/websvrd.cc
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(LIBFLAGS) $< -o $(SVRD_BIN_DIR)/$@



.PHONY: all clean cli srv
