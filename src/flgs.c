#include "flgs.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

uc NEED_WARN = 1;

const struct Flag flags[] = {
	{"-д", CF_DEBUG},  {"-о", CF_DEBUG}, {"-d", CF_DEBUG}, {"-ц", CF_TARGET},
	{"-t", CF_TARGET}, {"-w", CF_WARNS}, {"-п", CF_WARNS},
};

void exit_msg(char *msg) {
	printf("%s\n", msg);
	exit(1);
}
void exit_msg_arg(char *msg, char *arg) {
	printf("%s %s\n", msg, arg);
	exit(1);
}

struct Flags *get_flags(int argc, char **args) {
	struct Flags *f = malloc(sizeof(struct Flags));

	if (argc < 3)
		exit_msg("Использвание: [исходный код] [исполнимый файл]");
	if (access(args[1], F_OK) != 0) {
		printf("ОШИБКА: Файл [%s] с предполагаемым исходным кодом не "
			   "существует или не доступен\n",
			   args[1]);
		exit(1);
	}
	f->filename = args[1];
	f->outname = args[2];
	f->tr = Linux_ELF_86_64; // also need to be in args later
	f->debug = 0b01;		 // 0b10 prints cmnds by bytes

	struct Flag flag;
	int i, j, flen = (int)lenofarr(flags);
	char *arg;
	for (i = 2; i < argc; i++) {
		arg = args[i];
		for (j = 0; j < flen; j++) {
			flag = flags[j];

			if (sc(arg, flag.v)) {
				i++;
				if (i >= flen)
					exit_msg_arg("Нехватает значения для флага", arg);

				switch (flag.f) {
				case CF_DEBUG:
					f->debug = atoi(args[i]);
					goto get_flags_exit_j_loop;
				case CF_TARGET:
					goto get_flags_exit_j_loop;
				case CF_WARNS:

					NEED_WARN = atoi(args[i]);
					goto get_flags_exit_j_loop;
				}
			}
		}
	get_flags_exit_j_loop:;
	}
	return f;
}
