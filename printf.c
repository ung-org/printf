#define _XOPEN_SOURCE 700
#include <errno.h>
#include <ctype.h>
#include <libgen.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int errors = 0;
static const char *progname = "printf";

static void diagnose(const char *fmt, ...)
{
	char ofmt[strlen(fmt) + 10];
	sprintf(ofmt, "%s: %s\n", progname, fmt);

	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, ofmt, ap);
	va_end(ap);

	errors = 1;
}

static const char *escape(const char *esc, char *out)
{
	if (isdigit(*esc)) {
		char oct[4] = {0};
		oct[0] = *esc++;
		if (isdigit(*esc)) {
			oct[1] = *esc++;
		}
		if (isdigit(*esc)) {
			oct[2] = *esc++;
		}

		*out = (char)strtol(oct, NULL, 8);
		return esc;
	}

	switch (*esc) {
	case '\\':
		*out = '\\';
		break;

	case 'a':
		*out = '\a';
		break;

	case 'b':
		*out = '\b';
		break;

	case 'f':
		*out = '\f';
		break;

	case 'n':
		*out = '\n';
		break;

	case 'r':
		*out = '\r';
		break;

	case 't':
		*out = '\t';
		break;

	case 'v':
		*out = '\v';
		break;

	default:
		diagnose("unknown escape \"\\%c\"", *esc);
		break;
	}
	return esc + 1;
}

static int echo(const char *s, char *out)
{
	size_t i = 0;
	while (*s) {
		if (*s != '\\') {
			out[i++] = (*s++);
			continue;
		}

		s++;
		if (*s == 'c') {
			return 1;
		}

		if (*s == '0' || !isdigit(*s)) {
			char c = '\0';
			s = escape(s + 1, &c);
			out[i++] = (c);
		} else {
			diagnose("unknown escape \"\\%c\"", *s);
			s++;
		}
	}

	return 0;
}

static const char *convert(const char *conv, const char *operand)
{
	double d;
	long long int i;
	unsigned long long int u;
	char *end = NULL;
	char oconv[strlen(conv) + 11];
	char *ocend = oconv;
	char fconv[10];

	memset(oconv, 0, sizeof(oconv));
	strcpy(oconv, "%");
	ocend++;

	/* flags */
	while (strspn(conv, "-+ #0")) {
		*ocend++ = *conv++;
	}

	/* width */
	while (isdigit(*conv)) {
		*ocend++ = *conv++;
	}

	/* precision */
	if (*conv == '.') {
		*ocend++ = *conv++;
		while (isdigit(*conv)) {
			*ocend++ = *conv++;
		}
	}

	/* conversion specifier */
	switch (*conv) {
	case 'b':
		{
			char ebuf[strlen(operand) + 1];
			int quit = echo(operand, ebuf);
			strcat(oconv, "s");
			printf(oconv, ebuf);
			if (quit) {
				exit(errors);
			}
		}
		break;

	case 's':
		strcat(oconv, "s");
		printf(oconv, operand);
		break;

	case 'c':
		strcat(oconv, "c");
		printf(oconv, *operand);
		break;

	case 'a':
	case 'A':
	case 'e':
	case 'E':
	case 'f':
	case 'F':
	case 'g':
	case 'G':
		errno = 0;
		d = strtod(operand, &end);
		if (errno != 0) {
			diagnose("overflow converting \"%s\"", operand);
		}
		if (*end != '\0') {
			if (end == operand) {
				diagnose("\"%s\" expected numeric value", operand);
			} else {
				diagnose("\"%s\" not completely converted", operand);
			}
		}
		sprintf(fconv, "%c", *conv);
		strcat(oconv, fconv);
		printf(oconv, d);
		break;

	case 'd':
	case 'i':
		errno = 0;
		if (*operand == '\'' || *operand == '"') {
			i = operand[1];
		} else {
			i = strtoll(operand, &end, 0);
			if (*end != '\0') {
				if (end == operand) {
					diagnose("\"%s\" expected numeric value", operand);
				} else {
					diagnose("\"%s\" not completely converted", operand);
				}
			}
		}

		if (errno != 0) {
			diagnose("overflow converting \"%s\"", operand);
		}

		sprintf(fconv, "ll%c", *conv);
		strcat(oconv, fconv);
		printf(oconv, i);
		break;

	case 'o':
	case 'u':
	case 'x':
	case 'X':
		errno = 0;
		if (*operand == '\'' || *operand == '"') {
			u = operand[1];
		} else {
			u = strtoull(operand, &end, 0);
			if (errno != 0) {
				if (end == operand) {
					diagnose("\"%s\" expected numeric value", operand);
				} else {
					diagnose("overflow converting \"%s\"", operand);
				}
			}
		}

		if (*end != '\0') {
			diagnose("\"%s\" not completely converted", operand);
		}

		sprintf(fconv, "ll%c", *conv);
		strcat(oconv, fconv);
		printf(oconv, u);
		break;

	default:
		diagnose("invalid conversion specifier '%c'", *conv);
		return conv;
	}

	return conv + 1;
}

int printf_main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	while (getopt(argc, argv, "") != -1) {
		return 1;
	}

	int formatind = optind++;
	const char *format = argv[formatind];
	const char *operand = argv[optind++];

	int counting = 1;
	int count = 0;
	do {
		while (*format) {
			if (*format == '%') {
				if (*(format + 1) == '%') {
					putchar('%');
					format += 2;
				} else {
					if (counting) {
						count++;
					}
					format = convert(format + 1, operand);
					if (operand) {
						operand = argv[optind++];
					}
				}
			} else if (*format == '\\') {
				char c = '\0';
				format = escape(format + 1, &c);
				putchar(c);
			} else {
				putchar(*format);
				format++;
			}
		}

		if (count == 0) {
			break;
		}

		format = argv[formatind];
		counting = 0;
	} while (operand);

	return errors;
}

int echo_main(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++) {
		char ebuf[strlen(argv[i]) + 1];
		memset(ebuf, '\0', sizeof(ebuf));
		int quit = echo(argv[i], ebuf);
		printf("%s", ebuf);
		if (quit) {
			return errors;
		}
		putchar(i == argc - 1 ? '\n' : ' ');
	}
	return errors;
}

int main(int argc, char *argv[])
{
	if (!strcmp(basename(argv[0]), "echo")) {
		progname = "echo";
		return echo_main(argc, argv);
	}

	return printf_main(argc, argv);
}
