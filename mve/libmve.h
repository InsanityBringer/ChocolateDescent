#ifndef _LIBMVE_H
#define _LIBMVE_H

#include "mve/mvelib.h"

#define MVE_ERR_EOF 1

int  MVE_rmPrepMovie(int filehandle, int x, int y, int track);
int  MVE_rmStepMovie();
void MVE_rmHoldMovie();
void MVE_rmEndMovie();

void MVE_sndInit(int x);

void MVE_sfCallbacks(mve_cb_ShowFrame *func);
void MVE_memVID(void* first, void* second, size_t len);
void MVE_palCallbacks(mve_cb_SetPalette* func);
void MVE_ReleaseMem();

#endif /* _LIBMVE_H */
