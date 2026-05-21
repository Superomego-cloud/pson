#include "json_dsa.h"
#ifndef  PSON_DSA_H

// convenience macro to check if something is a variable expression
#define isvar(t) (t == VAR_JT || t == FUNC_JT)

typedef struct psc PSON_scope;

// scopes work as stacks
// whenever you parse an object or call a function
// a new variable context is created
// the most recent context is taken
// thing is I now gotta completely rework the dict system
// this is gonna take some time

struct psc{
    JSON_container *dict;
    PSON_scope *par;
};

JSON_val *PSON_dereferenceValue(PSON_scope *scope, JSON_val *var){
    
    JSON_val *v = JSON_getDict(scope->dict, var);
    if(v == NULL){
        if(scope->par == NULL) return NULL;
        return PSON_dereferenceValue(scope->par, var);
    }

}


#endif