#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>
#include "mpc.h" //link to the mpc library  NOTE i am using "" because i am searching the local directory for this header file


/* Defining eval_op, test for which operator is passed in and performs the C operation on input */
long eval_op(long x, char *op, long y){
	
	if(strcmp(op, "+") == 0) {return x + y;}

	if(strcmp(op,"-") == 0) {return x - y;}

	if(strcmp(op, "*") == 0) {return x * y;}

	if(strcmp(op, "/") == 0) {return x / y;}

	return 0;
}

/*recurrsive evaluation function */
long eval(mpc_ast_t *t){
	
	/*If tagged as number return it directly. */
	if(strstr(t -> tag, "number"))
	{
		return atoi(t -> contents);
	}

	/*The operator is always second child*/
	char *op = t -> children[1] -> contents;

	/* We store the third child in 'x' */
	long x = eval(t -> children[2]);

	/*Iterate the remaining children and comvining */
	int i = 3;

	while(strstr(t -> children[i] -> tag, "expr"))
	{
		x = eval_op(x, op, eval(t -> children[i]));
		++i;
	}

	return x;
}	

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
	operator : '+' | '-' | '*' | '/' | '%';				\
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

		/*This section of code displays the tree*/
		/*Attempt to Parse the user Input */
		mpc_result_t r;
		

				
		if(mpc_parse("<stdin>", input, Lispy, &r))
		{
			mpc_ast_t *result = r.output;
			printf("%li\n",eval(result));
			mpc_ast_delete(r.output);
		} 
		
		else
		{
			//If there is an error 

			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
			
		
		/*Free memory created for input*/
		free(input);
	}
	
	/* Undefine and  Delete our PARSERS */
	mpc_cleanup(4, Number, Operator, Expr, Lispy);
	return 0;

}
