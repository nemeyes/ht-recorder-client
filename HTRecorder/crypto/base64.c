#include <stdio.h>
#include <stdlib.h>

#define XX 127

static const char basis_64[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char index_64[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,XX,XX,XX,
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};
#define CHAR64(c)  (index_64[(unsigned char)(c)])

/*
 * Decode in-place the base64 data in 'input'.  Returns the length
 * of the decoded data, or -1 if there was an error.
 */
int base64_decode(char *input, unsigned int length)
{
    int len = 0;
	int in_len = 0;
    unsigned char *output = (unsigned char *)input;
    int c1, c2, c3, c4;

    while (in_len<length && *input) {
		c1 = *input++;
		if (CHAR64(c1) == XX){
			return -1;
		}
		c2 = *input++;
		if (CHAR64(c2) == XX){
			return -1;
		}
		c3 = *input++;
		if (c3 != '=' && CHAR64(c3) == XX){
			return -1; 
		}
		c4 = *input++;
		if (c4 != '=' && CHAR64(c4) == XX){
			return -1;
		}

		in_len += 4;

		*output++ = (CHAR64(c1) << 2) | (CHAR64(c2) >> 4);
		++len;
		if (c3 == '='){
			break;
		}
		*output++ = ((CHAR64(c2) << 4) & 0xf0) | (CHAR64(c3) >> 2);
		++len;
		if (c4 == '='){ 
			break;
		}
		*output++ = ((CHAR64(c3) << 6) & 0xc0) | CHAR64(c4);
		++len;
    }

    return len;
}
	
/*
 * Encode the given binary string of length 'len' and return Base64
 * in a char buffer.  It allocates the space for buffer.
 * caller must free the space.
 */
char *base64_encode(char *binStr, unsigned len)
{
    char * buf;
    int buflen = 0;
    int c1, c2, c3;
    int maxbuf;

#ifdef RUBBISH
    maxbuf = len*4/3 + 1;  /* size after expantion */
#endif
    maxbuf = len*2 + 20;  /* size after expantion */

    if((buf = malloc(maxbuf)) == NULL)
	return NULL;
    while (len) {
	
		c1 = (unsigned char)*binStr++;
		buf[buflen++] = basis_64[c1>>2];

		if (--len == 0){
			c2 = 0;
		}else{
			c2 = (unsigned char)*binStr++;
		}
		buf[buflen++] = basis_64[((c1 & 0x3)<< 4) | ((c2 & 0xF0) >> 4)];

		if (len == 0) {
			buf[buflen++] = '=';
			buf[buflen++] = '=';
			break;
		}

		if (--len == 0){
			c3 = 0;
		}else{
			c3 = (unsigned char)*binStr++;
		}

		buf[buflen++] = basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)];
		if (len == 0){
			buf[buflen++] = '=';
			break;
		}
		--len;
		buf[buflen++] = basis_64[c3 & 0x3F];
    }

    buf[buflen]=0;
    return buf;
}

