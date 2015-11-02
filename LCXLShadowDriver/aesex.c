#include "aesex.h"

/*
aes_rval encfile(void *fin, size_t finlen, void **fout, size_t *outlen, aes_ctx *ctx)
{
	char            buf[BLOCK_LEN];
	if (fin==NULL||fout==NULL||*fout==NULL||outlen == NULL||ctx == NULL) {
		//Ê§°Ü
		return aes_bad;
	}
	

	char            buf[BLOCK_LEN], dbuf[2 * BLOCK_LEN];
	//fpos_t          flen;
	unsigned long   i, len, rlen;

	// set a random IV

	//fillrand(dbuf, BLOCK_LEN);

	// find the file length

	//fseek(fin, 0, SEEK_END);
	//fgetpos(fin, &flen); 
	//rlen = file_len(flen);
	rlen = finlen;
	// reset to start
	//fseek(fin, 0, SEEK_SET);

	if(rlen <= BLOCK_LEN)               
	{   // if the file length is less than or equal to 16 bytes

		// read the bytes of the file into the buffer and verify length
		//len = (unsigned long) fread(dbuf + BLOCK_LEN, 1, BLOCK_LEN, fin);
		//rlen -= len;        
		//if(rlen > 0) 
		//	return READ_ERROR;
		len = rlen;
		// pad the file bytes with zeroes
		for(i = len; i < BLOCK_LEN; ++i)
			dbuf[i + BLOCK_LEN] = 0;

		// xor the file bytes with the IV bytes
		for(i = 0; i < BLOCK_LEN; ++i)
			dbuf[i + BLOCK_LEN] ^= dbuf[i];

		// encrypt the top 16 bytes of the buffer
		aes_enc_blk(dbuf + BLOCK_LEN, dbuf + len, ctx);

		len += BLOCK_LEN;
		// write the IV and the encrypted file bytes
		if(fwrite(dbuf, 1, len, fout) != len)
			return WRITE_ERROR;
	}
	else
	{   // if the file length is more 16 bytes

		// write the IV
		if(fwrite(dbuf, 1, BLOCK_LEN, fout) != BLOCK_LEN)
			return WRITE_ERROR;

		// read the file a block at a time 
		while(rlen > 0 && !feof(fin))
		{  
			// read a block and reduce the remaining byte count
			len = (unsigned long)fread(buf, 1, BLOCK_LEN, fin);
			rlen -= len;

			// verify length of block 
			if(len != BLOCK_LEN) 
				return READ_ERROR;

			// do CBC chaining prior to encryption
			for(i = 0; i < BLOCK_LEN; ++i)
				buf[i] ^= dbuf[i];

			// encrypt the block
			aes_enc_blk(buf, dbuf, ctx);

			// if there is only one more block do ciphertext stealing
			if(rlen > 0 && rlen < BLOCK_LEN)
			{
				// move the previous ciphertext to top half of double buffer
				// since rlen bytes of this are output last
				for(i = 0; i < BLOCK_LEN; ++i)
					dbuf[i + BLOCK_LEN] = dbuf[i];

				// read last part of plaintext into bottom half of buffer
				if(fread(dbuf, 1, rlen, fin) != rlen)
					return READ_ERROR;

				// clear the remainder of the bottom half of buffer
				for(i = 0; i < BLOCK_LEN - rlen; ++i)
					dbuf[rlen + i] = 0;

				// do CBC chaining from previous ciphertext
				for(i = 0; i < BLOCK_LEN; ++i)
					dbuf[i] ^= dbuf[i + BLOCK_LEN];

				// encrypt the final block
				aes_enc_blk(dbuf, dbuf, ctx);

				// set the length of the final write
				len = rlen + BLOCK_LEN; rlen = 0;
			}

			// write the encrypted block
			if(fwrite(dbuf, 1, len, fout) != len)
				return WRITE_ERROR;
		}
	}
	if(rlen > 0)               
	{   // if the file length is less than or equal to 16 bytes

		// read the bytes of the file into the buffer and verify length
		//len = (unsigned long) fread(dbuf + BLOCK_LEN, 1, BLOCK_LEN, fin);
		//rlen -= len;        
		//if(rlen > 0) 
		//	return READ_ERROR;
		len = rlen;
		// pad the file bytes with zeroes
		for(i = len; i < BLOCK_LEN; ++i)
			dbuf[i + BLOCK_LEN] = 0;

		// xor the file bytes with the IV bytes
		for(i = 0; i < BLOCK_LEN; ++i)
			dbuf[i + BLOCK_LEN] ^= dbuf[i];

		// encrypt the top 16 bytes of the buffer
		aes_enc_blk(dbuf + BLOCK_LEN, dbuf + len, ctx);

		len += BLOCK_LEN;
		// write the IV and the encrypted file bytes
		if(fwrite(dbuf, 1, len, fout) != len)
			return WRITE_ERROR;
	}
	return 0;
}
*/

aes_rval aes_dec_mem(void *pin, int inlen, void *pout, int outlen, aes_ctx *ctx)
{
	//char            buf1[BLOCK_LEN];
	unsigned char *pinpos, *poutpos;
	
	if (inlen>outlen||inlen%(BLOCK_LEN)!=0) {
		return aes_bad;
	}
	pinpos = (unsigned char *)pin;
	poutpos = (unsigned char *)pout;
	while (inlen > 0) {
		if (aes_dec_blk(pinpos, poutpos, ctx) != aes_good) {
			return aes_bad;
		}
		pinpos+=BLOCK_LEN;
		poutpos+=BLOCK_LEN;
		inlen -= BLOCK_LEN;
	}
	return aes_good;
}
/*
int decfile(FILE *fin, FILE *fout, aes_ctx *ctx)
{
	char            buf1[BLOCK_LEN], buf2[BLOCK_LEN], dbuf[2 * BLOCK_LEN];
	char            *b1, *b2, *bt;
	fpos_t          flen;
	unsigned long   i, len, rlen;

	// find the file length

	fseek(fin, 0, SEEK_END);
	fgetpos(fin, &flen); 
	rlen = file_len(flen);
	// reset to start
	fseek(fin, 0, SEEK_SET);

	if(rlen <= 2 * BLOCK_LEN)
	{   // if the original file length is less than or equal to 16 bytes

		// read the bytes of the file and verify length
		len = (unsigned long)fread(dbuf, 1, 2 * BLOCK_LEN, fin);
		rlen -= len;
		if(rlen > 0)
			return READ_ERROR;

		// set the original file length
		len -= BLOCK_LEN;

		// decrypt from position len to position len + BLOCK_LEN
		aes_dec_blk(dbuf + len, dbuf + BLOCK_LEN, ctx);

		// undo CBC chaining
		for(i = 0; i < len; ++i)
			dbuf[i] ^= dbuf[i + BLOCK_LEN];

		// output decrypted bytes
		if(fwrite(dbuf, 1, len, fout) != len)
			return WRITE_ERROR; 
	}
	else
	{   // we need two input buffers because we have to keep the previous
		// ciphertext block - the pointers b1 and b2 are swapped once per
		// loop so that b2 points to new ciphertext block and b1 to the
		// last ciphertext block

		rlen -= BLOCK_LEN; b1 = buf1; b2 = buf2;

		// input the IV
		if(fread(b1, 1, BLOCK_LEN, fin) != BLOCK_LEN)
			return READ_ERROR;

		// read the encrypted file a block at a time
		while(rlen > 0 && !feof(fin))
		{
			// input a block and reduce the remaining byte count
			len = (unsigned long)fread(b2, 1, BLOCK_LEN, fin);
			rlen -= len;

			// verify the length of the read operation
			if(len != BLOCK_LEN)
				return READ_ERROR;

			// decrypt input buffer
			aes_dec_blk(b2, dbuf, ctx);

			// if there is only one more block do ciphertext stealing
			if(rlen > 0 && rlen < BLOCK_LEN)
			{
				// read last ciphertext block
				if(fread(b2, 1, rlen, fin) != rlen)
					return READ_ERROR;

				// append high part of last decrypted block
				for(i = rlen; i < BLOCK_LEN; ++i)
					b2[i] = dbuf[i];

				// decrypt last block of plaintext
				for(i = 0; i < rlen; ++i)
					dbuf[i + BLOCK_LEN] = dbuf[i] ^ b2[i];

				// decrypt last but one block of plaintext
				aes_dec_blk(b2, dbuf, ctx);

				// adjust length of last output block
				len = rlen + BLOCK_LEN; rlen = 0;
			}

			// unchain CBC using the last ciphertext block
			for(i = 0; i < BLOCK_LEN; ++i)
				dbuf[i] ^= b1[i];

			// write decrypted block
			if(fwrite(dbuf, 1, len, fout) != len)
				return WRITE_ERROR;

			// swap the buffer pointers
			bt = b1, b1 = b2, b2 = bt;
		}
	}

	return 0;
}
*/