#include "json_dsa.h"
#ifndef  PSON_DSA_H

JSON_val *PSON_dereferenceValue(PSON_scope *scope, JSON_val *var);

typedef struct psc PSON_scope;

struct psc{
    JSON_container *dict;
    PSON_scope *par;
};

char PSON_assignValue(PSON_scope *scope, JSON_val *var){
    
    if(var->type != JV_PAIR) return NULL;
    
    if(scope == NULL) return 0;
    return JSON_insertDict(scope->dict, ((JSON_val**) var->data)[0], ((JSON_val**) var->data)[1]);
}

JSON_val *PSON_dereferenceValue(PSON_scope *scope, JSON_val *var){
    
    if(scope == NULL || var == NULL) return NULL;

    if(((JSON_val**) var->data)[1] != NULL){
        if(!PSON_assignValue(scope, var)) return NULL;
        return ((JSON_val**) var->data)[1];
    }

    JSON_val *v = JSON_getDict(scope->dict, ((JSON_val**) var->data)[0]);
    if(v == NULL) return PSON_dereferenceValue(scope->par, var);

}

#endif