#include "json_dsa.h"

// convenience macros for telling when an element ends
#define iswhitespace(c) (c == ' ' || c == '\n' || c == '\t' || c == '\r')
#define issign(c) (c == '-' || c == '+')
#define isdigit(c) (c >= '0' && c <= '9')
#define endof(c) (c == ',' || c == ']' || c == '}')

/*
NUMBER PARSING FUNCTION

Makes sure that the entry is a valid number, then uses C stdlib to parse
Returns NULL if the number could not be parsed
*/

JSON_val JV_TRUE = {
    .type = SPECIAL_JT,
    .data = NULL,
    .len = 4
};

JSON_val JV_FALSE = {
    .type = SPECIAL_JT,
    .data = NULL,
    .len = 5
};

JSON_val JV_NULL = {
    .type = SPECIAL_JT,
    .data = NULL,
    .len = 4
};

char tr_str[] = "true";
char fa_str[] = "false";
char nu_str[] = "null";

/*
SPECIFICATION OF JSON PARSER FUNCTION:

MUST RETURN A POINTER TO A JSON VAL STRUCT
OR NULL IF THE VALUE COULD NOT BE PARSED
*/
typedef JSON_val* JSON_ParserFunc(char *data, size_t dlen);

JSON_ParserFunc JSON_parseNumeric;
JSON_ParserFunc JSON_parseString;
JSON_ParserFunc JSON_parseArray;
JSON_ParserFunc JSON_parseObject;
JSON_ParserFunc JSON_parseValue;
JSON_ParserFunc JSON_parseSpecial;

/*
NUMBER PARSING FUNCTION

Checks if the first character is a digit/sign
then parses numerical value
*/
JSON_val* JSON_parseNumeric(char *data, size_t dlen){

    if(dlen == 0) return NULL;
    size_t s = 0;

    char expo = 0, frac = 0;

    if(issign(data[0])){
        if(dlen == 1) return NULL;
        s++;
    }

    while(s < dlen){
        
        char c = data[s];
        if(endof(c) || iswhitespace(c)) break;

        if(c == 'e' || c == 'E'){

            if(expo) break;
            if(s+1 == dlen) break;

            if(!issign(data[s+1])){
                if(!isdigit(data[s+1])){
                    break;
                }
            }
            
            if(issign(data[s+1])){
                if(s+2 == dlen) break;
                s += 2;
            }
            else s++;

            expo++;
            continue;
        }

        if(c == '.'){

            if(frac || expo) break;
            
            if(s+1 == dlen) break;

            if(!issign(data[s+1])) 
                if(!isdigit(data[s+1]))
                    break;
            
            frac++;
            s++;
            continue;
        }

        if(!isdigit(c)) break;
        s++;

    }

    if(s == 0) return NULL;
    ALLOC(ret, JSON_val);

    ret->len = s;

    if(frac || expo){
        
        ALLOC(x, double);
        if(x == NULL) return NULL;

        sscanf(data, "%lf", x);
        ret->type = FLOAT_JT;
        ret->data = x;
    }
    else{

        ALLOC(x, long long);
        if(x == NULL) return NULL;

        sscanf(data, "%lld", x);
        ret->type = INTEGER_JT;
        ret->data = x;
    }

    return ret;
}

/*
STRING PARSING FUNCTION

Checks if the string terminates anywhere in the file
Returns a copy of the string
*/
JSON_val *JSON_parseString(char *data, size_t dlen){

    if(dlen == 0) return NULL;
    if(data[0] != '\"') return NULL;
    
    size_t s = 1;
    char f = 0;

    while(s < dlen){

        char c = data[s];
        
        if(c == '\\'){
            if(s+1 < dlen){
                if(data[s+1] == '\"'){
                    s += 2;
                    continue;
                }
            }
        }
        if(c == '\"'){
            s++;
            f++;
            break;
        }
        s++;
    }

    if(!f) return NULL;
    ALLOC(ret, JSON_val);
    
    if(ret != NULL){
        
        ret->type = STRING_JT;
        ret->len = s;
        
        M_ALLOC(ss, char, s+1);
        if(ss == NULL) return NULL;

        memcpy(ss, data, s*sizeof(char));
        ss[s+1] = 0;
        ret->data = ss;
    }

    return ret;

}

/*
SEQUENCE PARSING FUNCTION
Useful pattern for parsing arrays
*/
JSON_val *JSON_parseSequence(char start, char end, char* data, size_t dlen, JSON_ParserFunc parser){

    if(dlen == 0) return NULL;
    if(data[0] != start) return NULL;

    JSON_container *arr = JSON_createContainer();
    if(arr == NULL) return NULL;

    ALLOC(ret, JSON_val);
    if(ret == NULL){
        JSON_destroyContainer(arr, ARRAY_JT);
        return NULL;
    }

    ret->type = ARRAY_JT;
    ret->data = arr;

    size_t s = 1;
    JSON_val *cv = NULL;
    char sf = 1;

    while(s < dlen){

        char c = data[s];

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

            s++;
            ret->len = s;
            return ret;
        }

        if(c == ','){
            
            if(cv == NULL) break;
            if(!JSON_pushback(arr, cv)) break;

            cv = NULL;
            s++;
            sf = 0;
            continue;
        }

        if(cv != NULL) break;
        cv = parser(data + s, dlen - s);
        
        if(cv != NULL){
            s += cv->len;
            continue;
        }

        break;
    }

    JSON_destroyValue(cv);
    JSON_destroyValue(ret);
    return NULL;

}

inline JSON_val *JSON_parseArray(char *data, size_t dlen){
    return JSON_parseSequence('[', ']', data, dlen, JSON_parseValue);
}   

/*
PAIR PARSING FUNCTION, might have to rewrite this to use JSON_parseSequence instead
*/
JSON_val *JSON_parseMembers(char start, char end, char sep, char *data, size_t dlen, 
                            JSON_ParserFunc parser_first, JSON_ParserFunc parser_second){

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
        if(c == sep){
            
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
            cv = parser_second(data + s, dlen - s);
            
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

JSON_val *JSON_parseObject(char *data, size_t dlen){
    return JSON_parseMembers('{', '}', ':', data, dlen, JSON_parseString, JSON_parseValue);
}

JSON_val *JSON_parseValue(char *data, size_t dlen){

    if(dlen == 0) return NULL;

    JSON_val *ret;
    char c = data[0];

    if(c == '{') ret = JSON_parseObject(data, dlen);
    else if(c == '[') ret = JSON_parseArray(data, dlen);
    else if(c == '"') ret = JSON_parseString(data, dlen);
    else if(isdigit(c) || issign(c)) ret = JSON_parseNumeric(data, dlen);
    else ret = JSON_parseSpecial(data, dlen);

    return ret;

}

JSON_val *JSON_parseSpecial(char *data, size_t dlen){

    if(JSON_matchString(tr_str, data, 4, dlen)) return &JV_TRUE;
    if(JSON_matchString(nu_str, data, 4, dlen)) return &JV_NULL;
    if(JSON_matchString(fa_str, data, 5, dlen)) return &JV_FALSE;
    
    return NULL;
}