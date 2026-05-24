#include "json/json_dsa.h"
#ifndef  PSON_DSA_H

JSON_val *PSON_dereferenceValue(PSON_scope *scope, JSON_val *var);

typedef struct psc PSON_scope;

struct psc{
    JSON_container *dict;
    PSON_scope *par;
};

char PSON_assignValue(PSON_scope *scope, JSON_val *var){
    
    if(var->type != PAIR_JT) return NULL;
    
    if(scope == NULL) return 0;
    return JSON_insertDict(scope->dict, ((JSON_val**) var->data)[0], ((JSON_val**) var->data)[1]);
}

JSON_val *PSON_dereferenceValue(PSON_scope *scope, JSON_val *var){
    
    if(var->type != PAIR_JT) return NULL;
    if(scope == NULL || var == NULL) return NULL;

    if(((JSON_val**) var->data)[1] != NULL){
        if(!PSON_assignValue(scope, var)) return NULL;
        return ((JSON_val**) var->data)[1];
    }

    JSON_val *v = JSON_getDict(scope->dict, ((JSON_val**) var->data)[0]);
    if(v == NULL) return PSON_dereferenceValue(scope->par, var);

}

// ok now I have to reimplement all of this for iterables :wilted_rose:
char PSON_destroyValue(JSON_val *val){

    if(val->type == PAIR_JT){
        PSON_destroyValue(((JSON_val**) val->data)[0]);
        PSON_destroyValue(((JSON_val**) val->data)[1]);
        free(val->data);
        free(val);
        return;
    }
    

}

#endif