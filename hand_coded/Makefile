# Copyright QUB 2014

.PHONY : clean uninstall

SOURCE_DIR    := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

LIBEV_ROOT=/home/kostik/compilables/libev_4.24_install

LINK_LIBS     =-lstdc++ -lev -lpthread -L$(LIBEV_ROOT)/lib -lev

CXX_OPTS     :=  -I$(SOURCE_DIR) -I$(LIBEV_ROOT)/include -g

all : indie


CPP_SRCS=main.cpp trix.cpp common.cpp ewma.cpp dmi.cpp test_fixedptcpp.cpp vortex.cpp indicator_manager.cpp ad_line.cpp

CC_SRCS=running_stat.cc fixedpt_cpp.cc 

define depend_macro

%.o.d: $(1)
	@set -e; rm -f $$@; \
	$$(CC) -MM $$(CXXFLAGS) $$(CXX_OPTS) $$< > $$@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $$@ : ,g' < $$@.$$$$ > $$@; \
	rm -f $$@.$$$$

endef

$(eval $(call depend_macro,   $(SOURCE_DIR)/%.cpp))
$(eval $(call depend_macro,   $(SOURCE_DIR)/%.cc))
# ------- end dependencies

include $(CPP_SRCS:%.cpp=%.o.d)
include $(CC_SRCS:%.cc=%.o.d)

# a macro for the build patter rules
define build_macro

%.o : $(1)
	$${CC} $$< -o $$@ -c $${CXXFLAGS} $${CXX_OPTS}

endef

$(eval $(call build_macro, $(SOURCE_DIR)/%.cpp))
$(eval $(call build_macro, $(SOURCE_DIR)/%.cc))

# ------------------- MIR AGENT -------------------

GENERAL_FILES  = main.o running_stat.o trix.o fixedpt_cpp.o ewma.o dmi.o vortex.o indicator_manager.o ad_line.o

indie : $(GENERAL_FILES)
	${CC} $^ -o $@ ${CXXFLAGS} ${CXX_OPTS} ${LINK_LIBS} ${LDFLAGS}

test_fixedpt : fixedpt_cpp.o test_fixedptcpp.o
	${CC} $^ -o $@ ${CXXFLAGS} ${CXX_OPTS} -lstdc++ ${LDFLAGS}

clean_agent : 
	- rm $(GENERAL_FILES) $(GENERAL_FILES:%.o=%.o.d) indie

clean : clean_agent

