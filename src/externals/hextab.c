/* hextab: 16 bit sample table
 * Edward Kelly 2014
 
 * modified by Matt Davey 2018
 
 * based on g_array.c from the Pd source code
 */

#include "m_pd.h"
#include "hextab.h"

#include <stdlib.h>
#include <string.h>

t_class *hextab_class;

static void hextab_free(t_hextab *hex)
{
    if(hex->hextab)freebytes(hex->hextab, hex->length * sizeof(short));
    pd_unbind(&hex->x_obj.ob_pd, hex->tabName);
}

static void hextab_getinfo(t_hextab *hex)
{
    post("length = %d", hex->length);
    post("startsize = %d", hex->startSize);
    post("sourceLength = %d", hex->sourceLength);
    post("tablename = %s", hex->tabName->s_name);
}

static void hextab_clear(t_hextab *hex)
{
    int len = hex->length;
    short *copy = hex->hextab;
    while(len--)*copy++ = 0;
}

static void hextab_set(t_hextab *hex, t_float position, t_float value)
{
    int len = hex->length;
    int pos = position;
    if (pos >= 0 && pos < len)
    {
        float multiplier = 32760;
        float valueToBeCopied = value;
        if (valueToBeCopied < -1) valueToBeCopied = -1;
        else if (valueToBeCopied > 1) valueToBeCopied = 1;
        hex->hextab[pos] = valueToBeCopied * multiplier;
    }
}

void hextab_resize(t_hextab *hex, t_float newlength)
{
    int oldLength = hex->length;
    hex->length = newlength;
    
    hex->hextab = resizebytes(hex->hextab, oldLength * sizeof(short), hex->length * sizeof(short));
    
//    hextab_clear(hex);
    
    if(hex->usedindsp)canvas_update_dsp();
}

static void hextab_copy(t_hextab *hex, t_symbol *s, int argc, t_atom *argv)
{
    t_word *words = (0);
    t_garray *source = 0;
    short *copy;
    t_symbol *tabname;
    int n = 0;// i = 0;
    float multiplier = 32760;
    int actualLength;
    
    tabname = atom_getsymbol(argv);
    
    if((source = (t_garray *) pd_findbyclass (tabname, garray_class)))
    {
        if(garray_getfloatwords(source, &actualLength, &words))
        {
            if (argv[1].a_w.w_float >= 1 && argc > 1)
            {
                hex->sourceLength = argv[1].a_w.w_float;
            }
            else
            {
                hex->sourceLength = actualLength;
            }
            
            if (hex->sourceLength > hex->length)
            {
                hextab_resize(hex, hex->sourceLength);
            }
            
            n = hex->sourceLength;
            copy = hex->hextab;
            
            while(n--)
            {
                float valueToBeCopied = (*words++).w_float;
                if (valueToBeCopied < -1) valueToBeCopied = -1;
                else if (valueToBeCopied > 1) valueToBeCopied = 1;
                *copy++ = valueToBeCopied * multiplier;
            }
            if(hex->usedindsp)canvas_update_dsp();
            outlet_bang(hex->loaded);
        }
    }
}

static void hextab_resetsize(t_hextab *hex)
{
    hextab_resize(hex, hex->startSize);
}

static void hextab_size(t_hextab *hex, t_float newsize)
{
    hextab_resize(hex, newsize);
}

static void hextab_clear_section(t_hextab *hex, float startSample, float endSample)
{
    int len = hex->length;
    
    if (startSample >= 0 && startSample < endSample && endSample <= len)
    {
        short *copy = hex->hextab;
        for (int i = startSample; i <= endSample; i++)
        {
            *copy++ = 0;
        }
    }
}

void hextab_usedindsp(t_hextab *hex)
{
    hex->usedindsp = 1;
}

void *hextab_new(t_symbol *name, t_float f)
{
    t_hextab *hex = (t_hextab *)pd_new(hextab_class);
    hex->startSize = f;
    if (hex->startSize < 10)
    {
        hex->startSize = 10;
    }
    hex->length = hex->sourceLength = hex->startSize;
    int numBytes = hex->length * sizeof(short);
    hex->hextab = getbytes(numBytes);
    
    hex->tabName = name;
    pd_bind(&hex->x_obj.ob_pd, hex->tabName);
    
    hex->usedindsp = 0;
    
    hex->loaded = outlet_new(&hex->x_obj, &s_bang);
    
    return(hex);
}

int hextab_getarray(t_hextab *hex, int *length, short **tab)
{
    *tab = hex->hextab;
    *length = hex->length;
    return(1);
}

void hextab_setup(void)
{
    hextab_class=class_new(gensym("hextab"), (t_newmethod)hextab_new, (t_method)hextab_free, sizeof(t_hextab), 0, A_DEFSYM, A_DEFFLOAT, 0);
    
    //  post("hextab - minimal 16bit sample table");
    
    class_addmethod(hextab_class, (t_method)hextab_set, gensym("set"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(hextab_class, (t_method)hextab_copy, gensym("copy"), A_GIMME, A_GIMME, 0);
    class_addmethod(hextab_class, (t_method)hextab_getinfo, gensym("getinfo"), A_GIMME, 0);
    class_addmethod(hextab_class, (t_method)hextab_resetsize, gensym("resetsize"), A_GIMME, 0);
    class_addmethod(hextab_class, (t_method)hextab_size, gensym("size"), A_FLOAT, 0);
    class_addmethod(hextab_class, (t_method)hextab_clear_section, gensym("clear_section"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(hextab_class, (t_method)hextab_clear, gensym("clear"), A_GIMME, 0);
}

