#ifndef _shannonfano_h_
#define _shannonfano_h_

#ifdef __cplusplus
extern "C" {
#endif

int SF_Compresare( unsigned char *in, unsigned char *out,
                 unsigned int insize );
void SF_Decompresare( unsigned char *in, unsigned char *out,
                    unsigned int insize, unsigned int outsize );


#ifdef __cplusplus
}
#endif

#endif
