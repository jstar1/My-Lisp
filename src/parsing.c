#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>
#include "mpc.h" //link to the mpc library  NOTE i am using "" because i am searching the local directory for this header file


/* Create Enumeration of Possible lval Types */
enum {LVAL_ERR,LVAL_NUM,LVAL_SYM,LVAL_SEXPR};

/* Declare New lval struct */
/* lval stands for Lisp Value */
typedef struct lval{
	int type;
	long num;
	/* Error and Symbol types have some string data */
	char *err;
	char *sym;
	/* Count and Pointer to a list of "lval*" */
	int count;
	struct lval **cell;
} lval;

/* Create Enumeration of Possible Error Types */
//enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR, LERR_BAD_NUM};

/* Creating a new number type lval*/
lval *lval_num(long x){
	lval *v = malloc(sizeof(lval));
	v -> type = LVAL_NUM;
	v -> num = x;
	return v;
}

/* Create a new error type lval */
lval *lval_err(char *m){
	lval *v = malloc(sizeof(lval));
	v -> type = LVAL_ERR;
	v -> err = malloc(strlen(m) + 1);
	strcpy(v -> err, m);
	return v;
}

/* Construct a pointer to a new Symbol lval */
lval *lval_sym(char *s){
	lval *v = malloc(sizeof(lval));
	v -> type = LVAL_SYM;
	v -> sym = malloc(strlen(s) + 1);
	strcpy(v -> sym, s);
	return v;
}

/* A pointer to a new empty Sexpr lval */
lval *lval_sexpr(void){
	lval *v = malloc(sizeof(lval));
	v -> type = LVAL_SEXPR;
	v -> count = 0;
	v -> cell = NULL;
	return v;	
}
/* Function to free memory */
void lval_del(lval *v){

	switch (v -> type)
	{
		/* Do nothing special for number type */
		case LVAL_NUM: break;
		
		/* For Err or Sym free the string data */
		case LVAL_ERR: free(v -> err); break;
		case LVAL_SYM: free(v -> sym); break;
	
		/* If Sexpr then delete all elements inside */
		case LVAL_SEXPR:
			for (int i = 0; i < v -> count; ++i)
			{
				lval_del(v -> cell[i]);
			}
		/* Also free the memory allocated to contain the pointers */
		free(v -> cell);
		break;	
	}
	/* Free the memory allocated for the "lval" struct itself */
	free(v);	
}

lval *lval_read_num(mpc_ast_t* t){
	errno = 0;
	long x = strtol(t -> contents, NULL, 10);
	return errno != ERANGE ?
		lval_num(x) : lval_err("invalid number");
}

lval *lval_add(lval *v, lval *x){
	v -> count++;
	v -> cell = realloc(v -> cell, sizeof(lval*) * v -> count);
	v -> cell[v -> count -1] = x;
	return v;
}

lval *lval_read(mpc_ast_t* t){
	
	/* If Symbol or Number return conversion to that type */
	if(strstr(t -> tag, "number")) {return lval_read_num(t);}
	if(strstr(t -> tag, "symbol")) {return lval_sym(t -> contents);}

	/*If root (>) or sexpr then create empty list */
	lval* x = NULL;
	if(strcmp(t -> tag, ">") == 0) {x = lval_sexpr();}
	if(strstr(t -> tag, "sexpr")) {x = lval_sexpr();}

	/* FIll this list w ith any valid expression contain within */
	for (int i = 0; i < t -> children_num; ++i)
	{
		if(strcmp(t -> children[i] -> contents, "(") == 0) { continue;}
		if(strcmp(t -> children[i] -> contents, ")") == 0) { continue;} 
		if(strcmp(t -> children[i] -> contents, "}") == 0) { continue;}
		if(strcmp(t -> children[i] -> contents, "{") == 0) { continue;}
		if(strcmp(t -> children[i] -> tag,  "regex") == 0) { continue;}
		
		x = lval_add(x, lval_read(t-> children[i]));
	}
	return x;
}

//void lval_print(lval v){
//	switch(v.type){
//	
//	/* In the case the type is a number print it */
//	/* Then 'break out of the switch. */
//		case LVAL_NUM: printf("%li", v.num); break;
//	
//		/* In the case the type is an error */
//		case LVAL_ERR:
//		
//		/* Check what type of error it is and print it */
//		if(v.err == LERR_DIV_ZERO)
//		{
//			printf("Error: Divsion By Zero!");
///		}
//		if(v.err == LERR_BAD_OP)
//		{
//			printf("Error: Invalid  Operator!");
//		}
//		if(v.err == LERR_BAD_NUM)
//		{
//			printf("Error: Invalid Number!");
//		}
//		break;
//	}
//}

/*Forward declartion*/
void lval_print(lval* v);

/* function to print out S-Expressions*/
/* This function loops over all sub expression of an expression and prints with spaces */

void lval_expr_print(lval *v, char open, char close){

	putchar(open);
	for(int i = 0; i < v ->count; ++i)
	{
		/* Print Value contained within */
		lval_print(v -> cell[i]);

		/* Dont print trailing space if l ast element */
		if (i != (v -> count - 1))
		{
			putchar(' ');
		}
	}
	putchar(close);
}
/* FUnction used to print S-Expression*/
void lval_print(lval *v){
	switch (v -> type)
	{
		case LVAL_NUM: printf("%li", v -> num); break;
		case LVAL_ERR: printf("Error: %s", v -> err); break;
		case LVAL_SYM: printf("%s", v -> sym); break;
		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
	}
}

/* Print an "lval" followed by a newline */
void lval_println(lval *v) {lval_print(v); putchar('\n');}

/*Function to evaluate nodes in the tree*/
lval *lval_eval_sexpr(lval *v){

	/* Evaluate Children */
	for (int i = 0; i < v -> count; ++i)
	{
		v -> cell[i] = lval_eval(v -> cell[i]);
	}

	/*Error Checking */
	for (int i = 0; i < v -> count; ++i)
	{
		if (v -> cell[i] -> type == LVAL_ERR)
		{
			return lval_tale(v, i);
		}
	}

	/* Empty Expression */
	if (v -> count ==0) {return v;}

	/* Single Expression */
	if (v -> count ==1) {return lval_take(v,0);}

	/*Ensure First Element is Symbol*/
	lval *f = lval_pop(v, 0);
	if(f -> type != LVAL_SYM)
	{
		lval_def(f);
		lval_del(v);
		return lval_err("S-expression Does not start with symbol!");
	}

	/* Call builtin with operator */
	lval *result = builtin_op(v, f -> sym);
	lval_def(f);
	return result;
}


/* Defining eval_op, test for which operator is passed in and performs the C operation on input */
//lval eval_op(lval x, char *op, lval y){
//
//	/* If either value is an error return it */
//	if(x.type == LVAL_ERR) {return x;}
//	
//	if(y.type == LVAL_ERR) {return y;}
//
//	/* Otherwise do maths on the number values */
//
//	if(strcmp(op, "+") == 0) {return lval_num(x.num + y.num);}
//
//	if(strcmp(op, "-") == 0) {return lval_num(x.num - y.num);}
//
//	if(strcmp(op, "*") == 0) {return lval_num(x.num * y.num);}
//	
//	if(strcmp(op, "%") == 0) {return lval_num(x.num % y.num);}
//	
//	if(strcmp(op, "/") == 0) {
//
//	/* If second operand is zero return error */
//
//	return y.num == 0
//	? lval_err(LERR_DIV_ZERO)
//	: lval_num(x.num / y.num);
//	
//	}
//
//	return lval_err(LERR_BAD_OP);
//}

/*recurrsive evaluation function */
//lval eval(mpc_ast_t *t){
//	
//	/*If tagged as number return it directly. */
//	if(strstr(t -> tag, "number"))
//	{
//		//return atoi(t -> contents);
//		/*Check if there is some error in conversion */
///		errno = 0;
//		long x = strtol(t ->contents, NULL, 10);
//		return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
//	}
//
//	/*The operator is always second child*/
//	char *op = t -> children[1] -> contents;
//
//	/* We store the third child in 'x' */
//	lval x = eval(t -> children[2]);
//
//	/*Iterate the remaining children and comvining */
//	int i = 3;
//
//	while(strstr(t -> children[i] -> tag, "expr"))
//	{
//		x = eval_op(x, op, eval(t -> children[i]));
//		++i;
//	}
//
//	return x;
//}	

int main(int argc, char ** argv) 
{

	/*Creating parsers */

	mpc_parser_t *Number 	= mpc_new("number");
	mpc_parser_t *Symbol 	= mpc_new("symbol");
	mpc_parser_t *Sexpr 	= mpc_new("sexpr");
	//mpc_parser_t *Operator 	= mpc_new("operator");
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
	symbol 	 : '+' | '-' | '*' | '/' | '%';				\
	sexpr 	 : '(' <expr>* ')';					\
	expr 	 : <number> | <symbol> | <sexpr>;	 		\
	lispy    : /^/ <expr>+ /$/ ;					\
	",
	
	Number,Symbol,Sexpr,Expr,Lispy);


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
			
			lval *x = lval_read(r.output);
			lval_println(x);
			lval_del(x);
			
			/*
			lval result = eval(r.output);
			lval_println(result);
			mpc_ast_delete(r.output);
			
			mpc_ast_t *result = r.output;
			printf("%li\n",eval(result));
			mpc_ast_delete(r.output);
			*/
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
	mpc_cleanup(5, Number, Symbol,Sexpr, Expr, Lispy);
	return 0;

}
