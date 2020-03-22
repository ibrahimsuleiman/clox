#include<stdlib.h>
#include<string.h>
#include"scanner.h"


void init_scanner(struct scanner *s, const char *src)
{
        s->start = src;
        s->current = src;
        s->line = 1;
}

static bool is_at_end(struct scanner *s)
{
        return s->current[-1] == '\0';
}

static struct token make_token(struct scanner *s, token_type_t type)
{
        struct token token;
        token.start = s->start;
        token.type = type;
        token.length = (s->current - s->start);
        token.line = s->line;

        return token;
}

static struct token error_token(struct scanner *s,const char * message)
{
        struct token token;

        token.line = s->line;
        token.start = message;
        token.length = strlen(message);
        token.type = TOKEN_ERROR;

        return token;
}

static inline char advance(struct scanner *s)
{
        s->current++;
        return s->current[-1];
}


static bool match(struct scanner *s, char c)
{
        if(is_at_end(s)) return false;

        if(s->current[0] != c) return false;

        s->current++;
        return true;
}

static inline char peek(struct scanner *s)
{
        return s->current[0];
}

static char peek_next(struct scanner *s)
{
        if(is_at_end(s)) return '\0';
        return s->current[1];
}

static inline bool is_digit(char c)
{
        return c >= '0' && c <= '9';
}

static bool is_alpha(char c)
{
        return  (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c == '_');
}

static void skip_white_space_and_comments(struct scanner *s)
{
        for(;;) {
                char c = peek(s);
                switch(c){
                
                case '\n':
                        s->line++;
                        advance(s);
                        break;
                case ' ':
                case '\t':
                case '\r':
                        advance(s);
                        break;
                case '/':
                        if(peek_next(s) == '/') {
                                while(!is_at_end(s) && peek(s) != '\n') advance(s);
                        } else {
                                return;
                        }
                        break;
                default:
                        return;
                }
        }
}


static struct token string(struct scanner *s)
{
        while(!is_at_end(s) && peek(s) != '"') {
                if(peek(s) == '\n') s->line++;
                advance(s);
        };

        if(is_at_end(s)) return error_token(s, "Unterminated string.");

        advance(s); /* consume closing " */ 
        return make_token(s, TOKEN_STRING);

}

static struct token number(struct scanner *s)
{
        while(is_digit(peek(s))) advance(s);

        if(peek(s) == '.'){
                advance(s);
                while(is_digit(peek(s))) advance(s);
        }

        return make_token(s, TOKEN_NUMBER);
}


static token_type_t check_keyword(
        struct scanner *s, int start, int len, 
        const char *rest, token_type_t type)
{       
        int r = memcmp(s->start + start, rest, len);
        if((s->current - s->start) == (start + len) &&
           r == 0)
                return type;
        
        return TOKEN_IDENTIFIER;
}

static token_type_t identifier_type(struct scanner *s)
{       
 
        switch(s->start[0]){
        
        case 'a': 
                return check_keyword(s, 1, 2, "nd", TOKEN_AND);
        case 'c':
                return check_keyword(s, 1, 4, "lass", TOKEN_CLASS);
        case 'e':
                return check_keyword(s, 1, 3, "lse", TOKEN_ELSE);
        case 'i':
                return check_keyword(s, 1, 1, "f", TOKEN_IF);
        case 'n':
                return check_keyword(s, 1, 2, "il", TOKEN_NIL);
        case 'o':
                return check_keyword(s, 1, 1, "r", TOKEN_OR);
        case 'p':
                return check_keyword(s, 1, 4, "rint", TOKEN_PRINT);
        case 'r':
                return check_keyword(s, 1, 5, "eturn", TOKEN_RETURN);
        case 's':
                return check_keyword(s, 1, 4, "uper", TOKEN_SUPER);
        case 'v':
                return check_keyword(s, 1, 2, "ar", TOKEN_VAR);
        case 'w':
                return check_keyword(s, 1, 4, "hile", TOKEN_WHILE);
        
        case 'f':
                if(s->current - s->start > 1) {
                        switch(s->start[1]) {
                        case 'a':
                                return check_keyword(s, 2, 3, "lse", TOKEN_FALSE);
                        case 'u':
                                return check_keyword(s, 2, 1, "n", TOKEN_FUN);
                        case 'o':
                                return check_keyword(s, 2, 1, "r", TOKEN_FOR);
                        }
                }
                break;
        case 't':
                if(s->current - s->start > 1) {
                        switch(s->start[1]) {
                        case 'r':
                                return check_keyword(s, 2, 2, "ue", TOKEN_TRUE);      
                        case 'h':
                                return check_keyword(s, 2, 2, "is", TOKEN_THIS);
                        }
                }
                break;

        }
 
        return TOKEN_IDENTIFIER;
}

static struct token identifier(struct scanner *s)
{
        while(is_alpha(peek(s)) || is_digit(peek(s))) advance(s);

        return make_token(s, identifier_type(s));
}

struct token scan_token(struct scanner *s)
{ 
        skip_white_space_and_comments(s);

        s->start = s->current;

        char c = advance(s);

        switch(c) {
        
        case '(':
                return make_token(s,TOKEN_LEFT_PAREN);
        case ')':
                return make_token(s, TOKEN_RIGHT_PAREN);
        case '{':
                return make_token(s, TOKEN_LEFT_BRACE);
        case '}':
                return make_token(s, TOKEN_RIGHT_BRACE);
        case '+':
                return make_token(s, TOKEN_PLUS);
        case '-':
                return make_token(s, TOKEN_MINUS);
        case '*':
                return make_token(s, TOKEN_STAR);
        case '/':
                return make_token(s, TOKEN_SLASH);
        case ';':
                return make_token(s, TOKEN_SEMICOLON);
        case '.':
                return make_token(s, TOKEN_DOT);
        case ',':
                return make_token(s, TOKEN_COMMA);
        case '=':
                return make_token(s, match(s, '=') ? TOKEN_EQUAL_EQUAL: TOKEN_EQUAL);
        case '!':
                return make_token(s, match(s, '=') ? TOKEN_BANG_EQUAL: TOKEN_BANG);
        case '<':
                return make_token(s, match(s, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
                return make_token(s, match(s, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"':
                return string(s);
        }

        if(is_at_end(s)) return make_token(s, TOKEN_EOF);
        if(is_alpha(c)) return identifier(s);
        if(is_digit(c)) return number(s);

        return error_token(s, "Unxpected character.");
}