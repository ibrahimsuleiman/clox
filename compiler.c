#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "scanner.h"
#include "compiler.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

struct parser {
	struct token previous;
	struct token current;
	bool had_error;
	bool panic_mode;
};

struct parser parser;
struct chunk *compiling_chunk;

typedef enum {
	PREC_NONE,
	PREC_ASSIGNMENT, /* = */
	PREC_OR, /* or */
	PREC_AND, /* and */
	PREC_EQUALITY, /* ==, != */
	PREC_COMPARISON, /* <, <=, >, >= */
	PREC_TERM, /* +, - */
	PREC_FACTOR, /* *, / */
	PREC_UNARY, /*-, ! */
	PREC_CALL, /* () */
	PREC_PRIMARY

} precedence_t;

typedef void (*parse_fn)(bool can_assign);

struct parse_rule {
	parse_fn prefix;
	parse_fn infix;
	precedence_t precedence;
};

struct local {
	struct token name;
	int depth;
};

typedef enum {
	TYPE_FUNCTION,
	TYPE_SCRIPT,
} function_type_t;

struct compiler {
	struct compiler *enclosing; /* linked-list of compilers for each nested
				       function - nodes are not dynamically
				       allocated*/
	obj_function_t *function; /* currently compiling function */
	function_type_t type; /* script or actual function*/
	struct local locals[UINT8_COUNT];
	int local_count;
	int scope_depth;
};

struct compiler *current = NULL;

static void expression();
static void statement();
static void declaration();
static void var_declaration();
static struct parse_rule *get_rule(token_type_t type);
static void parse_precedence(precedence_t precedence);

static void end_scope();
static void begin_scope();

static struct chunk *current_chunk()
{
	return &current->function->chunk;
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

static int emit_jump(uint8_t instruction)
{
	emit_3_bytes(instruction, 0xff, 0xff);
	return current_chunk()->count - 2;
}

static void error_at(struct token *t, const char *msg)
{
	if (parser.panic_mode)
		return;

	parser.panic_mode = true;

	fprintf(stderr, "[line %d], Error ", t->line);
	if (t->type == TOKEN_EOF) {
		fprintf(stderr, " at end");
	} else if (t->type == TOKEN_ERROR) {
	} else {
		fprintf(stderr, " at '%.*s'", t->length, t->start);
	}

	fprintf(stderr, " : %s\n", msg);
	parser.had_error = true;
}

static void error_at_current(const char *msg)
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

#if 0
	if (c > UINT8_MAX) {
		/* undo the last byte written : OP_CONSTANT*/
		undo_last_write_to_chunk(current_chunk());
		/* remove the constant*/
		undo_previous_write(&(current_chunk()->constants));
		/*re-write as a long constant*/
		c = write_constant(current_chunk(), val, parser.previous.line);

		if (c > MAX_CONST_INDEX) {
			error("Too many constants in one chunk.");
		}
	} else {
		
		emit_byte(c);
		
	}
#endif

	if (c > UINT8_MAX) {
		error("Too many constants in one chunk");
	}
	return c;
}

static void emit_constant(value_t val)
{
	emit_byte(OP_CONSTANT);
	emit_byte(make_constant(val)); /* second byte emitted from
					  make_constant*/
}

static void emit_loop(int loopstart)
{
	emit_byte(OP_LOOP);
	int offset = current_chunk()->count - loopstart + 2;
	if (offset > UINT16_MAX)
		error("Loop body too large.");

	emit_byte((offset >> 8) & 0xff);
	emit_byte(offset & 0xff);
}

static void back_patch_jump(int offset)
{
	/*-2 to adjust for jmp instruction's operands*/
	int jmp = current_chunk()->count - offset - 2;

	if (jmp > UINT16_MAX)
		error("Too much code to jump over.");

	current_chunk()->code[offset] = (jmp >> 8) & 0xff;
	current_chunk()->code[offset + 1] = (jmp)&0xff;
}

static void emit_return()
{
	emit_byte(OP_NIL); /*functions without a return stmt implicitly return
			      nil*/
	emit_byte(OP_RETURN);
}

static void init_compiler(struct compiler *compiler, function_type_t type)
{
	/* capture the current surrounding function's compiler*/
	compiler->enclosing = current;

	compiler->function = NULL;
	compiler->type = type;

	compiler->scope_depth = 0;
	compiler->function = new_function();
	compiler->local_count = 0;
	current = compiler;

	/* claim stack slot 0 for vm's internal use*/
	struct local *local = &current->locals[current->local_count++];
	local->depth = 0;
	local->name.start = "";
	local->name.length = 0;

	if (type != TYPE_SCRIPT)
		current->function->name = copy_string(parser.previous.start,
						      parser.previous.length);
}

static obj_function_t *end_compiler()
{
	emit_return();
	obj_function_t *func = current->function;

#ifdef DEBUG_PRINT_CODE
	if (!parser.had_error) {
		disassemble_chunk(current_chunk(), func->name != NULL ?
							   func->name->chars :
							   "<script>");
	}
#endif

	/* restore the surrounding function's compiler*/
	current = current->enclosing;
	return func;
}

static void advance()
{
	parser.previous = parser.current;

	for (;;) {
		parser.current = scan_token();
		if (parser.current.type != TOKEN_ERROR)
			break;

		error_at_current(parser.current.start);
	}
}

static void consume(token_type_t type, const char *msg)
{
	if (parser.current.type == type) {
		advance();
		return;
	}

	error_at_current(msg);
}

static bool check(token_type_t type)
{
	return parser.current.type == type;
}

static bool match(token_type_t type)
{
	if (!check(type))
		return false;
	advance();
	return true;
}

static void binary(bool can_assign)
{
	token_type_t op = parser.previous.type;
	struct parse_rule *rule = get_rule(op);
	parse_precedence(rule->precedence + 1);

	switch (op) {
	case TOKEN_PLUS:
		emit_byte(OP_ADD);
		break;
	case TOKEN_MINUS:
		emit_byte(OP_SUB);
		break;
	case TOKEN_SLASH:
		emit_byte(OP_DIV);
		break;
	case TOKEN_STAR:
		emit_byte(OP_MULT);
		break;
	case TOKEN_EQUAL_EQUAL:
		emit_byte(OP_EQUAL);
		break;
	case TOKEN_BANG_EQUAL:
		emit_2_bytes(OP_EQUAL, OP_NOT);
		break;
	case TOKEN_GREATER:
		emit_byte(OP_GREATER);
		break;
	case TOKEN_GREATER_EQUAL:
		emit_2_bytes(OP_LESS, OP_NOT);
		break;
	case TOKEN_LESS:
		emit_byte(OP_LESS);
		break;
	case TOKEN_LESS_EQUAL:
		emit_2_bytes(OP_GREATER, OP_NOT);
		break;
	default:
		return;
	}
}

static void grouping(bool can_assign)
{
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

static void expression()
{
	parse_precedence(PREC_ASSIGNMENT);
}

static void block()
{
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
		declaration();
	}

	consume(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
}

static void expression_statement()
{
	expression();
	consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
	emit_byte(OP_POPX);
}

static void if_statement()
{
	consume(TOKEN_LEFT_PAREN, "Expected '(' after 'if'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");

	int then_jmp = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP); /* pop condition off stack - start of then branch*/
	statement();
	int else_jmp = emit_jump(OP_JUMP);
	back_patch_jump(then_jmp);

	emit_byte(OP_POP); /* pop condition off stack - start of else branch*/

	if (match(TOKEN_ELSE))
		statement();
	back_patch_jump(else_jmp);
}

static void while_statement()
{
	/* right before the condition */
	int loopstart = current_chunk()->count;

	consume(TOKEN_LEFT_PAREN, "Expected '(' after 'while'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");

	int exitjump = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP); /* path 1*/
	statement();

	emit_loop(loopstart);

	back_patch_jump(exitjump);
	emit_byte(OP_POP); /* path 2*/
}

static void for_statement()
{
	begin_scope();

	consume(TOKEN_LEFT_PAREN, "Expected ')' after 'for'.");

	if (match(TOKEN_SEMICOLON)) {
	} else if (match(TOKEN_VAR)) {
		var_declaration();
	} else {
		expression_statement(); /* emits OP_POP, and checks for ';' */
	}

	int loopstart = current_chunk()->count;

	int exitjump = -1;

	if (!match(TOKEN_SEMICOLON)) {
		expression();
		consume(TOKEN_SEMICOLON, "Expected ';'.");

		/* jump out of loop if false*/
		exitjump = emit_jump(OP_JUMP_IF_FALSE);
		emit_byte(OP_POP); /* pop the condition*/
	}

	if (!match(TOKEN_RIGHT_PAREN)) { /* there's an increment clause*/

		int bodyjump = emit_jump(OP_JUMP);
		int increment_start = current_chunk()->count;

		expression();
		emit_byte(OP_POP);

		consume(TOKEN_RIGHT_PAREN, "Expected ')' after 'for' clause.");

		emit_loop(loopstart);

		loopstart = increment_start;

		back_patch_jump(bodyjump);
	}

	statement();

	emit_loop(loopstart);

	if (exitjump != -1) {
		back_patch_jump(exitjump);
		emit_byte(OP_POP); /* pop condition */
	}

	end_scope();
}

static void print_statement()
{
	expression();
	consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
	emit_byte(OP_PRINT);
}

static void synchronize()
{
	parser.panic_mode = false;

	while (parser.current.type != TOKEN_EOF) {
		if (parser.previous.type == TOKEN_SEMICOLON)
			return;

		switch (parser.current.type) {
		case TOKEN_CLASS:
		case TOKEN_FUN:
		case TOKEN_VAR:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_WHILE:
		case TOKEN_PRINT:
		case TOKEN_RETURN:
			return;

		default:
		    /* do nothing */;
		}

		advance();
	}
}

/* todo: address case of 24-bit constant indices*/
static uint8_t identifier_constant(struct token *name)
{
	return make_constant(
		OBJ(copy_string((char *)name->start, name->length)));
}

static bool identifiers_equal(struct token *a, struct token *b)
{
	if (a->length != b->length)
		return false;
	return memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(struct compiler *c, struct token *name)
{
	/*We walk backwards to ensure variables with the same name
	in upper scopes are masked*/
	for (int i = c->local_count - 1; i >= 0; i--) {
		struct local *l = &c->locals[i];
		if (identifiers_equal(&l->name, name)) {
			if (l->depth == -1)
				error("Cannot read local variable in its own initializer");
			return i;
		}
	}

	return -1;
}

static void add_local(struct token name)
{
	if (current->local_count == UINT8_COUNT) {
		error("Too many local variables in function");
		return;
	}
	struct local *l = &current->locals[current->local_count++];
	/* don't worry about lifetime here. The source buffer is alive all
	 * through compilation*/
	l->name = name;
	l->depth = -1; /*special sentinel value; variable is in scope but not
			  ready for use*/
}

static void declare_variable()
{
	struct token *name = &parser.previous;

	for (int i = current->local_count - 1; i > 0; --i) {
		struct local *l = &current->locals[i];
		if (l->depth != -1 && l->depth < current->scope_depth)
			break;

		if (identifiers_equal(name, &l->name))
			error("Variable with this name has been declared in this scope");
	}
	if (current->scope_depth == 0)
		return;

	add_local(*name);
}

/* todo: address case of 24-bit constant indices*/
static uint8_t parse_variable(const char *errmsg)
{
	consume(TOKEN_IDENTIFIER, errmsg);
	declare_variable();
	if (current->scope_depth > 0)
		return 0;
	return identifier_constant(&parser.previous);
}

static void mark_initialized()
{
	/* nothing to mark initialized in top-level fun decls*/
	if (current->scope_depth == 0)
		return;

	current->locals[current->local_count - 1].depth = current->scope_depth;
}

/* todo: address case of 24-bit constant indices*/
static void define_variable(uint8_t global)
{
	/*local variables are not created at runtime*/
	/*they land right on top of the stack, where we want them*/
	if (current->scope_depth > 0) {
		mark_initialized();
		return;
	}

	emit_2_bytes(OP_DEFINE_GLOBAL, global);
}

static void function(function_type_t type)
{
	struct compiler compiler;
	init_compiler(&compiler, type);

	begin_scope();

	/* compile parameter list */
	consume(TOKEN_LEFT_PAREN, "Expected '(' after function name.");
	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			current->function->arity++;
			if (current->function->arity > 255)
				error_at_current(
					"Cannot have more than 255 parameters.");

			uint8_t param_const =
				parse_variable("Expected parameter name.");
			define_variable(param_const);

		} while (match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "Expected ')' after function parameters.");

	/* compile body */

	consume(TOKEN_LEFT_BRACE, "Expected '{' before function body.");
	block();

	/* create function object*/
	obj_function_t *func = end_compiler();
	emit_2_bytes(OP_CONSTANT, make_constant(OBJ(func)));
}

static void fun_declaration()
{
	uint8_t global = parse_variable("Expected function name.");
	/* unlike local variables, a function can refer to itself
	in its own declaration. So, it can be safely marked as initialized
	as soon as the name is parsed.*/
	mark_initialized();
	function(TYPE_FUNCTION);
	define_variable(global);
}

static void and__(bool can_assign)
{
	int endjump = emit_jump(OP_JUMP_IF_FALSE);

	emit_byte(OP_POP);
	parse_precedence(PREC_AND);

	back_patch_jump(endjump);
}

static void or__(bool can_assign)
{
	int elsejump = emit_jump(OP_JUMP_IF_FALSE);
	int endjump = emit_jump(OP_JUMP);

	back_patch_jump(elsejump);
	emit_byte(OP_POP);

	parse_precedence(PREC_OR);
	back_patch_jump(endjump);
}

static void var_declaration()
{
	uint8_t global = parse_variable("Expected variable name");

	if (match(TOKEN_EQUAL)) {
		expression();
	} else {
		emit_byte(OP_NIL);
	}

	consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration");

	define_variable(global);
}

static void declaration()
{
	if (match(TOKEN_VAR)) {
		var_declaration();
	} else if (match(TOKEN_FUN)) {
		fun_declaration();
	} else {
		statement();
	}

	if (parser.panic_mode)
		synchronize();
}

static void return_statement()
{
	if (current->type == TYPE_SCRIPT)
		error("return statement not allowed in top-level code");
	if (match(TOKEN_SEMICOLON)) {
		emit_return();
	} else {
		expression();
		consume(TOKEN_SEMICOLON, "Expected ';' after return value.");
		emit_byte(OP_RETURN);
	}
}

static void statement()
{
	if (match(TOKEN_PRINT)) {
		print_statement();
	} else if (match(TOKEN_LEFT_BRACE)) {
		begin_scope();
		block();
		end_scope();
	} else if (match(TOKEN_IF)) {
		if_statement();

	} else if (match(TOKEN_WHILE)) {
		while_statement();

	} else if (match(TOKEN_FOR)) {
		for_statement();
	} else if (match(TOKEN_RETURN)) {
		return_statement();
	} else {
		expression_statement();
	}
}

static void unary(bool can_assign)
{
	token_type_t op = parser.previous.type;

	parse_precedence(PREC_UNARY);

	switch (op) {
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

static void number(bool can_assign)
{
	double val = strtod(parser.previous.start, NULL);
	emit_constant(NUMBER(val));
}

static void literal(bool can_assign)
{
	switch (parser.previous.type) {
	case TOKEN_FALSE:
		emit_byte(OP_FALSE);
		break;
	case TOKEN_TRUE:
		emit_byte(OP_TRUE);
		break;
	case TOKEN_NIL:
		emit_byte(OP_NIL);
		break;
	default:
		return;
	}
}

static void string(bool can_assign)
{
	char *s = (char *)(parser.previous.start + 1); /* advance past the "
							  mark*/
	int l = parser.previous.length - 2;
	emit_constant(OBJ(copy_string(s, l)));
}

static void named_variable(struct token name, bool can_assign)
{
	uint8_t get_op;
	uint8_t set_op;

	int arg = resolve_local(current, &name);

	if (arg != -1) {
		get_op = OP_GET_LOCAL;
		set_op = OP_SET_LOCAL;
	} else {
		arg = identifier_constant(&name);
		get_op = OP_GET_GLOBAL;
		set_op = OP_SET_GLOBAL;
	}

	if (can_assign && match(TOKEN_EQUAL)) {
		expression();
		emit_2_bytes(set_op, (uint8_t)arg);
	} else {
		emit_2_bytes(get_op, (uint8_t)arg);
	}
}

static void variable(bool can_assign)
{
	named_variable(parser.previous, can_assign);
}

static uint8_t argument_list()
{
	uint8_t argc = 0;

	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			expression();
			if (argc == 255)
				error("Cannot have more than 255 arguments.");
			argc++;

		} while (match(TOKEN_COMMA));
	}

	consume(TOKEN_RIGHT_PAREN, "Expected ')' after function parameters.");

	return argc;
}

static void call(bool can_assign)
{
	uint8_t argc = argument_list();
	emit_2_bytes(OP_CALL, argc);
}

struct parse_rule rules[] = {
	{ grouping, call, PREC_CALL }, /* TOKEN_LEFT_PAREN         */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_RIGHT_PAREN        */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_LEFT_BRACE         */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_RIGHT_BRACE        */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_COMMA              */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_DOT                */
	{ unary, binary, PREC_TERM }, /* TOKEN_MINUS              */
	{ NULL, binary, PREC_TERM }, /* TOKEN_PLUS               */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_SEMICOLON          */
	{ NULL, binary, PREC_FACTOR }, /* TOKEN_SLASH              */
	{ NULL, binary, PREC_FACTOR }, /* TOKEN_STAR               */
	{ unary, NULL, PREC_NONE }, /* TOKEN_BANG               */
	{ NULL, binary, PREC_EQUALITY }, /* TOKEN_BANG_EQUAL         */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_EQUAL              */
	{ NULL, binary, PREC_EQUALITY }, /* TOKEN_EQUAL_EQUAL        */
	{ NULL, binary, PREC_COMPARISON }, /* TOKEN_GREATER            */
	{ NULL, binary, PREC_COMPARISON }, /* TOKEN_GREATER_EQUAL      */
	{ NULL, binary, PREC_COMPARISON }, /* TOKEN_LESS               */
	{ NULL, binary, PREC_COMPARISON }, /* TOKEN_LESS_EQUAL         */
	{ variable, NULL, PREC_NONE }, /* TOKEN_IDENTIFIER         */
	{ string, NULL, PREC_NONE }, /* TOKEN_STRING             */
	{ number, NULL, PREC_NONE }, /* TOKEN_NUMBER             */
	{ NULL, and__, PREC_AND }, /* TOKEN_AND                */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_CLASS              */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_ELSE               */
	{ literal, NULL, PREC_NONE }, /* TOKEN_FALSE              */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_FOR                */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_FUN                */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_IF                 */
	{ literal, NULL, PREC_NONE }, /* TOKEN_NIL                */
	{ NULL, or__, PREC_OR }, /* TOKEN_OR                 */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_PRINT              */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_RETURN             */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_SUPER              */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_THIS               */
	{ literal, NULL, PREC_NONE }, /* TOKEN_TRUE               */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_VAR                */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_WHILE              */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_ERROR              */
	{ NULL, NULL, PREC_NONE }, /* TOKEN_EOF                */
};

static void begin_scope()
{
	current->scope_depth++;
}

static void end_scope()
{
	current->scope_depth--;
	while (current->local_count > 0 &&
	       current->locals[current->local_count - 1].depth >
		       current->scope_depth) {
		emit_byte(OP_POP); /*pop local variable off the stack at the end
				      of lifetime*/
		current->local_count--;
	}
}

static struct parse_rule *get_rule(token_type_t type)
{
	return &rules[type];
}

static void parse_precedence(precedence_t prec)
{
	advance();
	parse_fn prefix_rule = get_rule(parser.previous.type)->prefix;

	if (prefix_rule == NULL) {
		error("Expected expression");
		return;
	}

	bool can_assign = prec <= PREC_ASSIGNMENT;

	prefix_rule(can_assign);

	while (prec <= get_rule(parser.current.type)->precedence) {
		advance();
		parse_fn infix_rule = get_rule(parser.previous.type)->infix;
		infix_rule(can_assign);
	}

	if (can_assign && match(TOKEN_EQUAL))
		error("Invalid assignment target.");
}

obj_function_t *compile(const char *src)
{
	parser.had_error = false;
	parser.panic_mode = false;

	init_scanner(src);

	struct compiler compiler;
	init_compiler(&compiler, TYPE_SCRIPT);

	advance();

	while (!match(TOKEN_EOF)) {
		declaration();
	}

	obj_function_t *function = end_compiler();
	return parser.had_error ? NULL : function;
}
