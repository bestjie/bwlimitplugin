BLUE    =\033[20;36m
GREEN   =\033[32m
RED     =\033[31m
ESC     =\033[0m
OK      =[$(GREEN) Ok $(ESC)]
FAILED  =[$(RED) failed $(ESC)]

CC=gcc



INCL=
LDFLAGS=
LIBS=-lc
CFLAGS=-Wall -g -shared -fPIC -DPIC


PLUGIN=bwlimitplugin.so

OBJECTS=\
  hash.o\
  config.o\
  bwlimitplugin.o
  

all: $(PLUGIN)

$(PLUGIN): $(OBJECTS)
	@echo -e 'BIN: $(GREEN) $(PLUGIN) $(ESC)'
	@$(CC) $(CFLAGS) $(OBJECTS) -o $(PLUGIN) $(LDFLAGS) $(LIBS)

%.o: %.cpp
	@echo -e 'OBJ: $(GREEN) $@ $(ESC)'
	@$(CC) $(INCL) $(CFLAGS) -o $@ -c $<

test: $(OBJECTS)
	@$(CC) -Wall $(OBJECTS) -o main $(LDFLAGS) $(LIBS)

clean:
	-rm $(PLUGIN) *.o 
