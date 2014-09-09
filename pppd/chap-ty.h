#ifndef __CHAP_TY_H__
#define __CHAP_TY_H__

#include <stdint.h>

extern bool ty_dial;		/* Wanna auth. ourselves with CHAP_TY */

void prepare_ty_dial();
void apply_ty_dial(uint8_t *data);

#endif
