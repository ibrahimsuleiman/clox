#ifndef CLOX_SCANNER_H
#define CLOX_SCANNER_H

#include"common.h"

struct scanner {
        const char *start;      /* marks the beginning of current lexeme*/
        const char *current;    /*marks current token */
        int line;
};


typedef enum {

/*Single-character tokens.   */                      
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,                
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,                
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,    
  TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

  /* One or two character tokens */              
  TOKEN_BANG, TOKEN_BANG_EQUAL,                       
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,                     
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,                 
  TOKEN_LESS, TOKEN_LESS_EQUAL,                       

  /* literals */                                       
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,       

  /* Keywords. */                                       
  TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,    
  TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
  TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS, 
  TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,                 

  TOKEN_ERROR,                                        
  TOKEN_EOF  

} token_type_t;

struct token{
        token_type_t type;
        int line;
        int length;
        const char *start;
};

void init_scanner(struct scanner *s, const char *src);
struct token scan_token(struct scanner *s);

#endif