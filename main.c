#include "json/json_parser.h"
#include "json/json_print.h"

int main(int argc, char** argv){

    if(argc != 2){
        printf("Wrong amount of arguments. Please only pass the file path of your JSON.");
        return 0;
    }

    FILE *f = fopen(argv[1], "r");
    
    if(f == NULL){
        fprintf(stderr, "File \"%s\" could not be opened. Are you sure this file exists?\n", argv[1]);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(len);
    fread(buf, sizeof(char), len, f);

    JSON_val *main_object = JSON_parseValue(buf, len);

    if(main_object == NULL){
        fprintf(stderr, "There was an error parsing the file\n");
        return -1;
    }

    JSON_printValLn(main_object);
    return 0;
}