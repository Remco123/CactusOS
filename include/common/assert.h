#ifndef __CACTUSOS__COMMON__ASSERT_H
#define __CACTUSOS__COMMON__ASSERT_H

void printf(char*);

void printAssert( char const * const strA, char const * const strB, char const * const strC)
{
    printf((char*)strA); printf((char*)strB); printf((char*)strC);
}
#define __symbol2value( x ) #x
#define __symbol2string( x ) __symbol2value( x )

#define assert( expression ) ( ( expression ) ? (void) 0 \
        : printAssert( "Assertion failed: " #expression \
                          ", function ", __func__, \
                          ", file " __FILE__ \
                          ", line " __symbol2string( __LINE__ ) \
                          ".\n" ) )

#endif