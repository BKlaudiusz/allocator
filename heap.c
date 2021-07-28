#include "heap.h"
//
// Created by root on 11/14/20.
//

// wskaźnik globalny na sterte

struct heap_t* heap= NULL;
#define ALIGNED 8
int align_size(int size) {
    return (size + ALIGNED - 1) & ~(ALIGNED - 1);
}
//inicjalizacja stery
int heap_setup(void)
{
    int minimalsize = (sizeof(struct heap_t) + 2 * sizeof(struct block_t) );
    void* memory = custom_sbrk(minimalsize);
    if(!memory)
        return -1;
    // inicjalizacja struktury
    heap =memory;
    heap->sizeheap = minimalsize;
    // inicjalizacja pol stery
    struct block_t* phead = (struct block_t*)((uint8_t*)heap + sizeof(struct heap_t)); // b. graniczny lewy
    struct block_t* ptail = (struct block_t*)((uint8_t*)phead + 1*sizeof(struct block_t)); // b. graniczny prawy
    // [HHHHHHHHpheadpptail]

    heap->phead = phead;
    heap->ptail = ptail;
    heap->heap_start = (uint8_t*)memory;

    phead->pnext = ptail;
    phead->pprev = NULL;
    ptail->pnext = NULL;
    ptail->pprev = phead;
    phead->size = 0; ptail->size = 0;
    phead->free = 0; ptail->free = 0;
    phead->usersize = 0; ptail->usersize = 0;
    phead->checksum = chcecksum((void *)phead,sizeof(struct  block_t));
    ptail->checksum = chcecksum((void *)ptail,sizeof(struct  block_t));
    heap->checksum = chcecksum((void *)heap,sizeof(struct  heap_t));

    return 0;
}
void heap_clean(void)
{
    if(heap)
    {

        custom_sbrk(-heap->sizeheap);
        heap = NULL;
    }
}
void* heap_malloc(size_t size)
{
    if(heap)
    {
        if(size ==0 || heap == NULL)
            return NULL;
        struct block_t* pnew = findfirstfreeblock(size);
        if(!pnew){
            // trzeba poprosic o wiecej pamieci
            void * newblock  = custom_sbrk(sizeof(struct  block_t)+ size+2*SIZEBLOCK_FIANCE);
            if(newblock!= (void *)-1){
                //dodanie ilosci danych pobranych od OS
                heap->sizeheap +=size + sizeof(struct  block_t)+2*SIZEBLOCK_FIANCE;
                //przesuniecie koncowego bloku
                struct  block_t* newtail = (struct  block_t*)((uint8_t *)newblock+size+2*SIZEBLOCK_FIANCE);
                newtail->free = 0;
                newtail->size = 0;
                newtail->pnext= NULL;
                newtail->usersize = 0;
                newtail->pprev = heap->ptail;
                newtail->checksum = chcecksum((uint8_t *)newtail,32);
                //zmiana size starego konnca na blok,
                heap->ptail->pnext = newtail;
                heap->ptail->size = size;
                heap->ptail->usersize = size;
                heap->ptail->checksum = chcecksum((uint8_t*)heap->ptail,32);
                heap->ptail = newtail;
                // ustawienie plotek
                *(uint8_t*)(newblock) = FENCE;
                *((uint8_t *)newblock + size+ SIZEBLOCK_FIANCE) = FENCE;
                heap->checksum = chcecksum((uint8_t*)heap,sizeof(struct  heap_t));
                return ((uint8_t *)newblock + SIZEBLOCK_FIANCE);
            }
            return NULL;
        }else
        {
            //dodanie plotek
            *((uint8_t *)pnew+sizeof(struct block_t)) = FENCE;
            *((uint8_t *)pnew+sizeof(struct block_t)+size+SIZEBLOCK_FIANCE) = FENCE;
            pnew->free = 0;
            pnew->usersize = size;
           // *(char*)((uint8_t *)pnew+sizeof(struct block_t)+size) = FENCE;
            //ustawic size jaki chce user a z reszta nie wiem co zrobic
            //suma kontrolna
            pnew->checksum = chcecksum((uint8_t*)pnew,32);
            //heap->checksum = chcecksum((uint8_t*)heap,32);
            return ((uint8_t *)pnew +sizeof(struct block_t) + SIZEBLOCK_FIANCE) ;
        }
    }
    return  NULL;
}
//liczenie sumy kontrolnej blokow
unsigned int chcecksum(const void * pointer,size_t size)
{
    unsigned int checksum = 0;
    size_t i ;
    for ( i = 0; i <  size - 2*sizeof(int); ++i)
    {
        checksum += *((uint8_t*)pointer + i);
    }
    for ( i = size - 1*sizeof(int); i < size; ++i)
    {
        checksum += *((uint8_t*)pointer + i);
    }
    return checksum;
}
void* heap_calloc(size_t number, size_t size)
{
    void* pointerToReturn = heap_malloc(number*size);
    if(pointerToReturn)
    {
        for(size_t i = 0;i<number*size;++i)
        {
            *((uint8_t *)pointerToReturn+i) = 0;
        }
    }
    return  pointerToReturn;
}


void* heap_realloc(void* memblock, size_t count)
{
    if(heap)
    {
        if(memblock == NULL)
        {
            return heap_malloc(count);
        }
        struct block_t* pcurrent;
        for (pcurrent = heap->phead; pcurrent != NULL; pcurrent = pcurrent->pnext) {

            if (((uint8_t *) pcurrent + sizeof(struct block_t) + SIZEBLOCK_FIANCE )== (uint8_t *) memblock)
            {
                if(!count)
                {
                    heap_free(memblock);
                    return NULL;
                }
                //sprawdzenie czy to jest ten sam blok i nie chce zwiekszyc o 0
                if(pcurrent->size == count)
                    return memblock;
                // sprawdzenie czy chce zmniejszyc
                if(pcurrent->usersize> count)
                {
                    //tak zmiejszamy
                    pcurrent->usersize = count;
                    //nowa płotka  z tylu
                    *((uint8_t *)pcurrent +sizeof(struct block_t)+ pcurrent->usersize+ SIZEBLOCK_FIANCE) = FENCE;
                    //nowa check suma
                    pcurrent->checksum = chcecksum((uint8_t*)pcurrent,32);
                    return memblock;
                } else
                {
                    // powiekszamy
                    // sprawdznie czy mozemy rozszerzyc w tym samym bloku
                    if(pcurrent->size>= count)
                    {
                        pcurrent->usersize = count;
                        *((uint8_t *)pcurrent + sizeof(struct block_t) + pcurrent->usersize+ SIZEBLOCK_FIANCE) = FENCE;
                        pcurrent->checksum = chcecksum((uint8_t*)pcurrent,32);
                    }
                    // Sprawdzamy czy blok po prawo jest wolny i nie jest ostatni
                    if(pcurrent->pnext->free && pcurrent->pnext != heap->ptail)
                    {
                        if(pcurrent->pnext->size>(count- pcurrent->size) + sizeof(struct block_t)+2*SIZEBLOCK_FIANCE)
                        {
                            struct block_t * newneibor = ( struct block_t *)((uint8_t * )pcurrent + sizeof(struct block_t)
                                    + pcurrent->size +2*SIZEBLOCK_FIANCE);

                            newneibor->size = pcurrent->pnext->size  - (count- pcurrent->size) ;
                            newneibor->free = 1;
                            newneibor->pprev = pcurrent;
                            newneibor->pnext = pcurrent->pnext->pnext;
                            newneibor->checksum = chcecksum((uint8_t*)newneibor,32);

                            pcurrent->usersize = count;
                            pcurrent->size += count- pcurrent->size;

                            pcurrent->pnext->pprev = newneibor;
                            pcurrent->pnext->checksum = chcecksum((uint8_t*)pcurrent->pnext,32);

                            pcurrent->pnext = newneibor;
                            pcurrent->checksum = chcecksum((uint8_t*)pcurrent,32);

                            *((uint8_t *)pcurrent +sizeof(struct block_t)+pcurrent->usersize+ SIZEBLOCK_FIANCE) = FENCE;
                            return  memblock;
                            // mozna rozszerzyc i cos zostanie do nowej sturktury

                        }else if(pcurrent->pnext->size + sizeof(struct block_t)>=(count- pcurrent->size))
                        {
                            // usuwamy wolny CAŁY wolny blok
                            pcurrent->usersize = count;
                            pcurrent->size += pcurrent->pnext->size + SIZEBLOCK_FIANCE*2 + sizeof(struct block_t);

                            pcurrent->pnext = pcurrent->pnext->pnext;
                            pcurrent->pnext->pprev = pcurrent;

                            *((uint8_t *)pcurrent +sizeof(struct block_t)+pcurrent->usersize+ SIZEBLOCK_FIANCE) = FENCE;

                            pcurrent->checksum = chcecksum((uint8_t*)pcurrent,32);
                            pcurrent->pnext->checksum = chcecksum((uint8_t*)pcurrent->pnext,32);
                            return memblock;
                        }
                    }else
                    {
                        // na koncu listy przesuwamy blok koncowy
                        unsigned  int  newsize = count -  pcurrent->usersize;
                        void * newblock = custom_sbrk(newsize);
                        if(newblock !=(void *)-1)
                        {
                            // przesuwamy tail
                            struct block_t * newtail = ( struct block_t *)((uint8_t * )newblock + newsize - sizeof(struct block_t));
                            newtail->size = 0;
                            newtail->usersize = 0;
                            newtail->pprev = pcurrent;
                            newtail->pnext = NULL;
                            newtail->free = 0;

                            pcurrent->usersize +=newsize;
                            pcurrent->size+=newsize;
                            pcurrent->pnext = newtail;
                             // plotki
                            *((uint8_t *)pcurrent +sizeof(struct block_t)) = FENCE;
                            *((uint8_t *)pcurrent +sizeof(struct block_t)+ pcurrent->usersize+ SIZEBLOCK_FIANCE) = FENCE;
                            //checksumy
                            pcurrent->checksum = chcecksum((uint8_t*) pcurrent,32);
                            newtail->checksum = chcecksum((uint8_t*) newtail,32);

                            heap->sizeheap +=newsize;
                            heap->ptail = newtail;
                            heap->checksum = chcecksum((uint8_t*)heap,sizeof(struct  heap_t));
                            return  memblock;
                            // zwieszamy rozmiar bloku
                            //ustawiamy plotke
                            // zmieniamy chcecksumy
                        } else
                            return  NULL;
                    }

                }
            }
        }
    }
    if(get_pointer_type(memblock) == pointer_valid) {
        // nowy blok
        void *newblockfrommalloc = heap_malloc(count);
        if (newblockfrommalloc) {
            //zwolnienie starego
            memcpy(newblockfrommalloc,memblock,(( struct block_t *)((uint8_t * )memblock - sizeof(struct block_t)-SIZEBLOCK_FIANCE))->usersize);
            heap_free(memblock);
            return newblockfrommalloc;
        }else
        {
            return NULL;
        }
    }

    return  NULL;
}
void  heap_free(void* memblock)
{
    if(memblock!=NULL && heap && heap_validate()==0) {
        struct block_t *pcurrent;

        for (pcurrent = heap->phead; pcurrent != heap->ptail; pcurrent = pcurrent->pnext) {

            if ((uint8_t *) pcurrent + sizeof(struct block_t) + SIZEBLOCK_FIANCE == (uint8_t *) memblock){
                pcurrent->free = 1;
                pcurrent->checksum = chcecksum((uint8_t *) pcurrent, sizeof(struct block_t));
                //sprawdzanie czy sasiad nie jest wolny i laczenie
                //prawy
                if (((uint8_t*)pcurrent +pcurrent->size + 2*SIZEBLOCK_FIANCE  + sizeof(struct block_t)) ==(uint8_t*)(pcurrent->pnext) &&
                pcurrent->pnext != heap->ptail  && pcurrent->pnext->free == 1) {           
                    pcurrent->size += sizeof(struct block_t) + pcurrent->pnext->size + 2 * SIZEBLOCK_FIANCE;
                    pcurrent->pnext->pnext->pprev = pcurrent;
                    pcurrent->pnext->pnext->checksum = chcecksum((uint8_t*)pcurrent->pnext->pnext, sizeof(struct block_t));
                    pcurrent->pnext = pcurrent->pnext->pnext;
                    pcurrent->checksum = chcecksum((uint8_t*)pcurrent, sizeof(struct block_t));

                }
                //lewy i nie jest granica
                if (((uint8_t*)pcurrent->pprev +pcurrent->pprev->size + 2*SIZEBLOCK_FIANCE+sizeof(struct block_t)) ==(uint8_t*)(pcurrent) &&
                        pcurrent->pprev != heap->phead && pcurrent->pprev->free == 1) {
                    pcurrent->pprev->size += sizeof(struct block_t) + pcurrent->size + 2 * SIZEBLOCK_FIANCE;
                    pcurrent->pnext->pprev = pcurrent->pprev;
                    pcurrent->pprev->pnext = pcurrent->pnext;
                    pcurrent->pprev->checksum = chcecksum((uint8_t*)pcurrent->pprev, sizeof(struct block_t));
                    pcurrent->pnext->checksum = chcecksum((uint8_t*)pcurrent->pnext, sizeof(struct block_t));
                }
                break;
            }
        }
    }
}
size_t   heap_get_largest_used_block_size(void)
{
    if(heap && !heap_validate())
    {
        int isselect = 0;
        unsigned int maxSize= 0;
        const struct block_t* pcurrent;
        for (pcurrent = heap->phead; pcurrent != NULL; pcurrent = pcurrent->pnext)
        {
            if(!pcurrent->free)
            {
                if(!isselect) {
                    isselect = 1;
                    maxSize = pcurrent->usersize;
                }
                else if( pcurrent->usersize> maxSize)
                    maxSize = pcurrent->usersize;

            }

        }
        return (size_t)maxSize;
    }
    return 0;
}
enum pointer_type_t get_pointer_type(const void* const pointer)
{
    if(pointer == NULL)
        return  pointer_null;
    if(heap_validate()!= 0)
        return pointer_heap_corrupted;
    if(heap)
    {
        if((uint8_t *)pointer<(uint8_t *)heap || (uint8_t *)pointer>(uint8_t *)heap->ptail+sizeof(struct block_t))
            return pointer_unallocated;
        struct block_t* pcurrent;
        for (pcurrent = heap->phead; pcurrent != NULL; pcurrent = pcurrent->pnext) {
            if(((uint8_t *)pcurrent+sizeof(struct block_t)+SIZEBLOCK_FIANCE)==(uint8_t *)pointer)
            {
                if(pcurrent->free)
                    return pointer_unallocated;
                else
                    return pointer_valid;
            }else
            if((uint8_t *)pcurrent+sizeof(struct block_t)==(uint8_t *)pointer|| ((uint8_t *)pcurrent+sizeof(struct block_t)+pcurrent->usersize+ SIZEBLOCK_FIANCE)==(uint8_t *)pointer)
            {

                if(pcurrent->free)
                    return pointer_unallocated;

                return pointer_inside_fences;
            }else if(((uint8_t*)pcurrent<=(uint8_t*)pointer) && ((uint8_t*)((uint8_t *)pcurrent+sizeof(struct  block_t))>(uint8_t*)pointer))
            {
                return pointer_control_block;
            }
            else if((uint8_t *)pcurrent + sizeof(struct  block_t)+SIZEBLOCK_FIANCE < (uint8_t *)pointer &&
                     ((uint8_t *)pcurrent + sizeof(struct  block_t)+SIZEBLOCK_FIANCE+pcurrent->usersize) > (uint8_t *)pointer)
            {
                if(pcurrent->free)
                    return pointer_unallocated;

                return pointer_inside_data_block;
            }else if(pcurrent->free && ((uint8_t *)pcurrent + sizeof(struct  block_t)+SIZEBLOCK_FIANCE)<= (uint8_t *)pointer &&
                ((uint8_t *)pcurrent + sizeof(struct  block_t)+SIZEBLOCK_FIANCE+pcurrent->size)>= (uint8_t *)pointer)
            {
                return pointer_unallocated;
            }else if(!pcurrent->free && ((uint8_t *)pcurrent + sizeof(struct  block_t)+SIZEBLOCK_FIANCE+pcurrent->usersize)< (uint8_t *)pointer &&
                     ((uint8_t *)pcurrent + sizeof(struct  block_t)+SIZEBLOCK_FIANCE+pcurrent->size)> (uint8_t *)pointer)
            {
                return pointer_unallocated;
            }
        }
        return pointer_unallocated;
    }
    return pointer_null;
}
int heap_validate(void)
{
    if (heap == NULL)
        return 2;

    const struct block_t* pcurrent;
//    struct block_t* pcurrentcopy=heap->phead;
    for (pcurrent = heap->phead; pcurrent != NULL; pcurrent = pcurrent->pnext)
    {
        if(pcurrent->checksum!= chcecksum((uint8_t *)pcurrent,32))
        {
            return 3;
        }
        //sprawdzam plotki
        if((pcurrent!= heap->ptail && pcurrent!= heap->phead) && pcurrent->size !=0 && pcurrent->free == 0)
        {
            if(
                    (*((uint8_t * )pcurrent +sizeof(struct block_t)) != FENCE
                     ||
                     (*((uint8_t *)pcurrent +sizeof(struct block_t) + pcurrent->usersize+ SIZEBLOCK_FIANCE) != FENCE)))
            {
                return 1;
            }
        }
    }
    return  0;
}
void* heap_malloc_aligned(size_t count)
{

    if(heap && heap_validate() == 0&& count>0)
    {
        //najpierw chodzimy i sprawdzamy
        struct block_t* pnew = findfirstfreeblockallign(count);

        if(!pnew) {
            const size_t  mask = PAGE_SIZE -1;
            const uintptr_t  mem = (uintptr_t) custom_sbrk(0);
            void *ptr = (void *)((mem + mask) & ~mask);
            size_t l = (uint8_t*)ptr - (uint8_t*)heap->ptail - sizeof(struct block_t);
            if (l <sizeof(struct block_t) + SIZEBLOCK_FIANCE)
            {
                l += PAGE_SIZE;
            }
                void *newblock = custom_sbrk( sizeof(struct block_t) + l + count + 1 * SIZEBLOCK_FIANCE);
                if (newblock != (void *) -1) {
                    //dodanie ilosci danych pobranych od OS
                    heap->sizeheap += sizeof(struct block_t) + l + count + 1 * SIZEBLOCK_FIANCE;
                    // sturktura nowej alokacji
                    struct block_t *alocation = (struct block_t *) ((uint8_t *) newblock + l - SIZEBLOCK_FIANCE -sizeof(struct block_t));
                 
                    //przesuniecie koncowego bloku
                    struct block_t *newtail = (struct block_t *) ((uint8_t *) newblock + l + count +
                            1 * SIZEBLOCK_FIANCE);


                    alocation->free = 0;
                    alocation->size = count;
                    alocation->usersize = count;
                    alocation->pnext = newtail;
                    alocation->pprev = heap->ptail;
                    alocation->checksum = chcecksum((uint8_t *) alocation, 32);


                    newtail->free = 0;
                    newtail->size = 0;
                    newtail->pnext = NULL;
                    newtail->usersize = 0;
                    newtail->pprev = alocation;
                    newtail->checksum = chcecksum((uint8_t *) newtail, 32);


                    heap->ptail->free = 1;
                    heap->ptail->pnext = alocation;
                    heap->ptail->checksum = chcecksum((uint8_t *) heap->ptail, 32);
                    heap->ptail = newtail;
                    heap->checksum = chcecksum((void *) heap, sizeof(struct heap_t));
                    // plotki
                    *((uint8_t *) alocation + sizeof(struct block_t)) = FENCE;
                    *((uint8_t *) alocation + sizeof(struct block_t) + alocation->usersize + SIZEBLOCK_FIANCE) = FENCE;

                    return ((uint8_t *) alocation + sizeof(struct block_t) + SIZEBLOCK_FIANCE);
                }
        }else
        {
            *((uint8_t *)pnew+sizeof(struct block_t)) = FENCE;
            *((uint8_t *)pnew+sizeof(struct block_t)+count+SIZEBLOCK_FIANCE) = FENCE;
            pnew->free = 0;
            //CAD4 6FDE - CAD4 6000
            if(count > pnew->usersize)
            {
                pnew->size = pnew->size+(((uint8_t*)pnew->pnext-SIZEBLOCK_FIANCE -((uint8_t*)(pnew +sizeof(struct block_t))+SIZEBLOCK_FIANCE+pnew->size)));
               // pnew->size = ((uint8_t*)(pnew->pnext - SIZEBLOCK_FIANCE) -((uint8_t*)(pnew +sizeof(struct block_t)+SIZEBLOCK_FIANCE)));
            }
            pnew->usersize = count;
            // *(char*)((uint8_t *)pnew+sizeof(struct block_t)+size) = FENCE;
            //ustawic size jaki chce user a z reszta nie wiem co zrobic
            //suma kontrolna
            pnew->checksum = chcecksum((uint8_t*)pnew,32);
            //heap->checksum = chcecksum((uint8_t*)heap,32);
            return ((uint8_t *)pnew +sizeof(struct block_t) + SIZEBLOCK_FIANCE) ;
        }
    }
    return NULL;
}
//TODO do napisania
void* heap_calloc_aligned(size_t number, size_t size)
{
    void* pointerToReturn = heap_malloc_aligned(number*size);
    if(pointerToReturn)
    {
        for(size_t i = 0;i<number*size;++i)
        {
            *((uint8_t *)pointerToReturn+i) = 0;
        }
    }
    return  pointerToReturn;
}
//TODO do napisania
void* heap_realloc_aligned(void* memblock, size_t size)
{
    if(heap) {
        if (memblock == NULL) {
            return heap_malloc_aligned(size);
        }
        for (struct block_t *pcurrent = heap->phead;
             pcurrent != heap->ptail;
             pcurrent = pcurrent->pnext)
        {
            if ((uint8_t *)((uint8_t *)pcurrent+sizeof(struct block_t)+SIZEBLOCK_FIANCE)==(uint8_t *)memblock) {
                if(size==0)
                {
                    heap_free(memblock);
                    return NULL;
                }
                if(pcurrent->size == size)
                    return memblock;
                // sprawdzenie czy chce zmniejszyc
                if(pcurrent->usersize>= size) {
                    //tak zmiejszamy
                    pcurrent->usersize = size;
                    //nowa płotka  z tylu
                    *((uint8_t *) pcurrent + sizeof(struct block_t) + pcurrent->usersize + SIZEBLOCK_FIANCE) = FENCE;
                    //nowa check suma
                    pcurrent->checksum = chcecksum((uint8_t *) pcurrent, 32);
                    return memblock;
                }else if(pcurrent->pnext != heap->ptail)
                {
                    //czy jest wolny ten 0 sama struktura stary ogon  odleglosc == 0
                    if(pcurrent->pnext->free && pcurrent->pnext != heap->ptail && pcurrent->pnext->size ==0)
                    {
                        pcurrent->size += sizeof(struct block_t) + pcurrent->pnext->size + 2 * SIZEBLOCK_FIANCE;
                        pcurrent->pnext->pnext->pprev = pcurrent;

                        //  if(pcurrent->pnext->pnext != heap->ptail)
                        pcurrent->pnext->pnext->checksum = chcecksum((uint8_t*)pcurrent->pnext->pnext, sizeof(struct block_t));
                        pcurrent->pnext = pcurrent->pnext->pnext;
                        pcurrent->checksum = chcecksum((uint8_t*)pcurrent, sizeof(struct block_t));
                        if(pcurrent->usersize>= size)
                        {
                            pcurrent->usersize = size;
                            *((uint8_t *)pcurrent +sizeof(struct block_t)+ pcurrent->usersize+ SIZEBLOCK_FIANCE) = FENCE;
                            pcurrent->checksum = chcecksum((uint8_t*)pcurrent, sizeof(struct block_t));
                            return  memblock;
                        }
                    }
                    // rozmiar za ale przed kolejna stuktura jak w malloc
                    if((long)pcurrent->size+(((uint8_t*)pcurrent->pnext-SIZEBLOCK_FIANCE -((uint8_t*)(pcurrent +sizeof(struct block_t))+SIZEBLOCK_FIANCE+pcurrent->size))) >= (long)size)
                    {
                        pcurrent->usersize = size;
                        pcurrent->size = size;
                        *((uint8_t *)pcurrent +sizeof(struct block_t)+ pcurrent->usersize+ SIZEBLOCK_FIANCE) = FENCE;
                        pcurrent->checksum = chcecksum((uint8_t*)pcurrent, sizeof(struct block_t));
                        return  memblock;
                    }
                    //czy za nia jest wolny blok ktory mozna obciac
                    if(pcurrent->pnext->free  &&
                    pcurrent->pnext->size + (long)pcurrent->size+(((uint8_t*)pcurrent->pnext-SIZEBLOCK_FIANCE -((uint8_t*)(pcurrent +sizeof(struct block_t))+SIZEBLOCK_FIANCE+pcurrent->size)))
                    + (long)pcurrent->pnext->size+(((uint8_t*)pcurrent->pnext->pnext-SIZEBLOCK_FIANCE -((uint8_t*)(pcurrent +sizeof(struct block_t))+SIZEBLOCK_FIANCE+pcurrent->pnext->size)))>(long)size)
                    {
                        pcurrent->usersize = size;
                        pcurrent->size = size;
                        *((uint8_t *)pcurrent +sizeof(struct block_t)+ pcurrent->usersize+ SIZEBLOCK_FIANCE) = FENCE;
                        pcurrent->checksum = chcecksum((uint8_t*)pcurrent, sizeof(struct block_t));
                        return  memblock;
                    }
                    //nowa alokacja
                    // jesli jednak nie to nowy blok i kopia pamieci
                    void *ptr1 = heap_malloc_aligned(size);
                    if(ptr1) {
                        for (size_t i = 0; i <pcurrent->usersize; ++i) {
                            *((uint8_t *) ptr1 + i) =  *((uint8_t *) memblock + i) ;
                        }
                        heap_free(memblock);
                        return ptr1;
                    }else
                        return NULL;
                }else
                {
                    unsigned  int  newsize = size -  pcurrent->usersize;
                        void * newblocksbrk = custom_sbrk(newsize);
                        if(newblocksbrk !=(void *)-1)
                        {
                            // przesuwamy tail
                            struct block_t * newtail = ( struct block_t *)((uint8_t * )newblocksbrk + newsize - sizeof(struct block_t));
                            newtail->size = 0;
                            newtail->usersize = 0;
                            newtail->pprev = pcurrent;
                            newtail->pnext = NULL;
                            newtail->free = 0;

                            pcurrent->usersize +=newsize;
                            pcurrent->size+=newsize;
                            pcurrent->pnext = newtail;
                            // plotki
                            *((uint8_t *)pcurrent +sizeof(struct block_t)) = FENCE;
                            *((uint8_t *)pcurrent +sizeof(struct block_t)+ pcurrent->usersize+ SIZEBLOCK_FIANCE) = FENCE;
                            //checksumy
                            pcurrent->checksum = chcecksum((uint8_t*) pcurrent,32);
                            newtail->checksum = chcecksum((uint8_t*) newtail,32);

                            heap->sizeheap +=newsize;
                            heap->ptail = newtail;
                            heap->checksum = chcecksum((uint8_t*)heap,sizeof(struct  heap_t));
                            return  memblock;
                            // zwieszamy rozmiar bloku
                            //ustawiamy plotke
                            // zmieniamy chcecksumy
                    }

                }
            }
        }

    }
    return NULL;
}
void* findfirstfreeblockallign(unsigned int size)
{
    if(heap) {

        //struct block_t *firstfreeblock = heap->phead->pnext; // pierwszy blok uzytkownika
        for (struct block_t *pcurrent = heap->phead;
             pcurrent != heap->ptail;
             pcurrent = pcurrent->pnext) {
 
            if (pcurrent->free == 0)
                continue;

       
            // w zwyklym rozmiarze
            if (pcurrent->size >= size &&  ((intptr_t) ((intptr_t)pcurrent+sizeof(struct block_t)+SIZEBLOCK_FIANCE) & (intptr_t) (PAGE_SIZE - 1))==0) {
                // udalo sie znalesc dobry blok
                return pcurrent;
            }
            //pamietajac o plotce
            //
            if (pcurrent->pnext != heap->ptail && (long)pcurrent->size+(((uint8_t*)pcurrent->pnext-SIZEBLOCK_FIANCE -((uint8_t*)(pcurrent +sizeof(struct block_t))+SIZEBLOCK_FIANCE+pcurrent->size))) >= (long)size
                    &&  ((intptr_t) ((intptr_t)pcurrent+sizeof(struct block_t)+SIZEBLOCK_FIANCE) & (intptr_t) (PAGE_SIZE - 1))==0)
            {
               return pcurrent;
            }
        }
        
    }
    return  NULL;
}
void* findfirstfreeblock(unsigned int size)
{
    if(heap) {
      
        //struct block_t *firstfreeblock = heap->phead->pnext; // pierwszy blok uzytkownika
        for (struct block_t *pcurrent = heap->phead;
             pcurrent != heap->ptail;
             pcurrent = pcurrent->pnext) {

       
            if (pcurrent->free == 0)
                continue;

            if (pcurrent->size >= size) {
                // udalo sie znalesc dobry blok

                return pcurrent;
            }
        }
    }
    return  NULL;
}

