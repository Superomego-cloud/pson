#include "json/json_parser.h"
#include "pson_dsa.h"

typedef struct pbp PSON_func;
typedef JSON_val* PSON_parserfunc(PSON_scope *scope, char *data, size_t dlen);

JSON_val *PSON_parseValue(PSON_scope *scope, char *data, size_t dlen){

    if(dlen == 0) return NULL;

    JSON_val *val = JSON_parseValue(data, dlen);
    if(val != NULL) return val;

    if(data[0] == '[') return PSON_parseArray(scope, data, dlen);
    if(data[0] == '{') return PSON_parseObject(scope, data, dlen);

}

JSON_val *PSON_parseVar(PSON_scope *scope, char end, char *data, size_t dlen){
    
    size_t s = 0, st;
    char c = 0;
    
    while(s < dlen){
        if(iswhitespace(s)) ++s;
        else break;
    }
    
    if(s == dlen) return NULL;
    st = s;

    while(s < dlen){
        if(data[s] == '=' || data[s] == end || iswhitespace(data[s])) break;
        ++s;
    }

    if(s == dlen) return NULL;

    ALLOC(vn, JSON_val);
    ALLOC(vs, JSON_val);
    M_ALLOC(pair, JSON_val*, 2);
    M_ALLOC(buf, char, s-st);
    
    if(vs == NULL || pair == NULL || vn == NULL){
        free(vs);
        free(pair);
        free(vn);
        return NULL;
    }
    
    memcpy(buf, data+st, s-st);
    vn->type = VAR_JT;
    vn->data = buf;
    vn->len = s-st;
    
    pair[0] = vn;
    pair[1] = NULL;

    vs->data = (void *)pair;
    vs->type = PAIR_JT;

    while(s < dlen){

        if(iswhitespace(data[s])){
            ++s;
            continue;
        }

        if(data[s] == '='){
            ++s;
            break;
        }

        if(data[s] == end){
            vs->len = s+1;
            return vs;
        }
        
    }

    if(s == dlen){

        JSON_destroyValue(pair[0]);
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
        
        free(buf);
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

JSON_val *PSON_parseObject(PSON_scope *scope, char *data, size_t dlen){
    
    if(dlen == 0) return NULL;
    if(data[0] != '{') return NULL;

    JSON_container *dct = JSON_createContainer();
    if(dct == NULL) return NULL;

    
    ALLOC(ret, JSON_val);
    if(ret == NULL){
        JSON_destroyContainer(dct, OBJECT_JT);
        return NULL;
    }

    ret->type = OBJECT_JT;
    ret->data = dct;

    size_t s = 1;
    JSON_val *ck = NULL, *cv = NULL;
    char kf = 1, sf = 1;

    while(s < dlen){

        char c = data[s];

        if(iswhitespace(c)){
            s++;
            continue;
        }
        
        if(c == '}'){
            
            if(ck == NULL || cv == NULL){
                if(!sf) break;
                if(ck != cv) break;
            }
            else{
                JSON_insertDict(dct, ck, cv);
            }
            
            s++;
            ret->len = s;
            return ret;
        }
        if(c == ','){
            
            if(ck == NULL || cv == NULL){
                break;
            }

            JSON_insertDict(dct, ck, cv);
            s++;

            sf = 0;
            kf = 1;
            ck = cv = NULL;
            continue;
        }
        if(c == ':'){
            
            if(ck == NULL) break;
            if(cv != NULL) break;
           
            kf = 0;
            s++;
            continue;

        }

        if(kf){ 
            
            if(ck != NULL) break;
            
            if(c == '"') ck = JSON_parseString(data + s, dlen - s);

            if(ck != NULL){
                s += ck->len;
                continue;
            }

            JSON_val *var = PSON_parseVar(scope, ':', data + s, dlen - s);
            if(var != NULL){
                var = PSON_dereferenceValue(scope, var);
                if(var->type != STRING_JT) break; 
            }

            break;
            
        }
        else{
            
            if(cv != NULL) break;
            cv = PSON_parseValue(scope, data + s, dlen - s);
            
            if(cv != NULL){
                s += cv->len;
                continue;
            }
            break;

        }

    }

    JSON_destroyValue(ck);
    JSON_destroyValue(cv);
    JSON_destroyValue(ret);
    return NULL;

}

JSON_val *PSON_parseArray(PSON_scope *scope, char *data, size_t dlen){
    
}