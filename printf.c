#define _XOPEN_SOURCE 700
#include <errno.h>
#include <ctype.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int errors = 0;

static void diagnose(const char *fmt, ...)
{
	char ofmt[strlen(fmt) + 10];
	sprintf(ofmt, "printf: %s\n", fmt);

	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, ofmt, ap);
	va_end(ap);

	errors = 1;
}

static const char *escape(const char *esc)
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

		putchar(strtol(oct, NULL, 8));
		return esc;
	}

	switch (*esc) {
	case '\\':
		putchar('\\');
		break;

	case 'a':
		putchar('\a');
		break;

	case 'b':
		putchar('\b');
		break;

	case 'f':
		putchar('\f');
		break;

	case 'n':
		putchar('\n');
		break;

	case 'r':
		putchar('\r');
		break;

	case 't':
		putchar('\t');
		break;

	case 'v':
		putchar('\v');
		break;

	default:
		diagnose("unknown escape \"\\%c\"", *esc);
		break;
	}
	return esc + 1;
}

static const char *echo(const char *s)
{
	while (*s) {
		if (*s != '\\') {
			putchar(*s++);
			continue;
		}

		s++;
		if (*s == 'c') {
			return NULL;
		}

		if (*s == '0' || !isdigit(*s)) {
			s = escape(s);
		} else {
			diagnose("unknown escape \"\\%c\"", *s);
			s++;
		}
	}
	return s;
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
		return echo(operand);

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
			diagnose("\"%s\" not completely converted", operand);
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
				diagnose("\"%s\" not completely converted", operand);
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
				diagnose("overflow converting \"%s\"", operand);
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

int main(int argc, char *argv[])
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
				format = escape(format + 1);
			} else {
				putchar(*format);
				format++;
			}

			if (format == NULL) {
				break;
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
