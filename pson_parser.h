#include "json_parser.h"
typedef struct pbp PSON_func;

char v_str[] = "var";
char f_str[] = "func";

JSON_val JV_VAR = {
    .type = SPECIAL_JT,
    .data = NULL,
    .len = 3
};

JSON_val JV_FUNC = {
    .type = SPECIAL_JT,
    .data = NULL,
    .len = 4
};

// can't use switch case here because the expressions aren't constant value
// there might be a better way to do this tbh
JSON_val *PSON_parseSpecial(char *data, size_t dlen){

    if(JSON_matchString(v_str, data, 3, dlen)) return &JV_VAR;
    if(JSON_matchString(f_str, data, 4, dlen)) return &JV_FUNC;
    if(JSON_matchString(tr_str, data, 4, dlen)) return &JV_TRUE;
    if(JSON_matchString(nu_str, data, 4, dlen)) return &JV_NULL;
    if(JSON_matchString(fa_str, data, 5, dlen)) return &JV_FALSE;
    
    return NULL;
}