#include "analyzer.h"
#include <string.h>

int is_keyword(const char *word) {
    const char *keys[] = {
        "int","float","char","double","void","if","else","for","while",
        "do","return","break","continue","switch","case","default","struct",
        "long","short","unsigned","signed","const","static", NULL
    };
    for (int i = 0; keys[i]; i++) {
        if (strcmp(word, keys[i]) == 0) {
            return 1;
        }
    }
    return 0;
}
