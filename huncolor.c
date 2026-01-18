#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <hunspell/hunspell.h>

const char *aff_path = "/usr/share/hunspell/en_US.aff";
const char *dic_path = "/usr/share/hunspell/en_US.dic";

#define START "\033[1m"
#define STOP "\033[0m"

static int check_word(Hunhandle *hunhandle, const char *word, size_t len)
{
	char buf[80];

	// We're not doing German of Finnish...
	if (len >= sizeof(buf))
		return 1;

	memcpy(buf, word, len);
	buf[len] = 0;
	return Hunspell_spell(hunhandle, buf);
}

static void check_and_print(Hunhandle *hunhandle, const char *word, size_t len)
{
	if (check_word(hunhandle, word, len)) {
		write(1, word, len);
		return;
	}

	write(1, START, strlen(START));
	write(1, word, len);
	write(1, STOP, strlen(STOP));
}

// Print a single line, colorizing unrecognized words
//
// This is incredibly stupid, and only handles plain
// US-ASCII text.
static void print_one_line(Hunhandle *hunhandle, const char *line)
{
	const char *last = line, *begin = NULL;
	unsigned char c;
	bool not_a_word = false;

	for (unsigned char c; (c = *line) != 0 ; line++) {
		switch (*line) {
		case 'A' ... 'Z':
		case 'a' ... 'z':
			if (begin)
				continue;
			if (last != line) {
				write(1, last, line-last);
				last = line;
			}
			begin = line;
			continue;

		// Mixed letters and numbers / underscores are C identifiers
		case '0' ... '9':
		case '_':
			not_a_word = true;
			continue;

		// Special case
		case '\'':
			if (begin && !not_a_word && isalpha(line[1]))
				continue;
			/* fallthrough */
		default:
			if (!begin) {
				not_a_word = false;
				continue;
			}
			if (not_a_word)
				write(1, last, line-last);
			else
				check_and_print(hunhandle, last, line-last);
			not_a_word = false;
			last = line;
			begin = NULL;
		}
	}
	if (begin) {
		write(1, "<", 1);
		write(1, last, line-last);
		write(1, ">", 1);
	} else {
		write(1, last, line-last);
	}
}

static void exec_less(char **argv)
{
	execvp("less", argv);
	perror("Couldn't exec 'less'");
	exit(1);
}

// Note: for longer lines, we'll just break at random
// points.
//
// I simply don't care enough about spell checking
// for that kind of input, so I'm being intentionally
// lazy
#define MAX_LINE_LENGTH 1024

int main(int argc, char **argv)
{
	int fd[2];
	char line[MAX_LINE_LENGTH];

	if (isatty(0))
		exec_less(argv);

	Hunhandle *hunhandle = Hunspell_create(aff_path, dic_path);

	if (!hunhandle || pipe(fd))
		exec_less(argv);

	if (fork()) {
		dup2(fd[0], 0);
		close(fd[0]);
		close(fd[1]);
		exec_less(argv);
	}
	dup2(fd[1], 1);
	close(fd[0]);
	close(fd[1]);

	while (fgets(line, sizeof(line), stdin) != NULL) {
		if (!hunhandle) {
			fputs(line, stdout);
			continue;
		}
		print_one_line(hunhandle, line);
	}

	Hunspell_destroy(hunhandle);

	return 0;
}
