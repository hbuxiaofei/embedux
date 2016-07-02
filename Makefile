################默认生成RELEASE，DEBUG还是两者############################
#MAKE_DEFAULT=release
MAKE_DEFAULT=debug
#MAKE_DEFAULT=all


###################项目路径和程序名（需要配置）###########################
TARGET_NAME=libem.bin
TARGET_NAME_DEBUG=libem_debug.bin

DIR=.
BIN_DIR=$(DIR)/bin
INCLUDE_DIR=$(DIR)/src
SRC_DIR=$(DIR)/src


##########以下目录可以自己生成，可以不配置#############
OBJ_DIR=$(DIR)/obj
DEPS_DIR=$(DIR)/deps

OBJ_DIR_DEBUG=$(DIR)/obj_d
DEPS_DIR_DEBUG=$(DIR)/deps_d


###################编译选项及编译器（需要配置）##############################
CROSS=
CC=$(CROSS)gcc
SRC_FILE_TYPE=c

####for release
CFLAGS = -Wall -O2
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -D_GNU_SOURCE

####for debug
CFLAGS_DEBUG = -Wall -g3
CFLAGS_DEBUG += -ffunction-sections -fdata-sections
CFLAGS_DEBUG += -D_GNU_SOURCE
CFLAGS_DEBUG += -DWITH_DEBUG

LDFLAGS = -lpthread -ldl -lm -lrt
LDFLAGS += -Wl,-gc-sections

#####################从这里开始下面的内容一般不需要修改#########################
TARGET=$(BIN_DIR)/$(TARGET_NAME)
TARGET_DEBUG=$(BIN_DIR)/$(TARGET_NAME_DEBUG)
TARGET_DEBUG_STRIP= $(TARGET_DEBUG)_strip


###########################OBJ文件及路径########################################
SRC_FILES=$(shell find $(SRC_DIR) -name *.c -printf " %f")
vpath %.c $(shell find $(SRC_DIR) -name *.c -printf ":%h")

OBJS=$(patsubst %.$(SRC_FILE_TYPE), $(OBJ_DIR)/%.o,$(SRC_FILES))
DEPS=$(patsubst $(OBJ_DIR)/%.o, $(DEPS_DIR)/%.d, $(OBJS))

OBJS_DEBUG=$(patsubst %.$(SRC_FILE_TYPE), $(OBJ_DIR_DEBUG)/%.o,$(SRC_FILES))
DEPS_DEBUG=$(patsubst $(OBJ_DIR_DEBUG)/%.o, $(DEPS_DIR_DEBUG)/%.d, $(OBJS_DEBUG))




#########################INCLUDE头文件路径#######################################
INCLUDE= $(shell find $(INCLUDE_DIR) -name include -type d -printf " -I%h/%f")
        
##########################lib文件及路径#########################################


############################编译目标#############################################
.PHONY: all clean clean_debug clean_release clean_all debug debug_strip release  help 

$(MAKE_DEFAULT):

all:release debug

release:$(TARGET)

debug: $(TARGET_DEBUG)

debug_strip:$(TARGET_DEBUG)
	@echo ================================================================
	@echo $(TARGET_DEBUG_STRIP)
	@echo ================================================================	
	$(CROSS)strip $(TARGET_DEBUG)
	mv $(TARGET_DEBUG) $(TARGET_DEBUG_STRIP)
	@echo

$(TARGET):$(OBJS) 
	@echo 
	@echo ================================================================
	@echo $(TARGET)
	@echo ================================================================
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)
	$(CROSS)strip $(TARGET)
	@echo

$(TARGET_DEBUG):$(OBJS_DEBUG) 
	@echo 
	@echo ================================================================
	@echo $(TARGET_DEBUG)
	@echo ================================================================
	$(CC) -o $(TARGET_DEBUG) $(OBJS_DEBUG) $(LDFLAGS) 
	@echo


$(DEPS_DIR)/%.d: %.$(SRC_FILE_TYPE)
	@if [ ! -d $(DEPS_DIR) ]; then mkdir $(DEPS_DIR); fi
	@set -e; rm -f $@;\
	gcc -MM $(INCLUDE) $(CFLAGS) $< > $@.$$$$ || rm -f $@.$$$$ ; \
	sed -e 1's,^,$@ $(OBJ_DIR)/,' < $@.$$$$ > $@ ; \
	rm -f $@.$$$$
	@echo ==========update $@

$(DEPS_DIR_DEBUG)/%.d: %.$(SRC_FILE_TYPE)
	@if [ ! -d $(DEPS_DIR_DEBUG) ]; then mkdir $(DEPS_DIR_DEBUG); fi
	@set -e; rm -f $@;\
	gcc -MM $(INCLUDE) $(CFLAGS_DEBUG) $< > $@.$$$$ || rm -f $@.$$$$ ; \
	sed -e 1's,^,$@ $(OBJ_DIR_DEBUG)/,' < $@.$$$$ > $@ ; \
	rm -f $@.$$$$
	@echo ==========update $@
	
sinclude $(DEPS)
sinclude $(DEPS_DEBUG)

$(OBJ_DIR)/%.o:%.$(SRC_FILE_TYPE) 
	@if [ ! -d $(OBJ_DIR) ]; then mkdir $(OBJ_DIR); fi
	@echo 
	@echo ==========$<
	$(CC) $< -o $@ -c $(CFLAGS) $(INCLUDE) 

$(OBJ_DIR_DEBUG)/%.o:%.$(SRC_FILE_TYPE)
	@if [ ! -d $(OBJ_DIR_DEBUG) ]; then mkdir $(OBJ_DIR_DEBUG); fi
	@echo 
	@echo ==========$<
	$(CC) $< -o $@ -c $(CFLAGS_DEBUG) $(INCLUDE) 


clean:clean_$(MAKE_DEFAULT)

clean_all:clean_release clean_debug
	
clean_release:
	rm -f $(OBJS)  $(TARGET) 
	rm -rf $(DEPS_DIR)

clean_debug:
	rm -f $(OBJS_DEBUG)  $(TARGET_DEBUG) $(TARGET_DEBUG_STRIP)
	rm -rf $(DEPS_DIR_DEBUG)

help:
	@echo "Usage:"
	@echo "  make               -- you can set it to release,debug or all, now it is $(MAKE_DEFAULT)!"  
	@echo "  make release       -- generate release target!"  
	@echo "  make debug         -- generate debug target!"  
	@echo "  make debug_strip   -- generate debug target and strip it!" 
	@echo "  make all           -- generate release and debug target!" 
	@echo ""
	@echo "##Normally the following is not needed!" 
	@echo "  make clean         -- clean debug or release object files, according to make!" 
	@echo "  make clean_release -- clean release object files!" 
	@echo "  make clean_debug   -- clean debug object files!"  
	@echo "  make clean_all     -- clean all files!" 
	@echo ""   

#################################MAKEFILE结束##########################################
