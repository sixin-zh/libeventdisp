#
#  web server deamon
#	

SVRD_INC_DIR = .
SVRD_SRC_DIR = .
SVRD_BIN_DIR = /var/www

LIBEVENTDISP_INC_DIR = ../lib/inc
LIBEVENTDISP_SSO_DIR = ../build
LIBEVENTDISP_BIN_DIR = $(SVRD_BIN_DIR)/libsvrd


INCFLAGS += -I$(LIBEVENTDISP_INC_DIR)
LIBFLAGS += -L$(LIBEVENTDISP_BIN_DIR) -leventdisp -lpthread -lrt
CXXFLAGS += -g -Wall -Wextra

TARGETS = websvrd

.PHONY:

install: 	cleana migrate $(TARGETS) cleano start
		@echo TODO: export LD_LIBRARY_PATH="$(LIBEVENTDISP_BIN_DIR)"
		@echo Built OK ! 

%.od:   $(SVRD_SRC_DIR)/%.cc 
	@$(CXX) $(CXXFLAGS) $(INCFLAGS) $(LIBFLAGS) -I$(SVRD_INC_DIR) -c $< -o $@

websvrd: cache.od svr_common.od svr_tcp_ip.od svr_http.od websvrd.od
	 @$(CXX) $(CXXFLAGS) $(INCFLAGS) $(LIBFLAGS) $^ -o $(SVRD_BIN_DIR)/$@

.PHONY:
start:
	@echo Start server..
	@-cd $(SVRD_BIN_DIR) ; \
	./websvrd 

.PHONY:
cleano:	
	-rm -f *.o*

.PHONY:
cleana: 
	-rm -f *.o* $(TARGETS)


.PHONY: 
migrate:
	rm -fr $(LIBEVENTDISP_BIN_DIR)
	mkdir  $(LIBEVENTDISP_BIN_DIR)
	cp $(LIBEVENTDISP_SSO_DIR)/*.so $(LIBEVENTDISP_BIN_DIR)