#ifndef __CHAP_TY_H__
#define __CHAP_TY_H__

#include <stdint.h>

#define  TY_MOD_VERSION  "1.5.3"

#define prepare_ty_dial start_link_prepare
#define apply_ty_dial link_pre_establish

//"r0Vb5b?5s;(a7!JW"
#define TK_0 'r'
#define TK_1 '0'
#define TK_2 'V'
#define TK_3 'b'
#define TK_4 '5'
#define TK_5 'b'
#define TK_6 '?'
#define TK_7 '5'
#define TK_8 's'
#define TK_9 ';'
#define TK_10 '('
#define TK_11 'a'
#define TK_12 '7'
#define TK_13 '!'
#define TK_14 'J'
#define TK_15 'W'
#define TK_16 '\0'

extern bool ty_dial;		/* Wanna auth. ourselves with CHAP_TY */

void prepare_ty_dial(char *eas);
void apply_ty_dial(uint8_t *data);

#endif
