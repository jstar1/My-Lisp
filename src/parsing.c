#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>
#include "mpc.h" //link to the mpc library  NOTE i am using "" because i am searching the local directory for this header file

int main(int argc, char ** argv) 
{
	/*Creating parsers */

	mpc_parser_t *Number 	= mpc_new("number");
	mpc_parser_t *Operator 	= mpc_new("operator");
	mpc_parser_t *Expr 	= mpc_new("expr");
	mpc_parser_t *Lispy 	= mpc_new("lispy");

	/*Defining them with the Language */
	
	/*This language is called Polish Notation, this is a notation for 
	 * arithmetic where the operator comes before the operanads
	 * 	Ex 2+2  is + 2 2
	*/
	
	mpca_lang(MPCA_LANG_DEFAULT,
	"								\
	number	 : /-?[0-9]+/;						\
	operator : '+' | '-' |'*' |'/';					\
	expr 	 : <number> | '(' <operator> <expr>+ ')';	 	\
	lispy    : /^/ <operator> <expr>+ /$/ ;				\
	",
	
	Number,Operator,Expr,Lispy);


	/* Print Version and Exit Information */
	puts("Lispy Version 0.1");
	puts("Press Ctrl+c to Exit\n");

	/* infinite loop */

	while(1)
	{
		/*Output our prompt and get input */

		char *input = readline("Lispy> ");

		/* Add input to history */
		add_history(input);

		/*Attempt to Parse the user Input */
		mpc_result_t r;
		
		if(mpc_parse("<stdin>", input, Lispy, &r))
		{
			/*If this is succesful*/
			mpc_ast_print(r.output);
			mpc_ast_delete(r.output);
		} 
		
		else
		{
			/*If there is an error */

			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		
		/*
		 Echo input back to user 
		printf("Is this what you typed %s\n", input);
		*/
		
		/*Free memory created for input*/
		free(input);
	}
	
	/* Undefine and  Delete our PARSERS */
	mpc_cleanup(4, Number, Operator, Expr, Lispy);
	return 0;

}
