#ifndef __CHAP_TY_H__
#define __CHAP_TY_H__

#include <stdint.h>

#define  TY_MOD_VERSION  "4.0"
#define  TY_VERSION  "1.5.6"

extern bool ty_dial;		/* Wanna auth. ourselves with CHAP_TY */

void prepare_ty_dial();
void apply_ty_dial(uint8_t *data);

#endif
