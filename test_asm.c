#include <stdio.h>

int lex(char **,char**);



int main(void)
{
	char *in = "0xFEADBEEF 07 -10 hgh";
	char *b, *e;

	b = in;

	while (lex(&b, &e)) {
		printf("Match: [%lu - %lu] [%c - %c]\n", b - in, e - in, *b, *e);
		b = e + 1;
	}
}