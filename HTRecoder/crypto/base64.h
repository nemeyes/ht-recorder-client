#ifndef HAVE_B64
#define HAVE_B64

#ifdef __cplusplus
extern "C" {
#endif

char *base64_encode(char *binStr, unsigned len);
int base64_decode(char *input, unsigned int length);

#ifdef __cplusplus
}
#endif


#endif /* HAVE_VSNPRINTF */

