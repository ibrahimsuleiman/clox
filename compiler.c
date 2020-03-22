#include<stdlib.h>
#include"scanner.h"
#include"compiler.h"


void compile(const char *src)
{       
        struct scanner s;
        init_scanner(&s, src);
        
        int line = -1;
        for(;;) {
                struct token token = scan_token(&s);
                if(line != token.line) {
                        printf("%4d ", token.line);
                        line = token.line;
                } else {
                        printf("    | ");
                }

                printf("%2d '%.*s'\n", token.type, token.length, token.start);

                if(token.type == TOKEN_EOF) break;

        }
}