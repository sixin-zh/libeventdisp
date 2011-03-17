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
CXXFLAGS  += -g -Wall -Wextra

TARGETS = websvrd clientd

all: 	cleana $(TARGETS) cleano
	@echo TODO: export LD_LIBRARY_PATH="$(LIBEVENTDISP_BIN_DIR)/"
	@echo Built OK ! 

websvrd : svr_common.o svr_tcp_ip.o svr_http.o websvrd.o
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(LIBFLAGS) $^ -o $(SVRD_BIN_DIR)/$@

clientd : svr_common.o svr_tcp_ip.o svr_http.o clientd.o
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(LIBFLAGS) $^ -o $(SVRD_BIN_DIR)/$@

%.o: $(SVRD_SRC_DIR)/%.cc 
	@$(CXX) $(CXXFLAGS) $(INCFLAGS) $(LIBFLAGS) -c $< -o $@

cleano:	
	-rm -f *.o 

cleana: 
	-rm -f *.o $(TARGETS)

.PHONY: all cleano cleana
