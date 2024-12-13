#include "../dfa.h"
#include "../util/dla.h"




static void dfa_codegen_c_state_(FILE *out, dfa_t *dfa, re_ast_t *ast, int idx)
{
	bool accepting;
	int i;
	dla_t dsts[dfa->length];
	state_t *s;

	s = &dfa->states[idx];
	accepting = is_accepting_state(ast, s);
	fprintf(out, "S%d:\n", idx + 1);

	if (accepting)
		fprintf(out, "	*end = b;\n");




	if (!state_transition_inv(dfa, idx, dsts)) {
		for (i = 0; i < dfa->length; i++)
			bytes_deinit(&dsts[i]);
		fprintf(out, "	%s", accepting ? "return;\n" : "goto REDO;\n");
		return;
	}
	fprintf(out, "	++b;\n");
	if (idx == 0)
		fprintf(out, "start:\n");

	fprintf(out, "	switch (*b) {\n");

	for (i = 0; i < dfa->length; i++) {
		if (dsts[i].length != 0) {
			DLA_FOREACH(&dsts[i], u8_t, c, {

				fprintf(out, "		case 0x%X:", c);
				if (isprint(c))
					fprintf(out, "  // '%c'", c);
				fprintf(out, "\n");
			})
			fprintf(out, "			goto S%d;\n", i + 1);
		}
		bytes_deinit(&dsts[i]);
	}
	fprintf(out, "		default:\n			%s;\n	}\n", accepting ? "return" : "goto REDO");
}

void codegen_c(FILE *out, dfa_t *dfa, re_ast_t *ast)
{
	int i;
	fprintf(out, "void lex(const char **begin, const char **end)\n{\n");
	fprintf(out, "	const char *b = *begin;\n	*end = ((void*)0);\n	goto start;\n");

	for (i = 0; i < dfa->length; i++) {
		dfa_codegen_c_state_(out, dfa, ast, i);
	}
	fprintf(out, "REDO:\n	if (*end != ((void*)0))\n		return;\n");
	fprintf(out, "	switch (*b) {\n		case '\\0':\n		case '\\n':\n			return;\n");
	fprintf(out, "	}\n	++(*begin);\n	b = *begin;\n	goto start;\n}");
}