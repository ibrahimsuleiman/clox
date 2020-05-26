#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void repl()
{
	char line[1024];

	for (;;) {
		printf("> ");

		if (!fgets(line, sizeof(line), stdin)) {
			printf("\n");
			break;
		}
		interpret_vm(line);
	}
}

static char *read_file(const char *path)
{
	FILE *file = fopen(path, "rb");

	if (!file) {
		fprintf(stderr, "Couldn't open file \"%s\".", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	size_t file_size = ftell(file);
	rewind(file);

	char *buf = malloc(file_size + 1);

	if (!buf) {
		fprintf(stderr, "Not enough memory to read\n");
		exit(74);
	}
	size_t bytes_read = fread(buf, sizeof(char), file_size, file);

	if (bytes_read < file_size) {
		fprintf(stderr, "Couldn't read file\n");
		exit(74);
	}
	buf[bytes_read] = '\0';

	fclose(file);
	return buf;
}

static void run_file(const char *path)
{
	char *source = read_file(path);
	interpret_result_t r = interpret_vm(source);
	free(source);

	if (r == INTERPRET_COMPILE_ERROR)
		exit(65);
	if (r == INTERPRET_RUNTIME_ERROR)
		exit(70);
}

int main(int argc, char *argv[])
{
	init_vm();

	if (argc == 1) {
		repl();
	} else if (argc == 2) {
		run_file(argv[1]);
	} else {
		fprintf(stderr, "Usage: clox [path]\n");
		exit(64);
	}

	free_vm();

	return 0;
}