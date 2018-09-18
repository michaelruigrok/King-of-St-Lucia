CFLAGS=-pedantic -Wall -std=gnu99 -g
all: stlucia players

stlucia: koslShared.h koslShared.c hub.h hub.c
	gcc koslShared.c hub.c -o stlucia $(CFLAGS)

players: EAIT SCIENCE MABS HASS HABS

EAIT: koslShared.h koslShared.c player.h player.c eait.c
	gcc koslShared.c player.c eait.c -o EAIT $(CFLAGS)

SCIENCE: koslShared.h koslShared.c player.h player.c science.c
	gcc koslShared.c player.c science.c -o SCIENCE $(CFLAGS)

MABS: koslShared.h koslShared.c player.h player.c mabs.c
	gcc koslShared.c player.c mabs.c -o MABS $(CFLAGS)

HASS: koslShared.h koslShared.c player.h player.c hass.c
	gcc koslShared.c player.c hass.c -o HASS $(CFLAGS)

HABS: koslShared.h koslShared.c player.h player.c habs.c
	gcc koslShared.c player.c habs.c -o HABS $(CFLAGS)
