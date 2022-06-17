
 #
 # ssoo/xalloc.98/Makefile
 #
 # makefile de la practica de gestion de memoria dinamica
 #

CC= gcc
CFLAGS= -lm -g -Wall ${DEF}

OBJS_XALL= test_xalloc.o xalloc.o
OBJS_XRUN= xrun.o xalloc.o

test_xalloc: $(OBJS_XALL)
	$(CC) $(CFLAGS) -o $@ $(OBJS_XALL)

xrun: $(OBJS_XRUN)
	$(CC) $(CFLAGS) -o $@ $(OBJS_XRUN) -lm


#
# Se puede compilar para que xrun no llame al modulo xalloc (se hace xmalloc
# igual a malloc,...)
# 
# El comando que debe usarse para compilar esta version es:
#               make xrun DEF=-DNOXALL
#
