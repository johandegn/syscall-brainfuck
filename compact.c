#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main(int argc, char **argv) {
    FILE *in = fopen(argv[1], "r");
    FILE *out = fopen(argv[2], "w");
    int count = 0;
    char c;
    while ((c = fgetc(in)) != EOF) {
        switch (c) {
        case '>':
        case '<':
        case '+':
        case '-':
        case '[':
        case ']':
        case '&':
        case '!':
            fputc(c, out);
            if (++count == 80) {
                fputc('\n', out);
                count = 0;
            }
            break;
        }
        
    }
    fputc('\n', out);
    fclose(in);
    fclose(out);
    return 0;
}
