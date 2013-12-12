typedef struct {
    unsigned char *BytePtr;
    unsigned int  BitPos;
} huff_secventa_t;

typedef struct {
    int Symbol;
    unsigned int Count;
    unsigned int Code;
    unsigned int Bits;
} huff_sym_t;

typedef struct huff_codnod_struct huff_codnod_t;

struct huff_codnod_struct {
    huff_codnod_t *CopilA, *CopilB;
    int Count;
    int Symbol;
};

typedef struct huff_decodnod_struct huff_decodnod_t;

struct huff_decodnod_struct {
    huff_decodnod_t *CopilA, *CopilB;
    int Symbol;
};

/* 2^(8+1)-1 = 511 */
#define NR_MAX_NODURI 511

static void _HUFF_InitSecventa( huff_secventa_t *stream,
    unsigned char *buf )
{
  stream->BytePtr  = buf;
  stream->BitPos   = 0;
}

static unsigned int _HUFF_CitesteBit( huff_secventa_t *stream )
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

static unsigned int _HUFF_Citeste8Biti( huff_secventa_t *stream )
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

static void _HUFF_ScrieBiti( huff_secventa_t *stream, unsigned int x,
  unsigned int bits )
{
  unsigned int  bit, count;
  unsigned char *buf;
  unsigned int  mask;

  buf = stream->BytePtr;
  bit = stream->BitPos;

  /* Ataseaza bitii */
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

static void _HUFF_Distributie( unsigned char *in, huff_sym_t *sym,
  unsigned int size )
{
  int k;

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
}

static void _Huffman_StoreTree( huff_codnod_t *node, huff_sym_t *sym,
  huff_secventa_t *stream, unsigned int code, unsigned int bits )
{
  unsigned int sym_idx;

    if( node->Symbol >= 0 )
  {
    _HUFF_ScrieBiti( stream, 1, 1 );
    _HUFF_ScrieBiti( stream, node->Symbol, 8 );

    for( sym_idx = 0; sym_idx < 256; ++ sym_idx )
    {
      if( sym[sym_idx].Symbol == node->Symbol ) break;
    }

    sym[sym_idx].Code = code;
    sym[sym_idx].Bits = bits;
    return;
  }
  else
  {
    _HUFF_ScrieBiti( stream, 0, 1 );
  }

  _Huffman_StoreTree( node->CopilA, sym, stream, (code<<1)+0, bits+1 );
  _Huffman_StoreTree( node->CopilB, sym, stream, (code<<1)+1, bits+1 );
}

static void _HUFF_GenArbore( huff_sym_t *sym, huff_secventa_t *stream )
{
  huff_codnod_t nodes[NR_MAX_NODURI], *node_1, *node_2, *root;
  unsigned int k, num_symbols, nodes_left, next_idx;

  num_symbols = 0;
  for( k = 0; k < 256; ++ k )
  {
    if( sym[k].Count > 0 )
    {
      nodes[num_symbols].Symbol = sym[k].Symbol;
      nodes[num_symbols].Count = sym[k].Count;
      nodes[num_symbols].CopilA = (huff_codnod_t *) 0;
      nodes[num_symbols].CopilB = (huff_codnod_t *) 0;
      ++ num_symbols;
    }
  }

  root = (huff_codnod_t *) 0;
  nodes_left = num_symbols;
  next_idx = num_symbols;
  while( nodes_left > 1 )
  {
    /* Gaseste cele mai usoare noduri */
    node_1 = (huff_codnod_t *) 0;
    node_2 = (huff_codnod_t *) 0;
    for( k = 0; k < next_idx; ++ k )
    {
      if( nodes[k].Count > 0 )
      {
        if( !node_1 || (nodes[k].Count <= node_1->Count) )
        {
          node_2 = node_1;
          node_1 = &nodes[k];
        }
        else if( !node_2 || (nodes[k].Count <= node_2->Count) )
        {
          node_2 = &nodes[k];
        }
      }
    }

    /* Uneste-le */
    root = &nodes[next_idx];
    root->CopilA = node_1;
    root->CopilB = node_2;
    root->Count = node_1->Count + node_2->Count;
    root->Symbol = -1;
    node_1->Count = 0;
    node_2->Count = 0;
    ++ next_idx;
    -- nodes_left;
  }

  if( root )
  {
    _Huffman_StoreTree( root, sym, stream, 0, 0 );
  }
  else
  {
    root = &nodes[0];
    _Huffman_StoreTree( root, sym, stream, 0, 1 );
  }
}


static huff_decodnod_t * _HUFF_RecArbore( huff_decodnod_t *nodes,
  huff_secventa_t *stream, unsigned int *nodenum )
{
  huff_decodnod_t * this_node;

  /* Alege Nod*/
  this_node = &nodes[*nodenum];
  *nodenum = *nodenum + 1;

  this_node->Symbol = -1;
  this_node->CopilA = (huff_decodnod_t *) 0;
  this_node->CopilB = (huff_decodnod_t *) 0;

   if( _HUFF_CitesteBit( stream ) )
  {
    this_node->Symbol = _HUFF_Citeste8Biti( stream );

    return this_node;
  }

  this_node->CopilA = _HUFF_RecArbore( nodes, stream, nodenum );
  this_node->CopilB = _HUFF_RecArbore( nodes, stream, nodenum );

  return this_node;
}


int HUFF_Compresare( unsigned char *in, unsigned char *out,
  unsigned int insize )
{
  huff_sym_t       sym[256], tmp;
  huff_secventa_t stream;
  unsigned int     k, total_bytes, swaps, symbol;

  if( insize < 1 ) return 0;

  _HUFF_InitSecventa( &stream, out );
  _HUFF_Distributie( in, sym, insize );
  _HUFF_GenArbore( sym, &stream );
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

  /* codeaza secventa */
  for( k = 0; k < insize; ++ k )
  {
    symbol = in[k];
    _HUFF_ScrieBiti( &stream, sym[symbol].Code,
                        sym[symbol].Bits );
  }

  /* Calculeaza marimea rezultata */
  total_bytes = (int)(stream.BytePtr - out);
  if( stream.BitPos > 0 )
  {
    ++ total_bytes;
  }

  return total_bytes;
}

void HUFF_Decompresare( unsigned char *in, unsigned char *out,
  unsigned int insize, unsigned int outsize )
{
  huff_decodnod_t nodes[NR_MAX_NODURI], *root, *node;
  huff_secventa_t  stream;
  unsigned int      k, node_count;
  unsigned char     *buf;

  if( insize < 1 ) return;

  _HUFF_InitSecventa( &stream, in );

  /* Recuperare Arbore */
  node_count = 0;
  root = _HUFF_RecArbore( nodes, &stream, &node_count );

  /* Decodare secventa */
  buf = out;
  for( k = 0; k < outsize; ++ k )
  {
    /* Parcurge arborele */
    node = root;
    while( node->Symbol < 0 )
    {
      /* Urm. nod */
      if( _HUFF_CitesteBit( &stream ) )
        node = node->CopilB;
      else
        node = node->CopilA;
    }
    /* Gasit */
    *buf ++ = (unsigned char) node->Symbol;
  }
}