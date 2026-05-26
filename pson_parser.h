#include "json/json_parser.h"
#include "json/json_print.h"
#include "pson_dsa.h"

typedef JSON_val* PSON_ParserFunc(PSON_scope *scope, char *data, size_t dlen);

JSON_val *PSON_parseVar(PSON_scope *scope, char *data, size_t dlen);
PSON_ParserFunc PSON_parseArray;
PSON_ParserFunc PSON_parseObject;
PSON_ParserFunc PSON_parseValue;

// switch basically lets me use hashing to verify
char PSON_validateChar(char c){
    switch (c){
        case '{': return 0;
        case '}': return 0;
        case '[': return 0;
        case ']': return 0;
        case ',': return 0;
        case ';': return 0;
        case ':': return 0;
        default:  return 1;
    }
}

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
        if(data[s] == '=' || !PSON_validateChar(data[s]) || iswhitespace(data[s])) break;
        ++s;
    }

    if(s == dlen) return NULL;
    
    ALLOC(vn, JSON_val);
    ALLOC(vs, JSON_val);
    M_ALLOC(pair, JSON_val*, 2);
    M_ALLOC(buf, char, s-st+1);
    
    if(vs == NULL || pair == NULL || vn == NULL){
        free(vs);
        free(pair);
        free(vn);
        return NULL;
    }
    

    memcpy(buf, data+st, s-st);
    buf[s-st] = 0;
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

        if(!PSON_validateChar(data[s])){
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

    s += var_v->len;
    if(var_v->type == PAIR_JT){
        var_v = PSON_dereferenceValue(scope, var_v);
        if(var_v == NULL){
            free(buf);
            free(pair[0]);
            free(pair);
            free(vs);
            return NULL;
        }
    }

    pair[1] = var_v;
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
            if(cv->type == PAIR_JT){
                cv = PSON_dereferenceValue(local, cv);
                if(cv == NULL) break;
            }
            continue;
        }

        break;
    }

    JSON_destroyValue(cv);
    JSON_destroyValue(ret);
    PSON_destroyScope(local);
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
                            size_t dlen, PSON_ParserFunc parser_first, PSON_ParserFunc parser_second){

    if(dlen == 0) return NULL;
    if(data[0] != start) return NULL;

    JSON_container *dct = JSON_createContainer();
    PSON_scope *local = PSON_createScope(scope);
    
    ALLOC(ret, JSON_val);
    if(dct == NULL || ret == NULL || local == NULL){
        PSON_destroyScope(local);
        JSON_destroyContainer(dct, OBJECT_JT);
        free(ret);
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
            PSON_destroyScope(local);
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
            ck = parser_first(local, data + s, dlen - s);

            if(ck != NULL){

                s += ck->len;
                if(ck->type == PAIR_JT){
                    ck = PSON_dereferenceValue(local, ck);
                    if(ck == NULL) break;
                }

                while(s < dlen){
                    if(iswhitespace(data[s])) ++s;
                    else break;
                }

                if(s != dlen){

                    if(data[s] == ';'){
                        s++;
                        ck = NULL;
                        continue;
                    }
                    if(data[s] == pair){
                        if(ck->type != STRING_JT) break;
                        continue;
                    }
                }

                continue;
            }

            break;
            
        }
        else{
            
            if(cv != NULL) break;
            cv = parser_second(local, data + s, dlen - s);
            
            if(cv != NULL){
                s += cv->len;
                if(cv->type == PAIR_JT){
                    cv = PSON_dereferenceValue(local, cv);
                    if(cv == NULL) break;
                }
                continue;
            }
            break;

        }

    }

    JSON_destroyValue(ck);
    JSON_destroyValue(cv);
    JSON_destroyContainer(dct, OBJECT_JT);
    PSON_destroyScope(local);
    return NULL;

}

/*
OBJECT PARSING FUNCTION

Parses a JSON object. Pretty straightforward.
*/
inline JSON_val *PSON_parseObject(PSON_scope *scope, char *data, size_t dlen){
    return PSON_parseMembers(scope, '{', '}', ':', ',',
                             data, dlen, PSON_parseValue, PSON_parseValue);
}

JSON_val *PSON_parseValue(PSON_scope *scope, char *data, size_t dlen){

    if(dlen == 0) return NULL;

    JSON_val *ret;
    char c = data[0];

    if(c == '{') ret = PSON_parseObject(scope, data, dlen);
    else if(c == '[') ret = PSON_parseArray(scope, data, dlen);
    else if(c == '"') ret = JSON_parseString(data, dlen);
    else if(isdigit(c) || issign(c)) ret = JSON_parseNumeric(data, dlen);
    else{
        ret = JSON_parseSpecial(data, dlen);
        if(ret == NULL) ret = PSON_parseVar(scope, data, dlen);
    }


    return ret;

}
