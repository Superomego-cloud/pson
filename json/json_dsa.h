#ifndef  JSON_DSA_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// constants for FNV hashing algo
#define FNV_PRIME 16777619
#define FNV_OBASIS 2166136261

// convenience macros
#define MAX(x, y) (x > y? x : y)
#define MIN(x, y) (x > y? y : x)

// macros for memory allocation

// This macro auto writes the expression for getting a pointer to an allocated element of type T
#define ALLOC(V, T) T *V = (T*) malloc(sizeof(T))

/* 
This macro auto writes the expression for getting a pointer to an allocated array 
of n elements of type T
*/
#define M_ALLOC(V, T, n) T *V = (T*) malloc(n * sizeof(T))

typedef enum{

    SPECIAL_JT = 0,
    INTEGER_JT,
    FLOAT_JT,
    STRING_JT,
    ARRAY_JT,
    OBJECT_JT,
    PAIR_JT,
    VAR_JT,
    FUNC_JT 

}obj_t;

#define isnumber(t) (t == INTEGER_JT || t == DOUBLE_JT)
#define isstr(t) (t == STRING_JT || t == VAR_JT)
#define iscontainer(t) (t == ARRAY_JT || t == OBJECT_JT || t == FUNC_JT)

typedef struct pc JSON_container;
typedef struct pdl JSON_dictList;
typedef struct pll JSON_doubleList;
typedef struct pv JSON_val;
typedef struct pkv JSON_pair;

struct pv{
    obj_t type; // type of object
    size_t len; // amount of string processed to get it
    void* data; // underlying data structure 
};

struct pc{
    size_t size; // amount of items
    size_t val_c; // amount of elements contained 
    
    // if ARRAY_JT, array of JSON_val*
    // if OBJECT_JT, array of JSON_dictList*
    void** arr; 

    // null if ARRAY_JT, 
    // pointers for start and end of keys DLL if OBJECT_JT
    JSON_doubleList* keys_head; 
    JSON_doubleList* keys_end;   
};

// head of a linked list in dictionary
struct pdl{
    JSON_val *key;
    JSON_val *val;
    JSON_doubleList *iter_entry;
    JSON_dictList *next;
};

// iterator for dictionary, DLL for O(1) deletion and no need for random access
struct pll{
    JSON_doubleList *prev;
    JSON_doubleList *next;
    JSON_val *val;
};

// JSON string functions
uint32_t JSON_hash(JSON_val *key);
char JSON_cmpkey(JSON_val *a, JSON_val *b);
char JSON_matchString(char *str1, char *str2, size_t len1, size_t len2);

// internal function that resizes container
char JSON_resize(JSON_container *c, obj_t type, size_t nsize);

// functions for JSON dictionnaries
char JSON_insertDict(JSON_container *dct, JSON_val *key, JSON_val* val);
void JSON_eraseDict(JSON_container *dct, JSON_val *key);
JSON_val *JSON_getDict(JSON_container *dct, JSON_val *key);
char JSON_pushback(JSON_container *arr, JSON_val* val);

// destroyer functions 
void JSON_destroyValue(JSON_val *v);
void JSON_destroyContainer(JSON_container *c, obj_t type);

/*
FNV-1a algorithm
*/
uint32_t JSON_hash(JSON_val *key){

    uint64_t val = FNV_OBASIS;
    if(key->type != STRING_JT && key->type != VAR_JT) return 0;

    char *d = key->data;

    for(int i = 0; i < key->len; ++i){
        val = (val * FNV_PRIME)%(1LL << 32);
        val = val ^ d[i];
    }

    uint32_t ret = val;
    return ret;

} 

char JSON_resize(JSON_container *c, obj_t type, size_t nsize){

    if(c == NULL) return 0;
    if(type != ARRAY_JT && type != OBJECT_JT) return 0;

    if(nsize == 0){
        if(c->size == 0) nsize = 1;
        else nsize = c->size*2;
    }

    void **oldarr = c->arr;

    c->arr = (void**) malloc(nsize * sizeof(void*));
    if(c->arr == NULL){
        c->arr = oldarr;
        return 0;
    }
    size_t osize = c->size;
    c->size = nsize;

    if(type == ARRAY_JT){
        for(int i = 0; i < MIN(c->val_c, nsize); ++i) c->arr[i] = oldarr[i];
    }
    else{

        JSON_dictList **ha = (JSON_dictList **) oldarr;

        for(int i = 0; i < nsize; ++i) c->arr[i] = NULL;

        for(int i = 0; i < osize; ++i){
            
            JSON_dictList *cn = ha[i], *n;
            
            while(cn != NULL){
                
                uint32_t nloc = JSON_hash(cn->key)%nsize;
                
                n = cn->next;
                cn->next = c->arr[nloc];
                c->arr[nloc] = cn;

                cn = n;
                
            }
        }

    }

    free(oldarr);
    return 1;

}

char JSON_insertDict(JSON_container *dct, JSON_val *key, JSON_val* val){

    if(dct == NULL || key == NULL || val == NULL) return 0;
    if(key->type != STRING_JT) return 0;

    if(2*(dct->val_c + 1) >= dct->size){
        if(!JSON_resize(dct, OBJECT_JT, 0)) return 0;
    }

    size_t loc = JSON_hash(key) % dct->size;
    JSON_dictList *cn = dct->arr[loc];

    if(cn != NULL){

        while(1){
            
            if(JSON_cmpkey(key, cn->key)){
                cn->val = val;
                return 1;
            }
            if(cn->next != NULL) cn = cn->next;
            else break;

        }

    }

    dct->val_c++;

    ALLOC(ne, JSON_dictList); 
    ALLOC(nke, JSON_doubleList); 

    if(ne == NULL || nke == NULL) return 0;

    nke->val = key;
    nke->prev = dct->keys_end;
    nke->next = NULL;

    if(dct->keys_end != NULL){
        dct->keys_end->next = nke;
    }
    dct->keys_end = nke;
    
    if(dct->keys_head == NULL) dct->keys_head = nke;

    ne->iter_entry = nke;
    ne->key = key;
    ne->val = val;
    ne->next = NULL;

    if(cn == NULL) dct->arr[loc] = ne;
    else cn->next = ne;

    return 1;

}

char JSON_pushback(JSON_container *arr, JSON_val *val){

    if(arr == NULL || val == NULL) return 0;

    if((arr->val_c + 1) * 2 >= arr->size){
        if(!JSON_resize(arr, ARRAY_JT, 0)){
            return 0;
        }
    }

    ((JSON_val **) arr->arr)[arr->val_c] = val;
    arr->val_c++;

    return 1;
}

void JSON_eraseDict(JSON_container *dct, JSON_val *key){

    if(key == NULL || dct == NULL) return;
    if(key->type != STRING_JT) return;

    size_t loc = JSON_hash(key) % dct->size;
    JSON_dictList *p = NULL, *cn = dct->arr[loc];

    while(cn != NULL){

        if(!JSON_cmpkey(key, cn->key)){
            p = cn;
            cn = cn->next;
            continue;
        }

        
        if(p == NULL) dct->arr[loc] = cn->next;
        else p->next = cn->next;
                
        if(cn->iter_entry == dct->keys_head) dct->keys_head = cn->iter_entry->next;
        else cn->iter_entry->prev->next = cn->iter_entry->next;
        
        if(cn->iter_entry == dct->keys_end) dct->keys_end = dct->keys_end->prev;
        else cn->iter_entry->next->prev = cn->iter_entry->prev;

        free(cn->iter_entry);
        free(cn->key);
        free(cn->val);
        free(cn);
        
        return;

    }

}


void JSON_destroyValue(JSON_val *v){

    if(v == NULL) return;

    if(iscontainer(v->type)) JSON_destroyContainer(v->data, v->type);
    else{
        if(v->type == PAIR_JT){
            JSON_val **arr = v->data;
            JSON_destroyValue(arr[0]);
            JSON_destroyValue(arr[1]);
        }

        if(v->type != SPECIAL_JT){
            free(v->data); // ideally we don't want to free the number one
            free(v); 
        } 
    } 
    
    return;

}

void JSON_destroyContainer(JSON_container *c, obj_t type){

    if(c == NULL) return;

    if(type == ARRAY_JT){

        for(int i = 0; i < c->size; ++i){
            JSON_destroyValue(c->arr[i]);
        }
    }
    else if(type == OBJECT_JT){
        
        JSON_doubleList *nke = c->keys_head, *ke;

        while(nke != NULL){

            JSON_val *assoc_val = JSON_getDict(c, nke->val);
            JSON_destroyValue(assoc_val);
            JSON_destroyValue(nke->val);

            ke = nke;
            nke = ke->next;
            free(ke);
        }
        
    }
    else return;

    free(c->arr);
    free(c);

};

JSON_val *JSON_getDict(JSON_container *dct, JSON_val *key){

    size_t loc = JSON_hash(key) % dct->size;

    JSON_dictList *cn = dct->arr[loc];
    while(cn != NULL){
        if(JSON_cmpkey(key, cn->key)){
            return cn->val;
        }
        cn = cn->next;
    }

    return NULL;

}

char JSON_cmpkey(JSON_val *a, JSON_val *b){
    
    if(a->type != b->type) return 0;
    if(a->len != b->len) return 0;
    if(a->type != STRING_JT && a->type != VAR_JT) return 0;
    
    return JSON_matchString(a->data, b->data, a->len, b->len);

}

JSON_container* JSON_createContainer(){
    
    ALLOC(arr, JSON_container);
    
    if(arr != NULL){
        arr->size = 0;
        arr->val_c = 0;
        arr->keys_end = NULL;
        arr->keys_head = NULL;
    }
        
    return arr;
}

char JSON_matchString(char *str1, char *str2, size_t len1, size_t len2){
    
    if(len1 > len2) return 0;
    for(int i = 0; i < len1; ++i) if(str1[i] != str2[i]) return 0;
    return 1;   
}

#define JSON_DSA_H
#endif