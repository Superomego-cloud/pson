#include "json_dsa.h"

#ifndef JSON_PRINT_H

void JSON_printVal(JSON_val *v){

    switch (v->type){

        case SPECIAL_JT:
            if(v == &JV_NULL) printf("null");
            else if(v == &JV_TRUE) printf("true");
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

            JSON_doubleList* lst = dct->keys_head;

            while(lst != NULL){
                
                JSON_printVal(lst->val);
                printf(":");

                JSON_printVal(JSON_getDict(dct, lst->val));
                if(lst->next != NULL) printf(", ");
                lst = lst->next;
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

#define JSON_PRINT_H
#endif