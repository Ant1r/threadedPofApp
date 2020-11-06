/* hextab minimal 16bit library
 * Ed Kelly 2014
 * Matt Davey 2018
 */
#include "m_pd.h"

#define UNITBIT32 1572864.  /* 3*2^19; bit 32 has place value 1 */

#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__) \
|| defined(__OpenBSD__)
#include <machine/endian.h>
#endif

#if defined(__linux__) || defined(__CYGWIN__) || defined(__GNU__) || \
defined(ANDROID)
#include <endian.h>
#endif

#ifdef __MINGW32__
#include <sys/param.h>
#include <sys/windows.h>
#endif

#ifdef _MSC_VER
/* _MSVC lacks BYTE_ORDER and LITTLE_ENDIAN */
#define LITTLE_ENDIAN 0x0001
#define BYTE_ORDER LITTLE_ENDIAN
#include <windows.h>
#endif

#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN)
#error No byte order defined
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
# define HIOFFSET 1
# define LOWOFFSET 0
#else
# define HIOFFSET 0    /* word offset to find MSB */
# define LOWOFFSET 1    /* word offset to find LSB */

#endif

#include <string.h>

extern t_class *hextab_class;

typedef struct _hextab
{
    t_object x_obj;
    
    t_symbol *tabName;
    short *hextab;
    int length;
    int startSize;
    int sourceLength;
    
    int usedindsp;
    
    t_outlet *loaded;
} t_hextab;

EXTERN void hextab_usedindsp(t_hextab *hex);
EXTERN int hextab_getarray(t_hextab *hex, int *length, short **tab);
EXTERN void hextab_resize(t_hextab *hex, t_float newlength);
