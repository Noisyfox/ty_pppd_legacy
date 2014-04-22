#include <string.h>
#include <stdint.h>

inline uint32_t uint8to32(const uint8_t *v) {
	return (v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0];
}

void uint32to8(uint32_t v, uint8_t *d) {
	d[3] = (v >> 24) & 0xff;
	d[2] = (v >> 16) & 0xff;
	d[1] = (v >> 8) & 0xff;
	d[0] = v & 0xff;
}

void tea(const uint8_t *k, uint8_t *v, int rounds) {
	uint32_t delta = 0x9e3779b9,
		sum = delta * rounds,
		v0 = uint8to32(v),
		v1 = uint8to32(v + 4),
		key[] = { uint8to32(k), uint8to32(k + 4), uint8to32(k + 8), uint8to32(k + 12) };
	int i;
	if (rounds > 0) {
		sum = 0;
		for (i = 0; i != rounds; i++) {
			v0 += (v1 ^ sum) + key[sum & 3] + ((v1 << 4) ^ (v1 >> 5));
			sum += delta;
			v1 += (v0 ^ sum) + key[(sum >> 11) & 3] + ((v0 << 4) ^ (v0 >> 5));
		}
	}
	else {
		rounds = -rounds;
		sum = delta * rounds;
		for (i = 0; i != rounds; i++) {
			v1 -= (v0 ^ sum) + key[(sum >> 11) & 3] + ((v0 << 4) ^ (v0 >> 5));
			sum -= delta;
			v0 -= (v1 ^ sum) + key[sum & 3] + ((v1 << 4) ^ (v1 >> 5));
		}
	}

	uint32to8(v0, v);
	uint32to8(v1, v + 4);
}

void subn_1209C(const uint8_t *salt, uint8_t *resp2) {
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
		tea(salt, data, 16);
		tea(salt, data + 8, 16);
		break;
	case 1:
		tea(salt, data, -16);
		tea(salt, data + 8, -16);
		break;
	case 2:
		tea(salt, data, 32);
		tea(salt, data + 8, 32);
		break;
	case 3:
		tea(salt, data, -32);
		tea(salt, data + 8, -32);
		break;
	case 4:
		subn_1209C(salt, data);
		break;
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
