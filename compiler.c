#include<stdlib.h>
#include<stdio.h>
#include"scanner.h"
#include"compiler.h"
#include"value.h"


#ifdef DEBUG_PRINT_CODE
#include"debug.h"
#endif

struct parser{
        struct token previous;
        struct token current;
        bool had_error;
        bool panic_mode;
};


struct parser parser;
struct chunk *compiling_chunk;

typedef enum {
        PREC_NONE,
        PREC_ASSIGNMENT,        /* = */
        PREC_OR,                /* or */
        PREC_AND,               /* and */
        PREC_EQUALITY,          /* ==, != */
        PREC_COMPARISON,        /* <, <=, >, >= */
        PREC_TERM,              /* +, - */
        PREC_FACTOR,            /* *, / */
        PREC_UNARY,             /*-, ! */
        PREC_CALL,              /* () */
        PREC_PRIMARY

} precedence_t;

typedef void (*parse_fn)();

struct parse_rule {
        parse_fn prefix;
        parse_fn infix;
        precedence_t precedence;
};


static void expression();
static struct parse_rule *get_rule(token_type_t type);
static void parse_precedence(precedence_t precedence);


static struct chunk *current_chunk()
{
        return compiling_chunk;
}

static void emit_byte(uint8_t byte)
{
        write_chunk(current_chunk(), byte, parser.previous.line);
}

static void emit_2_bytes(uint8_t byte1, uint8_t byte2)
{
        emit_byte(byte1);
        emit_byte(byte2);
}

static void emit_3_bytes(uint8_t byte1, uint8_t byte2, uint8_t byte3)
{
        emit_2_bytes(byte1, byte2);
        emit_byte(byte3);
}

static void error_at(struct token *t, const char * msg)
{
        if(parser.panic_mode)
                return;

        parser.panic_mode = true;

        fprintf(stderr, "[line %d], Error ", t->line);
        if(t->type == TOKEN_EOF){
                fprintf(stderr," at end");
        } else if(t->type == TOKEN_ERROR){

        } else {
                fprintf(stderr, " at '%.*s'", t->length, t->start);
        }

        fprintf(stderr, " : %s\n", msg);
        parser.had_error = true;
}

static void error_at_current(const char * msg)
{
        error_at(&parser.current, msg);
}

static void error(const char *msg)
{
        error_at(&parser.previous, msg);
}

static int make_constant(value_t val)
{
        int c = add_constant(current_chunk(), val);

        if(c > UINT8_MAX) {
                /* undo the last byte written : OP_CONSTANT*/
                undo_last_write_to_chunk(current_chunk());
                /* remove the constant*/
                undo_previous_write(&(current_chunk()->constants));
                /*re-write as a long constant*/
                c = write_constant(current_chunk(), val, parser.previous.line);

                if(c > MAX_CONST_INDEX) {
                        error("Too many constants in one chunk.");
                }
        } else {
                emit_byte(c);
        }

        return c;
}

static void emit_constant(double val)
{
        emit_byte(OP_CONSTANT);
        make_constant(NUMBER(val));     /* second byte emitted from make_constant*/
}


static void emit_return()
{
        emit_byte(OP_RETURN);
}

static void advance()
{
        parser.previous = parser.current;

        for(;;){
                parser.current = scan_token();
                if(parser.current.type != TOKEN_ERROR)
                        break;


                error_at_current(parser.current.start);
        }
}

static void consume(token_type_t type, const char *msg)
{
        if(parser.current.type == type) {
                advance();
                return;
        }

        error_at_current(msg);
}


static void binary()
{
        token_type_t op = parser.previous.type;
        struct parse_rule *rule = get_rule(op);
        parse_precedence(rule->precedence + 1);

        switch(op) {
                case TOKEN_PLUS:        emit_byte(OP_ADD); break;
                case TOKEN_MINUS:       emit_byte(OP_SUB); break;
                case TOKEN_SLASH:       emit_byte(OP_DIV); break;
                case TOKEN_STAR:        emit_byte(OP_MULT); break;
                default:
                        return;

        }
}

static void grouping()
{
        expression();
        consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

static void expression()
{
       parse_precedence(PREC_ASSIGNMENT);
}

static void unary()
{
        token_type_t op = parser.previous.type;

        parse_precedence(PREC_UNARY);

        switch(op) {
                case TOKEN_MINUS:
                        emit_byte(OP_NEGATE);
                        break;
                case TOKEN_BANG:
                        emit_byte(OP_NOT);
                        break;
                default:
                        return;
        }
}

static void number()
{
        double val = strtod(parser.previous.start, NULL);
        emit_constant(val);
}

static void literal()
{
        switch(parser.previous.type) {
                case TOKEN_FALSE:
                        emit_byte(OP_FALSE);
                        break;
                case TOKEN_TRUE:
                        emit_byte(OP_TRUE);
                        break;
                case TOKEN_NIL:
                        emit_byte(TOKEN_NIL);
                        break;
                default:
                        return;
        }
}

struct parse_rule rules[] = {
  { grouping, NULL,    PREC_NONE },       /* TOKEN_LEFT_PAREN         */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_RIGHT_PAREN        */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_LEFT_BRACE         */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_RIGHT_BRACE        */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_COMMA              */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_DOT                */
  { unary,    binary,  PREC_TERM },       /* TOKEN_MINUS              */
  { NULL,     binary,  PREC_TERM },       /* TOKEN_PLUS               */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_SEMICOLON          */
  { NULL,     binary,  PREC_FACTOR },     /* TOKEN_SLASH              */
  { NULL,     binary,  PREC_FACTOR },     /* TOKEN_STAR               */
  { unary,     NULL,    PREC_NONE },       /* TOKEN_BANG               */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_BANG_EQUAL         */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_EQUAL              */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_EQUAL_EQUAL        */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_GREATER            */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_GREATER_EQUAL      */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_LESS               */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_LESS_EQUAL         */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_IDENTIFIER         */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_STRING             */
  { number,   NULL,    PREC_NONE },       /* TOKEN_NUMBER             */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_AND                */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_CLASS              */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_ELSE               */
  { literal,     NULL,    PREC_NONE },       /* TOKEN_FALSE              */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_FOR                */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_FUN                */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_IF                 */
  { literal,     NULL,    PREC_NONE },       /* TOKEN_NIL                */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_OR                 */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_PRINT              */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_RETURN             */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_SUPER              */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_THIS               */
  { literal,     NULL,    PREC_NONE },       /* TOKEN_TRUE               */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_VAR                */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_WHILE              */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_ERROR              */
  { NULL,     NULL,    PREC_NONE },       /* TOKEN_EOF                */
};


static void end_compiler()
{
        emit_return();

#ifdef DEBUG_PRINT_CODE
        if(!parser.had_error) {
                disassemble_chunk(current_chunk(), "code");
        }
#endif
}

static struct parse_rule *get_rule(token_type_t type)
{
        return &rules[type];
}

static void parse_precedence(precedence_t prec)
{
        advance();
        parse_fn prefix_rule = get_rule(parser.previous.type)->prefix;

        if(prefix_rule == NULL) {
                error("Expected expression");
                return;
        }

        prefix_rule();


        while (prec <= get_rule(parser.current.type)->precedence) {
                advance();
                parse_fn infix_rule = get_rule(parser.previous.type)->infix;
                infix_rule();
        }
}


bool compile(const char *src, struct chunk *chunk)
{
        parser.had_error = false;
        parser.panic_mode = false;
        compiling_chunk = chunk;

        init_scanner(src);
        advance();
        expression();
        consume(TOKEN_EOF, "Expected end of expression");

        end_compiler();

        return !parser.had_error;
}
