#include "json_parser.h"
typedef struct pbp PSON_func;

struct pbp{
    JSON_container *vars; // every time you call the function
    JSON_dictList *attr;
};

JSON_container vars;

char var_str[] = "var";
char vars_str[] = "varspace";

JSON_val *PSON_parseVar(){
   
}


JSON_val *PSON_declareVarspace(char *data, size_t dlen){
    
}

// function that returns the value of the variable within the expression 
JSON_val *PSON_derefVar(char *data, size_t dlen){
    
};

JSON_val *PSON_parseArgDef(){
   
}

void *PSON_printVal(JSON_val *v){
    // prints value of variable
    // otherwise prints attributes associated to constructor
}