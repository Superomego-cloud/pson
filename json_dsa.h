#ifndef  JSON_DSA_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// constants for FNV hashing algo
#define FNV_PRIME 16777619
#define FNV_OBASIS 2166136261

// convenience macros
#define MAX(x, y) x > y? x : y;
#define MIN(x, y) x > y? x : y;

// macros for memory allocation

// This macro auto writes the expression for getting a pointer to an allocated element of type T
#define ALLOC(V, T) T *V = (T*) malloc(sizeof(T))

/* 
This macro auto writes the expression for getting a pointer to an allocated array 
of n elements of type T
*/
#define M_ALLOC(V, T, n) T *V = (T*) malloc(n * sizeof(T))

typedef enum{

    BOOL_JT = 0,
    INTEGER_JT,
    FLOAT_JT,
    STRING_JT,
    ARRAY_JT,
    OBJECT_JT,
    VAR_JT,
    FUNC_JT,
    LAST_JT,

}obj_t;

typedef struct pc JSON_container;
typedef struct pdl JSON_dictList;
typedef struct pv JSON_val;

struct pv{

    obj_t type; // type of object
    size_t len; // amount of string processed to get it
    void* data; // underlying data structure 

};

struct pc{
    size_t size;
    size_t val_c;
    void** arr; // if type is ARRAY_JT, then this is an array of JSON_val 
                // otherwise it's an array of JSON_dictlist
};

struct pdl{
    JSON_val *key;
    JSON_val *val;
    JSON_dictList *next;
};

uint32_t JSON_hash(JSON_val *key);
char JSON_cmpstr(JSON_val *a, JSON_val *b);

char JSON_resize(JSON_container *c, obj_t type, size_t nsize);

char JSON_insertDict(JSON_container *dct, JSON_val *key, JSON_val* val);
void JSON_eraseDict(JSON_container *dct, JSON_val *key);
JSON_val *JSON_getValue(JSON_container *dct, JSON_val *key);

char JSON_pushback(JSON_container *arr, JSON_val* val);

void JSON_destroyValue(JSON_val *v);
void JSON_destroyContainer(JSON_container *c, obj_t type);
void JSON_printVal(JSON_val *v);
void JSON_printValLn(JSON_val *v);


/*
FNV-1a algorithm
*/
uint32_t JSON_hash(JSON_val *key){

    uint64_t val = FNV_OBASIS;
    if(key->type != STRING_JT) return 0;

    char *d = key->data;

    for(int i = 0; i < key->len; ++i){
        val = (val * FNV_PRIME)%(1LL << 32);
        val = val ^ d[i];
    }

    uint32_t ret = val;
    return ret;

}

char JSON_resize(JSON_container *c, obj_t type, size_t nsize){

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
        
        for(int i = 0; i < c->val_c; ++i){
            c->arr[i] = oldarr[i];
        }

    }
    else{
        JSON_dictList **ha = (JSON_dictList **) oldarr;

        for(int i = 0; i < nsize; ++i){
            c->arr[i] = NULL;
        }

        for(int i = 0; i < osize; ++i){
            
            JSON_dictList *cn = ha[i], *n;
            
            while(cn != NULL){
                
                JSON_insertDict(c, cn->key, cn->val);
                n = cn;
                cn = cn->next;
                free(n);

            }
        }

    }

    free(oldarr);
    return 1;

}

char JSON_insertDict(JSON_container *dct, JSON_val *key, JSON_val* val){

    if(key == NULL || val == NULL) return 0;

    if(2*(dct->val_c + 1) >= dct->size){
        if(!JSON_resize(dct, OBJECT_JT, 0)) return 0;
    }

    size_t loc = JSON_hash(key) % dct->size;
    JSON_dictList *cn = dct->arr[loc];

    if(cn != NULL){

        while(1){
            
            if(JSON_cmpstr(key, cn->key)){
                cn->val = val;
                return 1;
            }
            if(cn->next != NULL) cn = cn->next;
            else break;

        }

    }

    dct->val_c++;

    ALLOC(ne, JSON_dictList); 
    if(ne == NULL) return 0;

    ne->key = key;
    ne->val = val;
    ne->next = NULL;

    if(cn == NULL) dct->arr[loc] = ne;
    else cn->next = ne;

    return 1;

}

char JSON_pushback(JSON_container *arr, JSON_val *val){

    if((arr->val_c + 1) * 2 >= arr->size){
        if(!JSON_resize(arr, ARRAY_JT, 0)){
            return 0;
        }
    }
    
    arr->arr[arr->val_c] = val;
    arr->val_c++;

    return 1;
}

void JSON_eraseDict(JSON_container *dct, JSON_val *key){

    size_t loc = JSON_hash(key) % dct->size;
    JSON_dictList *p = NULL, *cn = dct->arr[loc];

    while(cn != NULL){

        if(JSON_cmpstr(key, cn->key)){
            
            if(p == NULL) dct->arr[loc] = cn->next;
            else p->next = cn->next;

            free(cn->key);
            free(cn->val);

        }
        p = cn;
        cn = cn->next;
    }

};

void JSON_destroyValue(JSON_val *v){

    if(v == NULL) return;

    if(v->type == OBJECT_JT || v->type == ARRAY_JT) JSON_destroyContainer(v->data, v->type);
    else free(v->data);

    free(v);
    return;

}

void JSON_destroyContainer(JSON_container *c, obj_t type){

    if(type == ARRAY_JT){

        for(int i = 0; i < c->size; ++i){
            JSON_destroyValue(c->arr[i]);
        }
    }
    else if(type == OBJECT_JT){
        for(int i = 0; i < c->size; ++i){
                
            JSON_dictList *cn = c->arr[i], *n;   
            while(cn != NULL){

                JSON_destroyValue(cn->key);
                JSON_destroyValue(cn->val);

                n = cn;
                cn = cn->next;
                free(n);
            }
        }
    }
    else{
        return;
    }

    free(c->arr);

};

JSON_val *JSON_getDict(JSON_container *dct, JSON_val *key){

    size_t loc = JSON_hash(key) % dct->size;

    JSON_dictList *cn = dct->arr[loc];
    while(cn != NULL){
        if(JSON_cmpstr(key, cn->key)){
            return cn->val;
        }
        cn = cn->next;
    }

    return NULL;

}

char JSON_cmpstr(JSON_val *a, JSON_val *b){
    
    if(a->type != STRING_JT || b->type != STRING_JT) return 0;
    if(a->len != b->len) return 0;

    char *ac = a->data, *bc = b->data;
    for(size_t i = 0; i < a->len; ++i){
        if(ac[i] != bc[i]) 
        return 0;
    }
    return 1;

}

char JSON_matchString(char *str1, char *str2, size_t len1, size_t len2){
    
    if(len1 > len2) return 0;
    
    for(int i = 0; i < len1; ++i) if(str1[i] != str2[i]) return 0;
    return 1;   

}

#define JSON_DSA_H
#endif