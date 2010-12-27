CC=gcc
INCLUDE_DIR =inc 
CC_FLAG=-Wall -g
SRC=src scm .
OBJ_FLAGS=-Wall -c 
#LDFLAGS +=-lz
EXECUTABLES=a.out
SOURCES=$(foreach DIR, $(SRC),$(wildcard $(DIR)/*.c)) 
INCLUDE += $(foreach includedir, $(INCLUDE_DIR),-I $(includedir))
INCLUDE_FILES += $(foreach includedir, $(INCLUDE_DIR),$(includedir)/*.h)
OBJECTS := $(addprefix .objs/,$(SOURCES:.c=.o)) 

all: ${EXECUTABLES}

opt1: ${EXECUTABLES}
	${CC} ${INCLUDE} ${CC_FLAG} -O1 ${LDFLAGS}  ${OBJECTS} -o $@
opt2: ${EXECUTABLES}
	${CC} ${INCLUDE} ${CC_FLAG} -O2 ${LDFLAGS}  ${OBJECTS} -o $@
opt3: ${EXECUTABLES}
	${CC} ${INCLUDE} ${CC_FLAG} -O3 ${LDFLAGS}  ${OBJECTS} -o $@


.PHONY: all clean distclean

${EXECUTABLES}: ${OBJECTS} ${INCLUDE_FILES}
	${CC} ${INCLUDE} ${CC_FLAG} ${LDFLAGS}  ${OBJECTS} -o $@  && ctags -R 

#if any header files gets modified compile the whole project again
.objs/%.o: %.c ${INCLUDE_FILES}
	${CC} ${INCLUDE} ${OBJ_FLAGS} $< -o $@

clean:
	 rm -f ${EXECUTABLES} ${OBJECTS} 
cleanscm:
	rm -rf .scm
distclean: clean

