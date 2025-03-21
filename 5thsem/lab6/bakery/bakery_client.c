/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include "bakery.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

void
bakery_prog_1(char *host)
{
	CLIENT *clnt;
#ifndef	DEBUG
	clnt = clnt_create (host, BAKERY_PROG, BAKERY_VER, "udp");
	if (clnt == NULL) {
		clnt_pcreateerror (host);
		exit (1);
	}
#endif	/* DEBUG */
	srand(getpid());
	while (1) {
		struct BAKERY_CALCULATOR  *result_1;
		struct BAKERY_CALCULATOR  get_number_1_arg;
		struct BAKERY_CALCULATOR  *result_2;
		struct BAKERY_CALCULATOR  perform_1_arg;

		sleep(rand() % 3 + 1);
		get_number_1_arg.op = rand() % 4;
		get_number_1_arg.arg1 = rand() % 100;
		get_number_1_arg.arg2 = rand() % 100 + 1;
		get_number_1_arg.pid = getpid();

		result_1 = get_number_1(&get_number_1_arg, clnt);
		if (result_1 == (struct BAKERY_CALCULATOR *) NULL) {
			clnt_perror (clnt, "call failed");
		}
		if (result_1->num == -1) {
			printf("no free numbers\n");
			exit(1);
        }
		printf("got number %d\n", result_1->num);

		sleep(rand() % 3 + 1);

		perform_1_arg = *result_1;
		result_2 = perform_1(&perform_1_arg, clnt);
		if (result_2 == (struct BAKERY_CALCULATOR *) NULL) {
			clnt_perror (clnt, "call failed");
		}
		if (result_2->num == -1) {
			printf("late\n");
		} else {
			char op;
			switch (result_2->op)
			{
			case ADD:
				op = '+';
				break;
			case SUB:
				op = '-';
				break;
			case MUL:
				op = '*';
				break;
			case DIV:
				op = '/';
				break;
			default:
				printf("error: invalid operation\n");
				break;
			}

			printf("%f %c %f = %f\n", result_2->arg1, op, result_2->arg2, result_2->result);
		}
	}
#ifndef	DEBUG
	clnt_destroy (clnt);
#endif	 /* DEBUG */
}


int
main (int argc, char *argv[])
{
	char *host;

	if (argc < 2) {
		printf ("usage: %s server_host\n", argv[0]);
		exit (1);
	}
	host = argv[1];
	bakery_prog_1 (host);
exit (0);
}
