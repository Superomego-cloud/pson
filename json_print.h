#include "json_dsa.h"

void JSON_printVal(JSON_val *v){

    switch (v->type){
        case BOOL_JT:
            if(v->data == 0) printf("null");
            else if(v->data == 1) printf("true");
            else printf("false");
            break;
        case INTEGER_JT:
            printf("%lld", *((long long*)v->data));
            break;
        case FLOAT_JT:
            printf("%lf", *((double*)v->data));
            break;
        case STRING_JT:
            printf("%s", (char *) v->data);
            break;
        
        case ARRAY_JT:
            JSON_container *arr = v->data;
            printf("[");
            for(int i = 0; i < arr->val_c; ++i){
                JSON_printVal(arr->arr[i]);
                if(i < arr->val_c - 1) printf(", ");
            }
            printf("]");
            break;
        case OBJECT_JT:
            JSON_container *dct = v->data;
            printf("{");
            for(int i = 0; i < dct->size; ++i){
                JSON_dictList *cn = dct->arr[i];
                while(cn != NULL){
                    JSON_printVal(cn->key);
                    printf(": ");
                    JSON_printVal(cn->val);
                    cn = cn->next;
                    printf(",\n");
                }
            }
            printf("}");
            break;
        default:
            printf("[SPEV]");
    }
}

void JSON_printValLn(JSON_val* v){
    JSON_printVal(v);
    printf("\n");
}
