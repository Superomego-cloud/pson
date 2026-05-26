#include "json/json_dsa.h"
#ifndef  PSON_DSA_H


typedef struct psc PSON_scope;

struct psc{
    JSON_container *dict;
    PSON_scope *par;
};

JSON_val *PSON_dereferenceValue(PSON_scope *scope, JSON_val *var);
char      PSON_assignValue(PSON_scope *scope, JSON_val *var);

PSON_scope *PSON_createScope(PSON_scope *par);
void PSON_destroyScope(PSON_scope *scope);

void PSON_destroyValue(JSON_val *val);

/*
VARIABLE ASSIGNMENT FUNCTION
this will delete the name-value pair that is passed
*/
char PSON_assignValue(PSON_scope *scope, JSON_val *var){
    
    if(scope == NULL || var == NULL) return 0;
    if(var->type != PAIR_JT) return 0;
    
    JSON_val **pair = (JSON_val**) var->data;
    char res = JSON_insertDict(scope->dict, pair[0], pair[1]);

    free(var->data);
    free(var);
    return res;

}

/*
VARIABLE DEREFERENCING FUNCTION
this will delete the name-value pair that is passed
*/
JSON_val *PSON_dereferenceValue(PSON_scope *scope, JSON_val *var){
    
    if(scope == NULL || var == NULL) return NULL;
    if(var->type != PAIR_JT) return NULL;

    JSON_val **pair = (JSON_val**) var->data;

    if(pair[1] != NULL){

        JSON_val *v = pair[1];

        if(!PSON_assignValue(scope, var)) return NULL;
        return JSON_copyValue(v);
    }

    JSON_val *v = JSON_getDict(scope->dict, pair[0]);
    if(v == NULL) return PSON_dereferenceValue(scope->par, var);;
    

    free(pair);
    free(var);
    return JSON_copyValue(v);

}

PSON_scope *PSON_createScope(PSON_scope *par){

    ALLOC(ret, PSON_scope);
    ret->dict = JSON_createContainer();

    if(ret->dict == NULL){
        free(ret);
        return NULL;
    }

    ret->par = par;

    if(par != NULL){
        if(par->dict->keys_head == NULL) ret->par = par->par;
    }
        
    return ret;

}

void PSON_destroyScope(PSON_scope *scope){
    
    if(scope == NULL) return;
    JSON_destroyContainer(scope->dict, OBJECT_JT);
    return;

}

// ok now I have to reimplement all of this for iterables :wilted_rose:
void PSON_destroyValue(JSON_val *val){

    if(val->type == PAIR_JT){
        PSON_destroyValue(((JSON_val**) val->data)[0]);
        PSON_destroyValue(((JSON_val**) val->data)[1]);
        free(val->data);
        free(val);
        return;
    }

}

#define PSON_DSA_H
#endif