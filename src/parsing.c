#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>
#include "mpc.h" //link to the mpc library  NOTE i am using "" because i am searching the local directory for this header file

/* Macro used to help with error checking */
#define LASSERT(args, cond, err) \
	if (!(cond)) {lval_del(args); return lval_err(err);}

/* Forward Declarations */
struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

/* Create Enumeration of Possible lval Types */
enum {LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR};

/*Function pointer*/
typedef lval* (*lbuiltin)(lenv*, lval*);

/* Declare New lval struct */
/* lval stands for Lisp Value */
struct lval{
	int type;
	
	long num;
	/* Error and Symbol types have some string data */
	char *err;
	char *sym;
	lbuiltin fun;
	
	/* Count and Pointer to a list of "lval*" */
	int count;
	lval **cell;
};

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

/* A pointer to a new empty Qexpr lval */
lval *lval_qexpr(void){
	lval *v = malloc(sizeof(lval));
	v -> type = LVAL_QEXPR;
	v -> count = 0;
	v -> cell = NULL;
	return v;
}

/* A pointer to a new empty fun lval */
lval *lval_fun(lbuiltin func){
	
	lval *v = malloc(sizeof(lval));
	v -> type = LVAL_FUN;
	v -> fun = func;
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
		
		case LVAL_FUN: break;
		
		/* Qexpr then delete all the elements inside */
		case LVAL_QEXPR:	       
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
	
	/*if Qexpr then create empty list*/
	if (strstr(t -> tag, "qexpr")) {x = lval_qexpr();}
	
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

/*Forward declartion*/
void lval_print(lval* v);

/*Forward declartion*/
lval *lval_eval(lval *v);

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
/* Function used to print S-Expression*/
void lval_print(lval *v){
	switch (v -> type)
	{
		case LVAL_NUM: printf("%li", v -> num); break;
		case LVAL_ERR: printf("Error: %s", v -> err); break;
		case LVAL_SYM: printf("%s", v -> sym); break;
		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
		case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
		case LVAL_FUN:	printf("<function>"); break;
	}
}

/*Function for copying an lval, copying numbers and strings*/
lval *lval_copy(lval *v){
	
	lval *x = malloc(sizeof(lval));
	x -> type = v -> type;

	switch (v -> type)
	{
		/* Copy Functions and Numbers Directly */
		case LVAL_FUN: x -> fun = v -> fun; break;
		case LVAL_NUM: x -> num = v -> num; break;
		
		/* Copy Strings using malloc and strcpy */
		case LVAL_ERR:
			x -> err = malloc(strlen(v -> err) + 1);
			strcpy(x -> err, v -> err); break;

		case LVAL_SYM:
			x -> sym = malloc(strlen(v -> sym) + 1);
			strcpy(x -> sym, v -> sym); break;
		
		/* Copy Lists by copying each sub-pression */
		case LVAL_SEXPR:
		case LVAL_QEXPR:
			x -> count = v -> count;
			x -> cell = malloc(sizeof(lval*) * x -> count);
			for (int i = 0; i < x -> count; ++i)
			{
				x -> cell[i] = lval_copy(v -> cell[i]);
			}
		break;
	}

	return x;
}

/* Print an "lval" followed by a newline */
void lval_println(lval *v) {lval_print(v); putchar('\n');}

/* Function extracts a single element from the S-Expression at index i 
 * ANd shifts the rest of the list backwards so that it no longer contains
 * that lval
 */
lval *lval_pop(lval *v, int i)
{
	/* Find the item at "i" */
	lval *x = v -> cell[i];

	/* Shift memory after the item at "i" over the top */
	memmove(&v -> cell[i], &v -> cell[i+1],sizeof(lval*) * (v -> count -i -1));

	/* Decrease the count of items in the list */
	v -> count--;

	/* Reallocate the memory used*/
	v -> cell = realloc(v -> cell, sizeof(lval*) * v -> count);
	return x;
}

/* lval_take deletes the list it has extracted the element from. Takes element
 * from the list and deletes the rest of the list
 */

lval *lval_take(lval *v, int i)
{
	lval *x = lval_pop(v,i);
	lval_del(v);
	return x;
}

lval *builtin_op(lval *a, char *op)
{
	/* Ensure all arguments are numbers */
	for (int i =0; i < a -> count; ++i)
	{
		if (a -> cell[i] -> type != LVAL_NUM)
		{
			lval_del(a);
			return lval_err("Cannot operate on non_number!");
		}
	}

	/* Pop the first element */
	lval *x = lval_pop(a,0);

	/* If no arguments and sub then perform unary negation */
	if ((strcmp(op, "-") == 0) && a -> count == 0)
	{
		x -> num = -x -> num;
	}
	
	/* While there are still elements remaining */
	while (a -> count > 0)
	{
		/* Pop the next element */
		lval *y = lval_pop(a,0);

		if(strcmp(op, "+") == 0) {x -> num += y -> num;}
		if(strcmp(op, "-") == 0) {x -> num -= y -> num;}
		if(strcmp(op, "*") == 0) {x -> num *= y -> num;}
		if(strcmp(op, "/") == 0) {
			if (y -> num == 0)
			{
				lval_del(x); 
				lval_del(y);
				x = lval_err("Divsion By Zero!"); break;
			}
			x -> num /= y -> num;
		}

		lval_del(y);
	}
	lval_del(a); return x;
}

lval *builtin_head(lval *a){
	
	LASSERT(a, a -> count == 1, "Function 'head' passed too many arguments!");

	LASSERT(a, a -> cell[0] -> type == LVAL_QEXPR, "Function 'head' passed incorrect type!");
	
	LASSERT(a, a -> cell[0] -> count != 0, "Function 'head' passed {}!");

	lval *v = lval_take(a, 0);	
	while (v -> count > 1) {lval_del(lval_pop(v,1));}
	
	return v;
}

lval *builtin_tail(lval *a){

	LASSERT(a, a -> count == 1, "Function 'tail' passed too many arguments!");	
	
	LASSERT(a, a -> cell[0] -> type == LVAL_QEXPR, "Function 'tail' passed incorrect type!");

	LASSERT(a, a -> cell[0] -> count != 0, "Function 'tail' passed {}!");


	/* Take first argument */
	lval *v = lval_take(a, 0);

	/* Delete first element and return */
	lval_del(lval_pop(v,0));
	
	return v;
}

/* Takes in a Q-Expression input and converts it to an S-Expression an
 * evaluates u sing lval_eval
 */
lval *builtin_list(lval *a)
{
	a -> type = LVAL_QEXPR;
	
	return a;
}

lval *builtin_eval(lval *a)
{
	LASSERT(a, a -> count == 1, "Function 'eval' passed too many arguments!");

	LASSERT(a, a -> cell[0] -> type == LVAL_QEXPR,"Function 'eval' passed incorrect type!");

	lval *x = lval_take(a, 0);

	x -> type = LVAL_SEXPR;

	return lval_eval(x);
}

lval *lval_join(lval *x, lval *y){

	/* For each cell in 'y' add it to 'x' */
	while(y -> count)
	{
		x = lval_add(x, lval_pop(y, 0));
	}

	/* Delete the empty 'y' and reutnr 'x' */
	lval_del(y);
	
	return x;
}

/* Check all arguments are Q-Expressions and then we join them togeter one by one
 *
 */
lval *builtin_join(lval *a){

	for (int i = 0; i < a-> count; ++i)
	{
		LASSERT(a, a -> cell[i] -> type == LVAL_QEXPR,
		"Function 'join' passed incorrect type.");
	}

	lval *x = lval_pop(a,0);

	while (a -> count)
	{
		x = lval_join(x, lval_pop(a,0));
	}

	lval_del(a);

	return x;
}

/* Function used to call the correct builtin function depnding on t he symbol it encounters */
lval *builtin(lval *a, char *func){
	
	if(strcmp("list", func) == 0) {return builtin_list(a);}
	if(strcmp("head", func) == 0) {return builtin_head(a);}
	if(strcmp("tail", func) == 0) {return builtin_tail(a);}
	if(strcmp("join", func) == 0) {return builtin_join(a);}
	if(strcmp("eval", func) == 0) {return builtin_eval(a);}
	if(strstr("+-/*", func)) {return builtin_op(a,func);}
	lval_del(a);
	return lval_err("Unknown Function!");
}

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
			return lval_take(v, i);
		}
	}

	/* Empty Expression */
	if (v -> count == 0) {return v;}

	/* Single Expression */
	if (v -> count == 1) {return lval_take(v,0);}

	/*Ensure First Element is Symbol*/
	lval *f = lval_pop(v, 0);
	if(f -> type != LVAL_SYM)
	{
		lval_del(f);
		lval_del(v);
		return lval_err("S-expression Does not start with symbol!");
	}

	/* Call builtin with operator */
	lval *result = builtin(v, f -> sym);
	lval_del(f);
	return result;
}

lval *lval_eval(lval *v)
{
	/* Evaluate Sexpression */
	if(v -> type == LVAL_SEXPR) {return lval_eval_sexpr(v);}

	/* All other lval types remain the same*/
	return v;
}
	

int main(int argc, char ** argv) 
{

	/*Creating parsers */

	mpc_parser_t *Number 	= mpc_new("number");
	mpc_parser_t *Symbol 	= mpc_new("symbol");
	mpc_parser_t *Sexpr 	= mpc_new("sexpr");
	mpc_parser_t *Qexpr 	= mpc_new("qexpr");
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
	symbol 	 : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/;			\
	sexpr 	 : '(' <expr>* ')';					\
	qexpr 	 : '{' <expr>* '}';					\
	expr 	 : <number> | <symbol> | <sexpr> | <qexpr>;	 	\
	lispy    : /^/ <expr>+ /$/ ;					\
	",
	
	Number,Symbol,Sexpr,Qexpr, Expr,Lispy);


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
			
			lval *x = lval_eval(lval_read(r.output));
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
	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
	return 0;

}
