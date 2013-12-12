typedef struct {
    unsigned char *BytePtr;
    unsigned int  BitPos;
} SF_secventa_t;

typedef struct {
    unsigned int Symbol;
    unsigned int Count;
    unsigned int Code;
    unsigned int Bits;
} SF_simbol_t;

typedef struct SF_NodArbore_struct SF_NodArbore_t;

struct SF_NodArbore_struct {
    SF_NodArbore_t *CopilA, *CopilB;
    int Symbol;
};

/* 2^(8+1)-1 = 511 */
#define  NR_MAX_NODURI 511

static void _SF_InitSecventa( SF_secventa_t *stream,
    unsigned char *buf )
{
    stream->BytePtr  = buf;
    stream->BitPos   = 0;
}


static unsigned int _SF_CitesteBit( SF_secventa_t *stream )
{
    unsigned int  x, bit;
    unsigned char *buf;

    buf = stream->BytePtr;
    bit = stream->BitPos;

    x = (*buf & (1<<(7-bit))) ? 1 : 0;
    bit = (bit+1) & 7;
    if( !bit )
    {
        ++ buf;
    }

    stream->BitPos = bit;
    stream->BytePtr = buf;

    return x;
}

static unsigned int _SF_Citeste8Biti( SF_secventa_t *stream )
{
    unsigned int  x, bit;
    unsigned char *buf;

    buf = stream->BytePtr;
    bit = stream->BitPos;

    x = (*buf << bit) | (buf[1] >> (8-bit));
    ++ buf;

    stream->BytePtr = buf;

    return x;
}

static void _SF_ScrieBiti( SF_secventa_t *stream, unsigned int x,
    unsigned int bits )
{
    unsigned int  bit, count;
    unsigned char *buf;
    unsigned int  mask;

    buf = stream->BytePtr;
    bit = stream->BitPos;

    mask = 1 << (bits-1);
    for( count = 0; count < bits; ++ count )
    {
        *buf = (*buf & (0xff^(1<<(7-bit)))) +
               ((x & mask ? 1 : 0) << (7-bit));
        x <<= 1;
        bit = (bit+1) & 7;
        if( !bit )
        {
            ++ buf;
        }
    }

    stream->BytePtr = buf;
    stream->BitPos  = bit;
}

static void _SF_Distributie( unsigned char *in, SF_simbol_t *sym,
    unsigned int size )
{
    int k, swaps;
    SF_simbol_t tmp;

       for( k = 0; k < 256; ++ k )
    {
        sym[k].Symbol = k;
        sym[k].Count  = 0;
        sym[k].Code   = 0;
        sym[k].Bits   = 0;
    }

    for( k = size; k; -- k )
    {
        sym[*in ++].Count ++;
    }

    do
    {
        swaps = 0;
        for( k = 0; k < 255; ++ k )
        {
            if( sym[k].Count < sym[k+1].Count )
            {
                tmp      = sym[k];
                sym[k]   = sym[k+1];
                sym[k+1] = tmp;
                swaps    = 1;
            }
        }
    }
    while( swaps );
}

static void _SF_GenArbore( SF_simbol_t *sym, SF_secventa_t *stream,
    unsigned int code, unsigned int bits, unsigned int first,
    unsigned int last )
{
    unsigned int k, size, size_a, size_b, last_a, first_b;

    if( first == last )
    {
        _SF_ScrieBiti( stream, 1, 1 );
        _SF_ScrieBiti( stream, sym[first].Symbol, 8 );

        sym[first].Code = code;
        sym[first].Bits = bits;
        return;
    }
    else
    {
        _SF_ScrieBiti( stream, 0, 1 );
    }

    /* Lungimea intervalului */
    size = 0;
    for( k = first; k <= last; ++ k )
    {
        size += sym[k].Count;
    }

    /* Lungimea ramurii A */
    size_a = 0;
    for( k = first; size_a < ((size+1)>>1) && k < last; ++ k )
    {
        size_a += sym[k].Count;
    }

    /* Nu este goala */
    if( size_a > 0 )
    {
        /* Continua*/
        _SF_ScrieBiti( stream, 1, 1 );

        /* Ramura A inlaturata din Distributie*/
        last_a  = k-1;

        _SF_GenArbore( sym, stream, (code<<1)+0, bits+1,
                               first, last_a );
    }
    else
    {
        /* Ramura este goala */
        _SF_ScrieBiti( stream, 0, 1 );
    }

    /* Lungimea Ramurii B */
    size_b = size - size_a;

    /* Goala? */
    if( size_b > 0 )
    {
        /* Continua */
        _SF_ScrieBiti( stream, 1, 1 );

        /* Inlaturata din Dist*/
        first_b = k;

        _SF_GenArbore( sym, stream, (code<<1)+1, bits+1,
                               first_b, last );
    }
    else
    {
        _SF_ScrieBiti( stream, 0, 1 );
    }
}

static SF_NodArbore_t * _SF_RecArbore( SF_NodArbore_t *nodes,
    SF_secventa_t *stream, unsigned int *nodenum )
{
    SF_NodArbore_t * this_node;

    this_node = &nodes[*nodenum];
    *nodenum = *nodenum + 1;

    this_node->Symbol = -1;
    this_node->CopilA = (SF_NodArbore_t *) 0;
    this_node->CopilB = (SF_NodArbore_t *) 0;

    if( _SF_CitesteBit( stream ) )
    {
        this_node->Symbol = _SF_Citeste8Biti( stream );

        return this_node;
    }

    if( _SF_CitesteBit( stream ) )
    {
        this_node->CopilA = _SF_RecArbore( nodes, stream, nodenum );
    }

    if( _SF_CitesteBit( stream ) )
    {
        this_node->CopilB = _SF_RecArbore( nodes, stream, nodenum );
    }

    return this_node;
}

int SF_Compresare( unsigned char *in, unsigned char *out,
    unsigned int insize )
{
    SF_simbol_t       sym[256], tmp;
    SF_secventa_t stream;
    unsigned int     k, total_bytes, swaps, symbol, last_symbol;

    if( insize < 1 ) return 0;

    _SF_InitSecventa( &stream, out );
    _SF_Distributie( in, sym, insize );

    /* Cate simboluri au fost folosite */
    for( last_symbol = 255; sym[last_symbol].Count == 0; -- last_symbol );
    if( last_symbol == 0 ) ++ last_symbol;

    _SF_GenArbore( sym, &stream, 0, 0, 0, last_symbol );

    /* Sorteaza distributia*/
    do
    {
        swaps = 0;
        for( k = 0; k < 255; ++ k )
        {
            if( sym[k].Symbol > sym[k+1].Symbol )
            {
                tmp      = sym[k];
                sym[k]   = sym[k+1];
                sym[k+1] = tmp;
                swaps    = 1;
            }
        }
    }
    while( swaps );

    /* Encode input stream */
    for( k = 0; k < insize; ++ k )
    {
        symbol = in[k];
        _SF_ScrieBiti( &stream, sym[symbol].Code,
                            sym[symbol].Bits );
    }

    total_bytes = (int)(stream.BytePtr - out);
    if( stream.BitPos > 0 )
    {
        ++ total_bytes;
    }

    return total_bytes;
}

void SF_Decompresare( unsigned char *in, unsigned char *out,
    unsigned int insize, unsigned int outsize )
{
    SF_NodArbore_t  nodes[ NR_MAX_NODURI], *root, *node;
    SF_secventa_t stream;
    unsigned int     k, node_count;
    unsigned char    *buf;

    if( insize < 1 ) return;

    _SF_InitSecventa( &stream, in );

    node_count = 0;
    root = _SF_RecArbore( nodes, &stream, &node_count );

    buf = out;
    for( k = 0; k < outsize; ++ k )
    {
        node = root;
        while( node->Symbol < 0 )
        {
          if( _SF_CitesteBit( &stream ) )
            node = node->CopilB;
          else
            node = node->CopilA;
        }

        *buf ++ = (unsigned char) node->Symbol;
    }
}
