#include "json_parser.h"
#include "pson_dsa.h"

typedef struct pbp PSON_func;

JSON_val *PSON_parseValue(PSON_scope *scope, char *data, size_t dlen){
    
    JSON_val *lit = JSON_parseValue(data, dlen);
    if(lit != NULL) return lit; // if parseable JSON value then return it as is

    // if it's not a JSON literal then
    // we parse it as an object
    // we then parse it as an array
    // we finally parse it as a variable
    // if it's an assignment expression we update the current scope
    // 

};
JSON_val *PSON_parseVar(PSON_scope *scope, char *data, size_t dlen){
    
    size_t s = 0, st;
    char c = 0;
    
    while(s < dlen){
        if(iswhitespace(s)) ++s;
        else break;
    }
    
    if(s == dlen) return NULL;
    st = s;

    while(s < dlen){
        if(data[s] == '=' || iswhitespace(data[s])) break;
        ++s;
    }

    if(s == dlen) return NULL;

    ALLOC(vn, JSON_val);
    ALLOC(vs, JSON_val);
    M_ALLOC(pair, JSON_val*, 2);
    
    if(vs == NULL || pair == NULL || vn == NULL){
        free(vs);
        free(pair);
        free(vn);
        return NULL;
    }
    
    memcpy(vn, data+st, s-st);

    pair[0] = vn;
    pair[1] = NULL;

    vs->data = (void *)pair;
    vs->type = PAIR_JT;

    while(s < dlen){

        if(iswhitespace(data[s])){
            ++s;
            continue;
        }

        if(data[s] == sep){
            vs->len = s+1;
            return vs;
        }

        if(data[s] == '=') break;

    }

    if(s == dlen){
        free(pair[0]);
        free(pair);
        free(vs);
        return NULL;
    }

    while(s < dlen){
        if(iswhitespace(data[s])) ++s;
        else break;
    }

    JSON_val *var_v = PSON_parseValue(scope, data + s, dlen - s);
    if(var_v == NULL){
        free(pair[0]);
        free(pair);
        free(vs);
        return NULL;
    }

    pair[1] = var_v;
    s += var_v->len;
    vs->len = s+1;
    return vs;    

};

JSON_val *PSON_parsePairLine(PSON_scope *scope, char *data, size_t dlen){

    size_t s = 0;
    JSON_val *cache;

    ALLOC(ret, JSON_val);
    M_ALLOC(pair, JSON_val*, 2);

    pair[0] = pair[1] = NULL;

    if(ret == NULL || pair == NULL){
        free(ret);
        free(pair);
        return NULL;
    } 

    ret->type = PAIR_JT;
    ret->data = (void *) pair;

    char k = 0;

    while(s < dlen){
        
        if(iswhitespace(data[s])) ++s;
        else{

            if(k == 0){

                cache = PSON_parseValue(scope, data + s, dlen - s);
                
                if(cache != NULL){
                    if(cache->type == STRING_JT){
                        pair[0] = cache;
                        s += cache->len;
                        ++k;
                        continue;
                    }
                    JSON_destroyValue(cache);
                }

                break;
                 
            }
            else if(k == 1){

                if(data[s] != ':') break;
                ++k;
                ++s;

            }
            else{
                
                cache = PSON_parseValue(scope, data + s, dlen - s);
                if(cache == NULL) break;
                pair[1] = cache;

                return ret;
            }

        }
    }
    
    JSON_destroyValue(pair[0]);
    JSON_destroyValue(pair[1]);
    free(pair);
    free(ret);
    return NULL;
};