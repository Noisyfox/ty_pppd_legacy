#include <string.h>
#include <stdlib.h>

#include "pppd.h"
#include "chap-ty.h"

bool ty_dial = 0;		/* Wanna auth. ourselves with CHAP_TY */

uint8_t CFG_TEA_KEY[] = "r0Vb5b?5s;(a7!JW";
const char pdext_fn[] = "/root/%s%s";
const char pdext_default[] = "omTelUnRVG/EQXfcvf7T0GHjfYK0Mlx5D1PAIxedgFCbdQj4u"
	"++iNdIyRPiSP7urRaZr5SEzQYEicTNLFC9Xkryfy8oAg7hOkIcLeWNHdQMPGQ3OV8Dt/tBo22O58Gi"
	"VpuO8LIm6pnLc3yjuYytgN9XkrONX5qcX+4fBgVyKwM2jzYn0gKNo4yKhU452bAOQuKXwSzDBpg+9v"
	"1Qab8Ze44YwNCYrxne1DvW5zesT6jW=";
const size_t MAX_FILE_SIZE = 4096;

char user_prefix[MAXNAMELEN] = "";
unsigned char ty_salt[16] = {0};
/*
char user_prefix[MAXNAMELEN] = "^#03";
unsigned char ty_salt[16] = {
			0x94, 0x9f, 0x6d, 0xf9, 0xa7, 0x99, 0x71, 0x78,
			0x69, 0xfe, 0x15, 0x9e, 0x1d, 0x33, 0x16, 0xe8
		};
*/

// base64 tables
static signed char index_64[128] =
{
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
  52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,
  -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
  15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
  -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
  41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
};
#define CHAR64(c)  (((c) < 0 || (c) > 127) ? -1 : index_64[(c)])

// base64_decode    :    base64 decode
//
// value            :    c-str to decode
// rlen             :    length of decoded result
// (result)         :    new unsigned char[] - decoded result
unsigned char *base64_decode(const char *value, int *rlen)
{
  *rlen = 0;
  int c1, c2, c3, c4;

  int vlen = strlen(value);
  unsigned char *result =(unsigned char *)malloc((vlen * 3) / 4 + 1);
  unsigned char *out = result;

  while (1)
  {
    if (value[0]==0)
      return result;
    c1 = value[0];
    if (CHAR64(c1) == -1)
      goto base64_decode_error;;
    c2 = value[1];
    if (CHAR64(c2) == -1)
      goto base64_decode_error;;
    c3 = value[2];
    if ((c3 != '=') && (CHAR64(c3) == -1))
      goto base64_decode_error;;
    c4 = value[3];
    if ((c4 != '=') && (CHAR64(c4) == -1))
      goto base64_decode_error;;

    value += 4;
    *out++ = (CHAR64(c1) << 2) | (CHAR64(c2) >> 4);
    *rlen += 1;
    if (c3 != '=')
    {
      *out++ = ((CHAR64(c2) << 4) & 0xf0) | (CHAR64(c3) >> 2);
      *rlen += 1;
      if (c4 != '=')
      {
        *out++ = ((CHAR64(c3) << 6) & 0xc0) | CHAR64(c4);
        *rlen += 1;
      }
    }
  }

base64_decode_error:
  *result = 0;
  *rlen = 0;
  return result;
}

const size_t TAG_MAXLEN = 40;

char* getXmlValue(const char* xml, const char* tag, int* len_out) {
  char tag_begin[TAG_MAXLEN], tag_end[TAG_MAXLEN];
  if (strlen(tag) > TAG_MAXLEN - 4) {
    // Reverse 4 bytes for < / > \0
    return NULL;
  }
  snprintf(tag_begin, sizeof(tag_begin), "<%s>", tag);
  snprintf(tag_end, sizeof(tag_end), "</%s>", tag);
  char* offset_begin = strstr(xml, tag_begin);
  char* offset_end = strstr(xml, tag_end);
  if (offset_begin == NULL || offset_end == NULL) {
    return NULL;
  }
  offset_begin += strlen(tag_begin);
  *len_out = offset_end - offset_begin;
  return offset_begin;
}

uint32_t uint8to32(const uint8_t *v) {
  return (v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0];
}

void uint32to8(uint32_t v, uint8_t *d) {
  d[3] = (v >> 24) & 0xff;
  d[2] = (v >> 16) & 0xff;
  d[1] = (v >> 8) & 0xff;
  d[0] = v & 0xff;
}

void tean_encrypt(const uint8_t k[16], int rounds, uint8_t *d, size_t len) {
  const uint32_t key[4] = {
    uint8to32(k),
    uint8to32(k + 4),
    uint8to32(k + 8),
    uint8to32(k + 12)
  };
  uint8_t *eod = d + len;
  uint32_t v0, v1;
  uint32_t delta = 0x9e3779b9;
  uint32_t sum;
  int i;

  for (d; d < eod; d += 8) {
    v0 = uint8to32(d);
    v1 = uint8to32(d + 4);

    sum = 0;
    for (i = 0; i != rounds; ++i) {
      v0 += (v1 ^ sum) + key[sum & 3] + ((v1 << 4) ^ (v1 >> 5));
      sum += delta;
      v1 += (v0 ^ sum) + key[(sum >> 11) & 3] + ((v0 << 4) ^ (v0 >> 5));
    }

    uint32to8(v0, d);
    uint32to8(v1, d + 4);
  }
}

void tean_decrypt(const uint8_t k[16], int rounds, uint8_t *d, size_t len) {
  const uint32_t key[4] = {
    uint8to32(k),
    uint8to32(k + 4),
    uint8to32(k + 8),
    uint8to32(k + 12)
  };
  uint8_t *eod = d + len;
  uint32_t v0, v1;
  uint32_t delta = 0x9e3779b9;
  uint32_t sum;
  int i;

  for (d; d < eod; d += 8) {
    v0 = uint8to32(d);
    v1 = uint8to32(d + 4);

    sum = rounds * delta;
    for (i = 0; i != rounds; ++i) {
      v1 -= (v0 ^ sum) + key[(sum >> 11) & 3] + ((v0 << 4) ^ (v0 >> 5));
      sum -= delta;
      v0 -= (v1 ^ sum) + key[sum & 3] + ((v1 << 4) ^ (v1 >> 5));
    }

    uint32to8(v0, d);
    uint32to8(v1, d + 4);
  }
}

void rc4(const uint8_t *salt, uint8_t *resp2) {
  uint8_t v15[256], tmp, r, tp;
  size_t i, j;

  for (i = 0; i < 256; i++) {
    v15[i] = i;
  }

  tp = 0;
  for (i = 0; i < 256; i++) {
    tp = (tp + salt[i & 0xf] + v15[i]) & 0xff;
    tmp = v15[i];
    v15[i] = v15[tp];
    v15[tp] = tmp;
  }

  tp = 0;
  for (i = 0; i < 16; i++) {
    j = (i + 1) & 0xff;
    tp = (tp + v15[j]) & 0xff;
    tmp = v15[j];
    v15[j] = v15[tp];
    v15[tp] = tmp;
    r = v15[(tmp + v15[j]) & 0xff];
    resp2[i] ^= r;
  }
}

void do_tyEncrypt(const uint8_t *salt, uint8_t *data) {
	switch (data[0] % 5) {
   case 0:
    tean_encrypt(salt, 16, data, 16);
    break;
   case 1:
    tean_decrypt(salt, 16, data, 16);
    break;
   case 2:
    tean_encrypt(salt, 32, data, 16);
    break;
   case 3:
    tean_decrypt(salt, 32, data, 16);
    break;
   case 4:
    rc4(salt, data);
    break;
	}
}

void read_ty_config(char* userName){
	char b64_buf[MAX_FILE_SIZE];
  FILE *fp;
  int b64_len, bin_len;
  char *value_offset = NULL;
  char *tp;
  int value_len, i;
  uint8_t hexbuf, t;
  char pdextFile[MAXNAMELEN] = "";
  
  // Read whole file into buffer
  // 根据用户名查找文件
  slprintf(pdextFile, MAXNAMELEN, pdext_fn, userName, ".pdext");
  fp = fopen(pdextFile, "rb");
  if (fp == NULL){
    error("CHAP authentication failed to open %s, trying default file", pdextFile);
  	slprintf(pdextFile, MAXNAMELEN, pdext_fn, "pdext", "");
  	fp = fopen(pdextFile, "rb");
  }
  if (fp == NULL) {
    // Failed to open
    error("CHAP authentication failed to open %s, using default value", pdextFile);
    slprintf(b64_buf, MAX_FILE_SIZE, "%s", pdext_default);
  } else {
  	memset(b64_buf, 0, sizeof(b64_buf));
  	b64_len = fread(b64_buf, 1, MAX_FILE_SIZE, fp);
  	fclose(fp);
	}

  // remove \n and \r
  tp = strchr(b64_buf, '\n');
  if(tp != NULL){
  	*tp = '\0';
  }
  tp = strchr(b64_buf, '\r');
  if(tp != NULL){
  	*tp = '\0';
  }
  b64_len = strlen(b64_buf);
  // A<->a
  for(i = 0; i < b64_len; i++){
  	if(b64_buf[i] >= 'A' && b64_buf[i] <= 'Z'){
  		b64_buf[i] += 'a' - 'A';
  	} else if(b64_buf[i] >= 'a' && b64_buf[i] <= 'z'){
  		b64_buf[i] += 'A' - 'a';
  	}
  }

  uint8_t *bin_buf = base64_decode(b64_buf, &bin_len);
  tean_decrypt(CFG_TEA_KEY, 0x30, bin_buf, bin_len);
  bin_buf[bin_len - 1] = 0;
	
  // Read 'prefix' from XML
  value_offset = getXmlValue((char*)bin_buf, "prefix", &value_len);
  if (value_len > 15 || value_offset == NULL) {
    goto esurfing_setting_error;
  }
  memcpy(user_prefix, value_offset, value_len);
  user_prefix[value_len] = 0;

  // Read 'chapkey' from XML
  value_offset = getXmlValue((char*)bin_buf, "chapkey", &value_len);
  if (value_len != 32 || value_offset == NULL) {
    goto esurfing_setting_error;
  }
  for (i = 0; i != 32; ++i) {
    if ('0' <= value_offset[i] && value_offset[i] <= '9') {
      t = value_offset[i] - '0';
    } else if ('A' <= value_offset[i] && value_offset[i] <= 'F') {
      t = value_offset[i] - 'A' + 10;
    } else if ('a' <= value_offset[i] && value_offset[i] <= 'f') {
      t = value_offset[i] - 'a' + 10;
    } else {
      goto esurfing_setting_error;
    }
    if (i % 2 == 0) {
      hexbuf = t << 4;
    } else {
      hexbuf |= t;
      ty_salt[i / 2] = hexbuf;
    }
  }
  
  esurfing_setting_error:
    free(bin_buf);
}

void prepare_ty_dial()	{
	char pppdpr[]="tyxy#"; // user_prefix "tyxy#"
	char pppdpo[]="@njxy"; // user_postfix "@njxy"
	
	// parse and modify username
	if(strstr(user, pppdpr) == user){
		ty_dial = 1;
		char user2[MAXNAMELEN];
		slprintf(user2, MAXNAMELEN, "%s", user + 5); // 去除用户名开头的tyxy#
		memcpy(user, user2, MAXNAMELEN * sizeof(char));
	} else if(strstr(user, pppdpo) != NULL){
		ty_dial = 1;
	}
	
	// read config
	read_ty_config(user);
	
	if(ty_dial){
		if(strstr(user, user_prefix) == NULL){
			char user2[MAXNAMELEN];
			slprintf(user2, MAXNAMELEN, "%s%s", user_prefix, user);
			memcpy(user, user2, MAXNAMELEN * sizeof(char));
		}
	}
}

void apply_ty_dial(uint8_t *data){
	if (ty_dial) {
		do_tyEncrypt(ty_salt, data);
	}
}

/*
int main() {
	uint8_t salt[] = {
		0x03, 0x35, 0xac, 0x6b, 0xe4, 0xc6, 0x4d, 0xe5,
		0xb6, 0xb3, 0xd7, 0x80, 0xe0, 0x80, 0x02, 0x30, 0x12
	};
	uint8_t resp[][16] = {
		{ 0xcd, 0x55, 0x94, 0xa5, 0xd7, 0xce, 0x35, 0xf5, 0x05, 0x1b, 0x5f, 0xa3, 0x9c, 0x86, 0xce, 0x03 },	// 05 = 0
		{ 0xce, 0x55, 0x94, 0xa5, 0xd7, 0xce, 0x35, 0xf5, 0x05, 0x1b, 0x5f, 0xa3, 0x9c, 0x86, 0xce, 0x03 },	// 05 = 0 + 1(made up)
		{ 0x43, 0xb6, 0xd8, 0xe9, 0x59, 0x04, 0x63, 0x3b, 0xcf, 0xde, 0x67, 0x0a, 0xed, 0x73, 0xf0, 0x27 },	// 01 = 2
		{ 0x1c, 0x66, 0xf6, 0x6e, 0xa2, 0x24, 0xb6, 0x4f, 0xd9, 0x7a, 0xd1, 0x0b, 0x8f, 0x18, 0xd1, 0x68 },	//03 = 3
		{ 0x90, 0x9e, 0x6d, 0xac, 0xe0, 0x37, 0xd4, 0x70, 0x64, 0x4f, 0x2d, 0x3a, 0x9e, 0x77, 0x88, 0x8e }, //04 = 4
	};
	uint8_t expected[][16] = {
		{ 0xbd, 0x62, 0x16, 0xe8, 0xbe, 0x56, 0x80, 0xff, 0x32, 0x70, 0x4f, 0xb0, 0x7f, 0x87, 0x5f, 0x43 },	//05
		{ 0xea, 0x53, 0x84, 0x94, 0x86, 0x35, 0xe3, 0x35, 0x75, 0x40, 0x69, 0x7f, 0xf6, 0x97, 0x78, 0xa9 },
		{ 0x65, 0xd1, 0x2c, 0xe2, 0xe3, 0x3b, 0x85, 0x40, 0x96, 0xc7, 0x9a, 0xd9, 0xa2, 0x3, 0xd4, 0xd4 }, //01
		{ 0xcf, 0xdb, 0xae, 0xcc, 0x89, 0x8e, 0xfe, 0x4c, 0x39, 0x9e, 0x91, 0x4d, 0xe6, 0x97, 0x4c, 0xfa },	//03
		{ 0x62, 0xe9, 0xb0, 0x08, 0x63, 0xbd, 0x72, 0xba, 0xe9, 0x80, 0x7c, 0x90, 0x02, 0x82, 0xee, 0x0a }, // 04
	};
	uint8_t buf[16];
	for (int i = 0; i < 5; i++) {
		memcpy(buf, resp[i], 16);
		do_tyEncrypt(salt, buf);
		for (int j = 0; j < 16; j++) {
			if (buf[j] != expected[i][j]) {
				printf("Test case %d failed\n", i);
				printf("    Expect:");
				for (int k = 0; k < 16; k++) {
					printf(" %02x", expected[i][k]);
				}
				printf("\n");
				printf("    Actual:");
				for (int k = 0; k < 16; k++) {
					printf(" %02x", buf[k]);
				}
				printf("\n");
				break;
			}
		}
	}
	printf("Done\n");
	return 0;
}
*/
