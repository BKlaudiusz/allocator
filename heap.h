//
// Created by root on 11/14/20.
//

#ifndef PROJECT1_HEAP_H
#define PROJECT1_HEAP_H
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "custom_unistd.h"
#define FENCE '#'
#define PAGE_SIZE 4096
#define SIZEBLOCK_T 32
#define SIZEBLOCK_FIANCE 1
#define MEMORY_SIZE_HEAP 0
enum pointer_type_t
{
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};
//struktoru kontrolna stery
struct heap_t {
    //poczatek sterty
    struct block_t* phead;
    // koniec sterty
    struct block_t* ptail;
    //pierwszy wolny blok
    uint8_t* heap_start;
    size_t sizeheap;
    unsigned int checksum;
};
// struktura blokow
struct block_t {
    // nastepny blok zaalokowany
    struct block_t* pnext;
    // poprzedni blok zaalakowany
    struct block_t* pprev;
    // rozmiar
    unsigned int size;
    // wolny czy zajety
    short free; // 1-wolny; 0-zajęty
    unsigned int checksum;
    unsigned int usersize;
};

int heap_setup(void);
void heap_clean(void);
void* heap_malloc(size_t size);
void* heap_calloc(size_t number, size_t size);
unsigned int chcecksum(const void * pointer,size_t size);
void* heap_realloc(void* memblock, size_t count);
void  heap_free(void* memblock);
size_t   heap_get_largest_used_block_size(void);
enum pointer_type_t get_pointer_type(const void* const pointer);
void* findfirstfreeblockallign(unsigned int size);
int heap_validate(void);
void* heap_malloc_aligned(size_t count);
void* heap_calloc_aligned(size_t number, size_t size);
void* heap_realloc_aligned(void* memblock, size_t size);
void* findfirstfreeblock(unsigned int size);
#endif //PROJECT1_HEAP_H
