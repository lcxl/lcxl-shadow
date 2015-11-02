#ifndef _AES_EX_H_
#define _AES_EX_H_

#include "aesopt.h"

#define BLOCK_LEN   16

aes_rval aes_dec_mem(void *pin, int inlen, void *pout, int outlen, aes_ctx *ctx);

#endif