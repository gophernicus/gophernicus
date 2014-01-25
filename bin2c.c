/*
 * Convert any file into a C #define
 *
 * Yes, this would have been a perl one-liner, but I didn't want
 * to include compile-time dependency for perl...
 */

#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	FILE *fp;
	char *source = NULL;
	char *name = NULL;
	int first = 1;
	int zero = 0;
	int c;
	int i;

	/* Parse args */
        while ((c = getopt(argc, argv, "n:0")) != -1) {
                switch(c) {
			case 'n': name = optarg; break;
			case '0': zero = 1; break;
                }
        }

	source = argv[optind];
	if (!name) name = source;

	/* Check args */
	if (!source) {
		fprintf(stderr, "Usage: %s [-0] [-n <name>] <source>\n", argv[0]);
		return 1;
	}

	/* Try to open the source file */
	if ((fp = fopen(source, "r")) == NULL) {
		perror("Couldn't open source file");
		return 1;
	}

	/* Convert */
	printf("/* Automatically generated from %s */\n\n"
		"#define %s { \\\n", source, name);

	do {
		for (i = 0; i < 16; i++) {
			if ((c = fgetc(fp)) == EOF) {
				if (zero--) c = '\0';
				else break;
			}

			if (i == 0 && !first) printf(", \\\n");
			if (i > 0) printf(", ");

			printf("0x%02x", c);
			first = 0;
		}
	} while (c != EOF);

	printf("}\n\n");
	fclose(fp);
	return 0;
}

