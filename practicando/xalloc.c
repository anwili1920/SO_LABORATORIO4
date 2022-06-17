
/*
 * ssoo/xalloc.98/xalloc.c
 *
 * CONTENIDO INICIAL:
 *	Codigo correspondiente a la Seccion 8.7 del libro:
 *	"The C Programing Language", de B. Kernigham y D. Ritchie.
 *
 * En este fichero se incluiran las rutinas pedidas 
 *
 */

#include <unistd.h>
#include "xalloc.h"

/*
 * Definicion de la cabecera para huecos y bloques. 
 * La union con un campo de tipo Align fuerza que el tama~no
 * de la cabecera sea multiplo del tama~no de este tipo.
 */
typedef long Align;    /* for alignment to long boundary */

union header {   /* block header: */
	struct {
		union header *ptr;  /* next block if on free list */
		size_t size;     /* size of this block */
	} s;
	Align x;             /* force alignment of blocks */
};

typedef union header Header;

/*
 * La lista de huecos esta ordenada por direcciones y es circular.
 * base es el "falso" hueco de tama~no cero que asegura que la lista
 * nunca esta vacia 
 */
static Header base;   /* empty list to get started */

/*
 * freep apuntara al hueco situado en la lista antes del hueco
 * por el que comenzara la busqueda.
 * Necesario para implementar la estrategia next-fit
 */
static Header  *freep = NULL;  /* start of the free list */


#define NALLOC 1024

/*
 * morecore: ask system for more memory 
 *
 * Esta funcion se llama desde xmalloc cuando no hay espacio.
 * Aumenta el tama~no de la zona de datos como minimo en NALLOC*sizeof(Header)
 * y a~nade esta nueva zona a la lista de huecos usando xfree.
 *
 */

static Header *morecore(size_t nu)
{
	char *cp;
	Header *up;

	if (nu < NALLOC){
		//nu = NALLOC; //originalmente estaba en unidades cabecera
		// NALLOC*sizeof(Header) : cantidad de bytes
		nu= ((NALLOC*sizeof(Header))+sizeof(Align)-1)/sizeof(Align);
	}	
		
	//cp= sbrk(nu * sizeof(Header)); //originalmente estaba en unidades cabecera
	cp= sbrk(nu * sizeof(Align));
	if (cp == (char *) -1) /* no space at all */
		return NULL;
	up = (Header *) cp; // el casteo es header porque va a ocupar un bloque de tamaño header
	up ->s.size = nu; // ya se guarda en unidades align
	//xfree((void *)(up+1));//originalmente estaba en unidades cabecera
	size_t head = (sizeof(Header)+sizeof(Align)-1)/sizeof(Align); // una cabeza en unidades Align
	xfree((void *)(up+head));
	return freep;
}

/* xmalloc: general-purpose storage allocator */
void *xmalloc (size_t nbytes)
{
	Header  *p, *prevp;
	size_t nunits,tam1HeadUAlign;

	/* 
	   Calcula cuanto ocupara la peticion medido en tama~nos de
	   cabecera (incluyendo la propia cabecera). 
	   El termino "sizeof(Align)-1" provoca un redondeo por exceso.
	   El termino "+ tamHeadUAlign" es para incluir la propia cabecera.
	*/
	//nunits = (nbytes+sizeof(Header)-1)/sizeof(Header) + 1; //original en unidades cabecera
	  tam1HeadUAlign=(sizeof(Header)+sizeof(Align)-1)/sizeof(Align);
	  nunits = (nbytes+sizeof(Align)-1)/sizeof(Align) + tam1HeadUAlign;

	/* En la primera llamada se construye una lista de huecos con un
	   unico elemento de tama~no cero (base) que se apunta a si mismo */
	if (( prevp = freep) == NULL ) { /* no free list yet */
		base.s.ptr = freep = prevp = & base; // esto se cumple porque freep es una variable "static" , por eso solamente serà NULL en la primera llamada
		base.s.size = 0;
	}

	/*
	   Recorre la lista circular de huecos, empezando por el siguiente al
	   que apunta freep, hasta que encuentra uno que satisface la peticion
	   o da toda una vuelta a la lista (no hay espacio suficiente)
	*/
	for (p= prevp->s.ptr; ; prevp = p, p = p->s.ptr) {
		if (p->s.size >= nunits) {  /* big enough */
			if (p->s.size == nunits)  /* exactly */
				prevp->s.ptr = p->s.ptr;
			else {  /* allocate tail end */
				p->s.size -= nunits;
				//p+= p->s.size; // aqui mueve el inicio del puntero size bytes siguientes en unidades cabecera
				
				p+= (p->s.size/tam1HeadUAlign);// lo transformo a unidades cabeza
				p->s.size = nunits;//le asigna su nuevo tamaño
			}
			freep = prevp; /* estrategia next-fit */
			return (void *)(p+1); /* se modifica ??? devuelve un puntero a la
						 zona de datos del bloque */
		}
		/* Si ha dado toda la vuelta pide mas memoria y vuelve
		   a empezar */
		if (p == freep) /* wrapped around free list */
			if ((p = morecore(nunits)) == NULL)
				return NULL;  /* none left */
	}
}


/* xfree: put block ap in the free list */
void xfree(void *ap)
{
	Header *bp, *p;
	size_t tam1HeadUAlign=(sizeof(Header)+sizeof(Align)-1)/sizeof(Align);
	//bp = (Header *)ap - 1;  /* point to block header */ // unidades cabecera 
	bp = (Header *)ap - tam1HeadUAlign; // unidades align
	/*
	   Bucle que recorre la lista de huecos para buscar el hueco
	   anterior al bloque que se va a liberar.
	   Del bucle se sale cuando se encuentran los dos huecos
	   de la lista (el apuntado por p y el apuntado por p->s.ptr)
	   entre los que se incluira el nuevo hueco (el apuntado por bp).
	   Hay dos casos:
		- La direccion del nuevo hueco es mayor (bp > p) o
		menor (bp < p->s.ptr) que la de ningun otro hueco de la
		lista (corresponde al break)
		- La direccion del nuevo hueco esta comprendida entre
		dos huecos de la lista (corresponde a la salida normal
		del for)
	*/

	for (p= freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
		if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
			break;  /* freed block at start or end of arena */ 


	/* Comprueba compactacion con hueco posterior */
	if (bp + (bp->s.size/tam1HeadUAlign) == p->s.ptr) {  /* join to upper nbr */
		bp->s.size += (p->s.ptr->s.size/tam1HeadUAlign);
		bp->s.ptr = p->s.ptr->s.ptr;
	} else
		 bp->s.ptr = p->s.ptr;

	/* Comprueba compactacion con hueco anterior */
	if (p + (p->s.size/tam1HeadUAlign) == bp) {         /* join to lower nbr */
		p->s.size += bp->s.size;
		p->s.ptr = bp->s.ptr;
	} else
		p->s.ptr = bp;

	freep = p; /* estrategia next-fit */
}

void *xrealloc(void * ptr, size_t size)
{
	return NULL;
}
