#include "json/json_parser.h"
#include "json/json_print.h"
#include "pson_dsa.h"

typedef JSON_val* PSON_ParserFunc(PSON_scope *scope, char *data, size_t dlen);

JSON_val *PSON_parseVar(PSON_scope *scope, char end, char *data, size_t dlen);
PSON_ParserFunc PSON_parseArray;
PSON_ParserFunc PSON_parseObject;
PSON_ParserFunc PSON_parseValue;

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
            vs->len = s;
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
    vs->len = s;
    return vs;

};

JSON_val *PSON_parseSequence(PSON_scope *scope, char start, char end, char sep, char* data, 
                             size_t dlen, PSON_ParserFunc parser){
    
    if(dlen == 0) return NULL;
    if(data[0] != start) return NULL;

    JSON_container *arr = JSON_createContainer();
    PSON_scope *local = PSON_createScope(scope);
    ALLOC(ret, JSON_val);

    if(arr == NULL || ret == NULL || local == NULL){
        
        PSON_destroyScope(local);
        JSON_destroyContainer(arr, ARRAY_JT);
        free(ret);

        return NULL;
    }

    ret->type = ARRAY_JT;
    ret->data = arr;

    size_t s = 1;
    JSON_val *cv = NULL;
    char sf = 1, c;

    while(s < dlen){

        c = data[s];

        if(iswhitespace(c)){
            s++;
            continue;
        }

        if(c == end){

            if(cv == NULL){
                if(!sf) break;
            }
            else{   
                if(!JSON_pushback(arr, cv)) break;
                if(!JSON_resize(arr, ARRAY_JT, arr->val_c)) break;
            }

            PSON_destroyScope(local);
            ret->len = s+1;
            return ret;
        }

        if(c == sep){
            
            if(cv == NULL) break;
            if(!JSON_pushback(arr, cv)) break;

            cv = NULL;
            s++;
            sf = 0;
            continue;
        }

        if(cv != NULL) break;
        cv = parser(local, data + s, dlen - s);

        if(cv != NULL){
            s += cv->len;
            continue;
        }

        cv = PSON_parseVar(local, sep, data + s, dlen - s);

        if(cv != NULL){
            s += cv->len;
            cv = PSON_dereferenceValue(local, cv);
            if(cv != NULL){
                continue;
            } 
        } 
        break;
    }

    PSON_destroyScope(local);
    JSON_destroyValue(cv);
    JSON_destroyValue(ret);
    return NULL;

}

inline JSON_val *PSON_parseArray(PSON_scope *scope, char *data, size_t dlen){
    return PSON_parseSequence(scope, '[', ']', ',', data, dlen, PSON_parseValue);
}

/*
PAIR parsing function

Returns a dictionary whose key/value pairs
are the entries described
*/
JSON_val *PSON_parseMembers(PSON_scope *scope, char start, char end, char pair, char sep, char *data, 
                            size_t dlen, JSON_ParserFunc parser_first, PSON_ParserFunc parser_second){

    if(dlen == 0) return NULL;
    if(data[0] != start) return NULL;

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
        
        if(c == end){
            
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
        if(c == sep){
            
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
        if(c == pair){
            
            if(ck == NULL) break;
            if(cv != NULL) break;
           
            kf = 0;
            s++;
            continue;

        }

        if(kf){ 
            
            if(ck != NULL) break;
            
            if(c == '"') ck = parser_first(data + s, dlen - s);

            if(ck != NULL){
                s += ck->len;
                continue;
            }

            break;
            
        }
        else{
            
            if(cv != NULL) break;
            cv = parser_second(scope, data + s, dlen - s);
            
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

/*
OBJECT PARSING FUNCTION

Parses a JSON object. Pretty straightforward.
*/
inline JSON_val *PSON_parseObject(PSON_scope *scope, char *data, size_t dlen){
    return PSON_parseMembers(scope, '{', '}', ':', ',',
                             data, dlen, JSON_parseString, PSON_parseValue);
}

JSON_val *PSON_parseValue(PSON_scope *scope, char *data, size_t dlen){

    if(dlen == 0) return NULL;

    JSON_val *ret;
    char c = data[0];

    if(c == '{') ret = PSON_parseObject(scope, data, dlen);
    else if(c == '[') ret = PSON_parseArray(scope, data, dlen);
    else if(c == '"') ret = JSON_parseString(data, dlen);
    else if(isdigit(c) || issign(c)) ret = JSON_parseNumeric(data, dlen);
    else ret = JSON_parseSpecial(data, dlen);

    return ret;

}
