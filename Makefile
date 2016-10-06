###############################################################
#                                                             #
#            Make File Modified by Ziho Shin                  # 
#                                                             #
###############################################################
CC      = gcc
LIBDIPC   = ../libfinsXdcom.a
CFLAGS  = -Wall -D__USER__ -I ../

all: finsdcomRx_mod finsdcomTx_mod #Server Client

#finsdcomRx: finsdcomRx.c
#	$(CC) $(CFLAGS) $^ -o finsdcomRx $(LIBDIPC) -lpthread

finsdcomRx_mod: finsdcomRx_mod.c
	$(CC) $(CFLAGS) $^ -o finsdcomRx_mod $(LIBDIPC) -lpthread -lrt

#finsdcomTx: finsdcomTx.c
#	$(CC) $(CFLAGS) $^ -o finsdcomTx $(LIBDIPC) -lpthread

finsdcomTx_mod : finsdcomTx_mod.c
	$(CC) $(CFLAGS) $^ -o finsdcomTx_mod $(LIBDIPC) -lpthread -lrt
	
#finsdcomDmaTx: finsdcomDmaTx.c
#	$(CC) $(CFLAGS) $^ -o finsdcomDmaTx $(LIBDIPC) -lpthread

#Server : socket_server5.c
#	$(CC) $(CFLAGS) $^ -o Server -lrt
#Client : socket_client3.c
#	$(CC) $(CFLAGS) $^ -o Client -lrt
	
Clean:
	rm -rf finscomTx_mod finscomRx_mod #Server Client
