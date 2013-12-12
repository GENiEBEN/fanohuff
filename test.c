#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shannonfano.h"
#include "huffman.h"

int ReadWord32( FILE *f )
{
  unsigned char buf[4];
  fread( buf, 4, 1, f );
  return (((unsigned int)buf[0])<<24) +
         (((unsigned int)buf[1])<<16) +
         (((unsigned int)buf[2])<<8)  +
           (unsigned int)buf[3];
}
void WriteWord32( int x, FILE *f )
{
  fputc( (x>>24)&255, f );
  fputc( (x>>16)&255, f );
  fputc( (x>>8)&255, f );
  fputc( x&255, f );
}

long GetFileSize( FILE *f )
{
    long pos, size;

    pos = ftell( f );
    fseek( f, 0, SEEK_END );
    size = ftell( f );
    fseek( f, pos, SEEK_SET );

    return size;
}
void Help( char *prgname )
{
    printf( "Mod Utilizare: %s commanda [algoritm] fisier_sursa fisier_dest\n\n", prgname );
    printf( "Comenzi:\n" );
    printf( "  c       Compresare\n" );
    printf( "  d       Deompresare\n\n" );
    printf( "Algoritm:\n" );
    printf( "  sf      Shannon-Fano\n" );
    printf( "  huff    Huffman\n" );
    }

int main( int argc, char **argv )
{
    FILE *f;
    unsigned char *in, *out, command, algo=0;
    unsigned int  insize, outsize=0, *work;
    char *inname, *outname;

    if( argc < 4 )
    {
        Help( argv[ 0 ] );
        return 0;
    }

    command = argv[1][0];
    if( (command != 'c') && (command != 'd') )
    {
        Help( argv[ 0 ] );
        return 0;
    }

    if( argc == 5 && command == 'c' )
    {
        algo = 0;
        if( strcmp( argv[2], "huff" ) == 0 )    algo = 1;
        if( strcmp( argv[2], "sf" ) == 0 )      algo = 2;
        if( !algo )
        {
            Help( argv[ 0 ] );
            return 0;
        }
        inname  = argv[ 3 ];
        outname = argv[ 4 ];
    }
    else if( argc == 4 && command == 'd' )
    {
        inname  = argv[ 2 ];
        outname = argv[ 3 ];
    }
    else
    {
        Help( argv[ 0 ] );
        return 0;
    }

    f = fopen( inname, "rb" );
    if( !f )
    {
        printf( "Nu pot deschide fisierul sursa \"%s\".\n", inname );
        return 0;
    }

    insize = GetFileSize( f );

    if( command == 'd' )
    {
        algo = ReadWord32( f );  /* Dummy */
        algo = ReadWord32( f );
        outsize = ReadWord32( f );
        insize -= 12;
    }

     switch( algo )
    {
        case 1: printf( "Huffman " ); break;
        case 2: printf( "Shannon-Fano " ); break;
    }
    switch( command )
    {
        case 'c': printf( "compresare " ); break;
        case 'd': printf( "decompresare " ); break;
    }
    printf( "%s in %s...\n", inname, outname );

    printf( "Fisier sursa: %d bytes\n", insize );
    in = (unsigned char *) malloc( insize );
    if( !in )
    {
        printf( "Memorie Insuficienta\n" );
        fclose( f );
        return 0;
    }
    fread( in, insize, 1, f );
    fclose( f );

    if( command == 'd' )
    {
        printf( "Fisier compresat: %d bytes\n", outsize );
    }


    /* Open output file */
    f = fopen( outname, "wb" );
    if( !f )
    {
        printf( "Fisierul destinatie nu poate fi deschis \"%s\".\n", outname );
        free( in );
        return 0;
    }

   
    if( command == 'c' )
    {
        /* Scrie Header*/
        fwrite( "LIC1", 4, 1, f );
        WriteWord32( algo, f );
        WriteWord32( insize, f );
     
        outsize = (insize*104+50)/100 + 384;
    }

    out = malloc( outsize );
    if( !out )
    {
        printf( "Memorie insuficienta\n" );
        fclose( f );
        free( in );
        return 0;
    }

        if( command == 'c' )
    {
        switch( algo )
        {
            case 1:
                outsize = HUFF_Compresare( in, out, insize );
                break;
            case 2:
                outsize = SF_Compresare( in, out, insize );
                break;
        }
        printf( "Fisier rezultat: %d bytes (%.1f%%)\n", outsize,
                100*(float)outsize/(float)insize );
    }
    else
    {
        switch( algo )
        {
            case 1:
                HUFF_Decompresare( in, out, insize, outsize );
                break;
            case 2:
                SF_Decompresare( in, out, insize, outsize );
                break;
            default:
                printf( "Metoda de compresie necunoscuta: %d\n", algo );
                free( in );
                free( out );
                fclose( f );
                return 0;
        }
    }

    fwrite( out, outsize, 1, f );
    fclose( f );

    free( in );
    free( out );

    return 0;
}