/* share table
 * Antoine Rousseau 2020
 */

#include "m_pd.h"
#include "g_canvas.h"
#include "hextab.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

static t_class *sharetable_class;

typedef struct {
	t_object x_obj;
} t_sharetable;

typedef struct t_shared_item_struct {
	t_symbol *sym;
	struct t_shared_item_struct *next;
} t_shared_item;

#ifndef PERTHREAD
#define PERTHREAD
#endif

static PERTHREAD t_shared_item *shared_list = NULL;

static t_shared_item *shared_list_find_in(t_shared_item *i, t_symbol *s)
{
	if(i == NULL || i->sym == s) return i;
	else return shared_list_find_in(i->next, s);
}

static t_shared_item *shared_list_find(t_symbol *s)
{
	return shared_list_find_in(shared_list, s);
}

static void shared_list_add(t_symbol *s)
{
	t_shared_item *tmp = shared_list;
	
	if(shared_list_find(s)) return;
	shared_list = getbytes(sizeof(t_shared_item));
	shared_list->sym = s;
	shared_list->next = tmp;
}

static void shared_list_remove_in(t_shared_item *i, t_symbol *s)
{
	if(!i) return;
	if(!i->next) return;
	if(i->next->sym == s) {
		t_shared_item *tmp = i->next;
		i->next = i->next->next;
		freebytes(tmp, sizeof(t_shared_item));
	} else shared_list_remove_in(i->next, s);
}

static void shared_list_remove(t_symbol *s)
{
	if(!shared_list) return;
	if(shared_list->sym == s) {
		t_shared_item *tmp = shared_list;
		shared_list = tmp->next;
		freebytes(tmp, sizeof(t_shared_item));
	} else shared_list_remove_in(shared_list, s);
}

void name_compress(const char *in, int inlen, char *out, int outlen);

static void sharetable_remove(t_sharetable *x, t_symbol *name)
{
	char buf[MAXPDSTRING];

	name_compress(name->s_name, strlen(name->s_name), buf, sizeof(buf));
	
	if (shm_unlink(buf) == -1) {
		//pd_error(x, "sharetable_remove: %s", strerror(errno));
	}
}

static char* sharetable_doshare(t_sharetable *x, t_symbol* name, const int bytes)
{
	struct stat sb;
	char *p;
	int fd;
	char buf[MAXPDSTRING];

	name_compress(name->s_name, strlen(name->s_name), buf, sizeof(buf));

	//post("sharetable chars:%d name:%s", strlen(buf), buf);
	//post("sharetable chars:%d name:%s orig:%s", strlen(buf), buf, name->s_name);

	fd = shm_open(buf,  O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

	if (fd == -1) {
		pd_error(x, "shm_open: %s", strerror(errno));
		return NULL;
	}

	if (fstat(fd, &sb)==-1){
		pd_error(x, "fstat: %s", strerror(errno));
		return NULL;
	}

	if (ftruncate(fd, bytes) == -1) {
		pd_error(x, "ftruncate: %s", strerror(errno));
		return NULL;
	}

	p = mmap(0, bytes, PROT_WRITE, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED){
		pd_error(x, "mmap: %s", strerror(errno));
		return NULL;
	}

	if (close(fd)==-1) {
		pd_error(x, "close: %s", strerror(errno));
	}
	return p;
}

static void sharetable_sharehex(t_sharetable *x, t_symbol *hexsym, t_symbol *sharesym)
{
	t_hextab *h;
	short *hextab = 0;
	int nbsamples;
	char* shared;

	if (!(h = (t_hextab *)pd_findbyclass(hexsym, hextab_class)))
		pd_error(x, "sharetable: %s: no such array", hexsym->s_name);
	else if (!hextab_getarray(h, &nbsamples, &hextab))
		pd_error(x, "%s: bad template for sharetable", hexsym->s_name);

	if(!hextab) return;
	
	if(sharesym == gensym("UNSHARE")) {
		if(shared_list_find(hexsym)) {
			if(munmap(h->hextab, nbsamples * 2) == -1)
				pd_error(x, "munmap: %s", strerror(errno));
			shared_list_remove(hexsym);
			h->hextab = getbytes(nbsamples * 2); // restore hextab memory
		}
		return;
	}
	
	if(sharesym == &s_)  sharesym = hexsym;

	shared = sharetable_doshare(x, sharesym, nbsamples * 2);
	if(shared) {
		//if(munmap(h->hextab, nbsamples * 2) == -1)
		if(shared_list_find(hexsym)) {
			if(munmap(h->hextab, nbsamples * 2) == -1)
				pd_error(x, "munmap: %s", strerror(errno));		
		} else {
			freebytes(h->hextab, nbsamples * 2);
		}
		h->hextab = (short*)shared;
		shared_list_add(hexsym);
	}
}

static void sharetable_sharetable(t_sharetable *x, t_symbol *tablesym, t_symbol *sharesym)
{
	t_garray *a;
	t_word *tab = 0;
	int nbsamples;
	char* shared;
	char** tabadr;

	if (!(a = (t_garray *)pd_findbyclass(tablesym, garray_class)))
		pd_error(x, "sharetable: %s: no such array", tablesym->s_name);
	else if (!garray_getfloatwords(a, &nbsamples, &tab))
		pd_error(x, "%s: bad template for sharetable", tablesym->s_name);

	if(!tab) return;
	tabadr = &garray_getarray(a)->a_vec;
	
	if(sharesym == gensym("UNSHARE")) {
		if(shared_list_find(tablesym)) {
			if(munmap(*tabadr, nbsamples * sizeof(t_word)) == -1)
				pd_error(x, "munmap: %s", strerror(errno));
			shared_list_remove(tablesym);
			*tabadr = getbytes(nbsamples * sizeof(t_word)); // restore hextab memory
		}
		/*if(munmap(*tabadr, nbsamples * 2) == -1) {
			// was not mapped
		} else {
			*tabadr = getbytes(nbsamples * sizeof(t_word)); // restore table memory
		}*/
		return;
	}
	
	if(sharesym == &s_)  sharesym = tablesym;

	shared = sharetable_doshare(x, sharesym, nbsamples * sizeof(t_word));
	if(shared) {
		/*fprintf(stderr, "munmap: %p %p %d ...", *tabadr, tabadr, nbsamples);
		if(*tabadr && nbsamples && munmap(*tabadr, nbsamples * sizeof(t_word)) == -1) {
			perror("munmap");
			//freebytes(*tabadr, nbsamples * sizeof(t_word));
		}*/
		if(shared_list_find(tablesym)) {
			if(munmap(*tabadr, nbsamples * sizeof(t_word)) == -1)
				pd_error(x, "munmap: %s", strerror(errno));		
		} else {
			freebytes(*tabadr, nbsamples * sizeof(t_word));
		}
		*tabadr = shared;
		shared_list_add(tablesym);
	}
}

void *sharetable_new()
{
	t_sharetable *hex = (t_sharetable *)pd_new(sharetable_class);  
	return(hex);
}

void sharetable_setup(void)
{
	sharetable_class = class_new(gensym("sharetable"),
		(t_newmethod)sharetable_new,
		0, sizeof(t_sharetable),
		0, 0);

	class_addmethod(sharetable_class, (t_method)sharetable_sharehex, gensym("sharehex"), A_SYMBOL, A_DEFSYM, 0);
	class_addmethod(sharetable_class, (t_method)sharetable_sharetable, gensym("sharetable"), A_SYMBOL, A_DEFSYM, 0);
	class_addmethod(sharetable_class, (t_method)sharetable_remove, gensym("remove"), A_SYMBOL, 0);
}

#include "crc64.h"
uint64_t crc64(const void* data, size_t nbytes)
{
    uint64_t crc = 0;
    for (uint64_t i = 0; i < nbytes; ++i) {
        uint8_t k = ((uint8_t*)data)[i];
        crc = __crc64_table[(uint8_t)(crc) ^ k] ^ (crc >> 8);
    }
    return crc;
}

void name_compress(const char *in, int inlen, char *out, int outlen)
{
	snprintf(out, outlen, "/%d_%lx", inlen, crc64(in, strlen(in)));
}

