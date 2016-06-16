#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>
#include "mpc.h"

int main(int argc, char ** argv) 
{
	/*Creating parsers */

	mpc_parser_t *Number 	= mpc_new("number");
	mpc_parser_t *Operator 	= mpc_new("operator");
	mpc_parser_t *Expr 	= mpc_new("expr");
	mpc_parser_t *Lipsy 	= mpc_new("lispy");

	/*Defining them with the Language */

	mpca_lang(MPCA_LANG_DEFAULT,
	"								/
	number	 : /-?[0-9]+/;						/
	operator : '+'| '-'|'*'|'/'					/
	expr 	 : <number> | '(' <operator> <expr>+ ')';	 	/
	lispy    : /^/ <operator> <expr>+ /$/ ;				/
	",
	Number,Operator,Expr,Lispy);




	/* Print Version and Exit Information */
	puts("Lispy Version 0.1");
	puts("Press Ctrl+c to Exit\n");

	/* infinite loop */

	while(1)
	{
		/*Output our prompt and get inout */

		char *input = readline("lipsy> ");

		/* ADd input to history */
		add_history(input);

		/* Echo input back to user */
		printf("Is this what you typed %s\n", input);

		/*Free memory created for input*/
		free(input);
	}
	
	/* Undefine and  Delete our PARSERS */
	MPC_CLEANUP(4, nUMBER, oPERATOR, Expr, Lipsy);
	return 0;

}
