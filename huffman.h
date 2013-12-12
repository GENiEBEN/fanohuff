#ifndef _huffman_h_
#define _huffman_h_

#ifdef __cplusplus
extern "C" {
#endif


int HUFF_Compresare( unsigned char *in, unsigned char *out,
                      unsigned int insize );
void HUFF_Decompresare( unsigned char *in, unsigned char *out,
                         unsigned int insize, unsigned int outsize );


#ifdef __cplusplus
}
#endif

#endif
