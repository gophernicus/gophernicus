/*
 * bin2c - Copyright (c) 2010 Kim Holviala <kim@holviala.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

