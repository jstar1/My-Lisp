#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>
#include "mpc.h" //link to the mpc library  NOTE i am using "" because i am searching the local directory for this header file

/* Macro used to help with error checking */
#define LASSERT(args, cond, fmt, ...) if (!(cond)) { lval *err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err;}

#define LASSERT_NUM(builtin, args, num)\
	LASSERT(args, (args -> count == num), "Function '%s' passed too many arguments. Got %i, Exprected %i." ,builtin, a -> count, num);

#define LASSERT_TYPE(builtin,args,index,expect)\
	LASSERT(args, (args -> cell[index] -> type == expect), "Function '%s' passed incorrect type for argument %i. Got %s, expected %s",builtin,index,ltype_name(args -> cell[index] -> type), ltype_name(LVAL_QEXPR));


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

	/* Basic */	
	long num;
	char *err;
	char *sym;
	
	/* Function */
	lbuiltin builtin;
	lenv *env;
	lval *formals;
	lval *body;

	/* Expression */	
	int count;
	lval **cell;
};

/* Declaring a lenv struct */
struct lenv{
	lenv *par;
	int count;
	char ** syms;
	lval **vals;
};

/* Creating a new number type lval*/
lval *lval_num(long x){
	lval *v = malloc(sizeof(lval));
	v -> type = LVAL_NUM;
	v -> num = x;
	return v;
}

/* Create a new error type lval */
lval *lval_err(char *fmt, ...){
	lval *v = malloc(sizeof(lval));
	v -> type = LVAL_ERR;

	/* Create a va list and initialize it */
	va_list va;
	va_start(va, fmt);

	/* Allocate 512 bytes of space */
	v -> err = malloc(512);

	/* printf the error string with a maximum of 511 characters */
	vsnprintf(v -> err, 511, fmt, va);

	/* Reallocate to number of bytes actually used */
	v -> err = realloc(v -> err, strlen(v -> err) + 1);

	/* Cleanup our va list */
	va_end(va);

	return v;
}

char * ltype_name(int t){
	switch(t){
		case LVAL_FUN: return "builtintion";
		case LVAL_NUM: return "Number";
		case LVAL_ERR: return "Error";
		case LVAL_SYM: return "Symbol";
		case LVAL_SEXPR: return "S-Expression";
		case LVAL_QEXPR: return "Q-Expression";
		default: return "Unknown";
	}
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
lval *lval_fun(lbuiltin builtin){
	
	lval *v = malloc(sizeof(lval));
	v -> type = LVAL_FUN;
	v -> builtin = builtin;
	return v;
}

void lenv_del(lenv *e);

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
			if(!v -> builtin)
			{
				lenv_del(v -> env);
				lval_del(v -> formals);
				lval_del(v -> body);
			}
		break;
		
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
lval *lval_eval(lenv *e, lval *v);

/* builtintion to print out S-Expressions*/
/* This builtintion loops over all sub expression of an expression and prints with spaces */

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
		case LVAL_FUN:	 
			if(v -> builtin)
			{
				printf("<builtin>");
	      	        }
			else
			{
				printf("(\\"); lval_print(v -> formals);
				putchar(' '); lval_print(v -> body); putchar('(');
			}
	}
}

/*Function for copying an lval, copying numbers and strings*/
lval *lval_copy(lval *v){
	
	lval *x = malloc(sizeof(lval));
	x -> type = v -> type;

	switch (v -> type)
	{
		/* Copy Functions and Numbers Directly */
		case LVAL_NUM: x -> num = v -> num; break;
		
		/* Copy Strings using malloc and strcpy */
		case LVAL_ERR:
			x -> err = malloc(strlen(v -> err) + 1);
			strcpy(x -> err, v -> err); break;

		case LVAL_SYM:
			x -> sym = malloc(strlen(v -> sym) + 1);
			strcpy(x -> sym, v -> sym); break;
		
		case  LVAL_FUN:
			if(v -> builtin)
			{
				x -> builtin = v -> builtin;

			}
			else
			{
				x -> builtin =  NULL;
				x -> env = lenv_copy(v -> env);
				x -> formals = lval_copy(v -> formals);
				x -> body = lval_copy(v -> body);
			}
			break;
		
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

/* Create new lenv */
lenv *lenv_new(void){
	lenv *e = malloc(sizeof(lenv));
	e -> par = NULL;
	e -> count = 0;
	e -> syms = NULL;
	e -> vals = NULL;
	return e;
}
	
/* Build environment for the function */
lval *lval_lambda(lval *formals, lval *body){
	lval *v = malloc(sizeof(lval));
	v -> type = LVAL_FUN;

	/* Set Builtin to NUll*/
	v -> builtin = NULL;

	/* Build new environment */
	v -> env = lenv_new();

	/* Set Formals and Body */
	v -> formals = formals;
	v -> body = body;
	return v;
}

lval *lenv_get(lenv *e, lval *k){

	for (int i = 0; i < e -> count; ++i)
	{
		/* Check if the stored string m atches the symbol string*/
		/* If it does, return a copy of a the vaule */
		if(strcmp(e -> syms[i], k -> sym) == 0)
		{
			return lval_copy(e -> vals[i]);
		}
	}

	/* If no symbol check in parent otherwise error */
	if(e -> par)
	{
		return lenv_get(e -> par, k);		
	}
	else
	{
		return lval_err("Unbound Symbol '%s'", k -> sym);
	}
}

/* Delete lenv */
void lenv_del(lenv *e){
	for(int i = 0; i < e -> count; ++i)
	{
		free(e -> syms[i]);
		lval_del(e -> vals[i]);
	}
	free(e -> syms);
	free(e -> vals);
	free(e);
}

void lenv_put(lenv *e, lval *k, lval *v){

	/* Iterate over all items in environment */
	/* This is to see if variable already exists */
	for(int i = 0; i < e -> count; ++i)
	{
		/* If variable is found delete item at that position */
		/* And replace with variable supplied by user */
		if(strcmp(e -> syms[i], k -> sym) == 0) 
		{
			lval_del(e -> vals[i]);
			e -> vals[i] = lval_copy(v);
			return;
		}
	}

	/* If no existing entry found allocated space for new entry */
	e -> count++;
	e -> vals = realloc(e -> vals, sizeof(lval*) * e -> count);
	e -> syms = realloc(e -> syms, sizeof(char*) * e -> count);

	/* COpy contents of lval and symbol string into new location */
	e -> vals[e -> count - 1] = lval_copy(v);
	e -> syms[e -> count - 1] = malloc(strlen(k -> sym) + 1);
	strcpy(e -> syms[e -> count - 1], k -> sym);
}


lval *builtin_op(lenv *e, lval *a, char *op)
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

lval *builtin_head(lenv *e, lval *a){
	
	//LASSERT(a, a -> count == 1, "Function 'head' passed too many arguments!");

	//LASSERT(a, a -> cell[0] -> type == LVAL_QEXPR, "Function 'head' passed incorrect type!");
	

	LASSERT(a, a -> cell[0] -> count != 0, "Function 'head' passed {}!");
	LASSERT_NUM("head", a, 1);
	LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
	lval *v = lval_take(a, 0);	
	
	while (v -> count > 1) {lval_del(lval_pop(v,1));}
	
	return v;
}

lval *builtin_tail( lenv *e, lval *a){

	//LASSERT(a, a -> count == 1, "Function 'tail' passed too many arguments!");	
	
	//LASSERT(a, a -> cell[0] -> type == LVAL_QEXPR, "Function 'tail' passed incorrect type!");

	LASSERT_NUM("tail", a,1);
	LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
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
lval *builtin_list(lenv *e, lval *a)
{
	a -> type = LVAL_QEXPR;
	
	return a;
}

lval *builtin_eval(lenv *e, lval *a)
{
	//LASSERT(a, a -> count == 1, "Function 'eval' passed too many arguments!");

	//LASSERT(a, a -> cell[0] -> type == LVAL_QEXPR,"Function 'eval' passed incorrect type!");

	LASSERT_NUM("eval", a, 1);
	LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);
	
	lval *x = lval_take(a, 0);

	x -> type = LVAL_SEXPR;

	return lval_eval(e, x);
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
lval *builtin_join(lenv *e, lval *a){

	for (int i = 0; i < a-> count; ++i)
	{
		//LASSERT(a, a -> cell[i] -> type == LVAL_QEXPR,
		//"Function 'join' passed incorrect type.");
		LASSERT_TYPE("join", a, i, LVAL_QEXPR);
	}

	lval *x = lval_pop(a,0);

	while (a -> count)
	{
		x = lval_join(x, lval_pop(a,0));
	}

	lval_del(a);

	return x;
}

/* Function used to call the correct builtin builtintion depnding on t he symbol it encounters */
lval *builtin(lenv *e,lval *a, char *builtin){
	
	if(strcmp("list", builtin) == 0) {return builtin_list(e, a);}
	if(strcmp("head", builtin) == 0) {return builtin_head(e, a);}
	if(strcmp("tail", builtin) == 0) {return builtin_tail(e, a);}
	if(strcmp("join", builtin) == 0) {return builtin_join(e, a);}
	if(strcmp("eval", builtin) == 0) {return builtin_eval(e ,a);}
	if(strstr("+-/*", builtin)) {return builtin_op(e, a, builtin);}
	lval_del(a);
	return lval_err("Unknown Function!");
}

/*Functions to define separate builtins for each of maths builtintions for language supports*/
lval *builtin_add(lenv *e, lval *a){return builtin_op(e, a, "+");}
lval *builtin_sub(lenv *e, lval *a){return builtin_op(e, a, "-");}
lval *builtin_mul(lenv * e, lval *a){return builtin_op(e, a, "*");}
lval *builtin_div(lenv * e, lval *a){return builtin_op(e, a , "/");}

/*Register built ins*/
void lenv_add_builtin(lenv *e, char *name, lbuiltin builtin){
	lval *k = lval_sym(name);
	lval *v = lval_fun(builtin);
	lenv_put(e, k, v);
	lval_del(k); 
	lval_del(v);
}

/*Function checks for error conditions then perofrms command and returns a value*/
lval *builtin_def(lenv *e, lval *a){
	
	//LASSERT(a, a -> cell[0] -> type == LVAL_QEXPR, "Function 'def' passed incorrect type!");
	LASSERT_TYPE("def", a, 0, LVAL_QEXPR);
	
	/* First argument is symbol list */
	lval *syms = a -> cell[0];

	/* Ensure all elements of first list are symbols */
	for (int i = 0; i < syms -> count; ++i)
	{
		//LASSERT(a, syms -> cell[i] -> type == LVAL_SYM, "Function 'def' cannot define non-symbol");
		LASSERT(a,(syms -> cell[i] -> type == LVAL_SYM), "Function 'def' cannot define n on-symbol at index '%i'",i);
	}

	/* Check correct number of symbols and values */
	LASSERT(a, syms -> count == a -> count - 1, "Function 'def' cannot define incorrect number of values to symbols");

	/* Assign copies of values to symbols */
	for (int i = 0; i < syms -> count; ++i)
	{
		lenv_put(e, syms -> cell[i], a -> cell[i+1]);
	}
	lval_del(a);
	
	return lval_sexpr();
}

/*builtin for the lambda function*/
lval *builtin_lambda(lenv *e, lval *a){
	/* Check Two arguments, each of which are Q-Expressions */
	LASSERT_NUM("\\", a, 2);
	LASSERT_TYPE("\\",a, 0, LVAL_QEXPR);
	LASSERT_TYPE("\\",a, 1, LVAL_QEXPR);

	/* Check first Q-Expression contains only Symbols */
	for (int i = 0; i < a -> cell[0] -> count; ++i)
	{
		LASSERT(a,(a -> cell[0] -> cell[i] -> type == LVAL_SYM), "Cannot define non-symbol, Got %s, Expected %s",
		ltype_name(a -> cell[0] -> cell[i] -> type), ltype_name(LVAL_SYM));
	}

	/* Pop first two arguments and pass them to lval_lambda */
	lval *formals = lval_pop(a, 0);
	lval *body = lval_pop(a, 0);
	lval_del(a);

	return lval_lambda(formals, body);

}

void lenv_add_builtins(lenv *e){
	
	/*List Functions */
	lenv_add_builtin(e, "list", builtin_list);
	lenv_add_builtin(e, "head", builtin_head);
	lenv_add_builtin(e, "tail", builtin_tail);
	lenv_add_builtin(e, "eval", builtin_eval);
	lenv_add_builtin(e, "join", builtin_join);
	lenv_add_builtin(e, "def", builtin_def);
	/* Mathematical Functions */
	lenv_add_builtin(e, "+", builtin_add);
	lenv_add_builtin(e, "-", builtin_sub);
	lenv_add_builtin(e, "*", builtin_mul);
	lenv_add_builtin(e, "/", builtin_div);
}

/*Function to evaluate nodes in the tree*/
lval *lval_eval_sexpr(lenv *e,lval *v){

	/* Evaluate Children */
	for (int i = 0; i < v -> count; ++i)
	{
		v -> cell[i] = lval_eval(e, v -> cell[i]);
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

	/*Ensure First Element is a f unction a fter evaluation*/
	lval *f = lval_pop(v, 0);
	if(f -> type != LVAL_FUN)
	{
		lval_del(f);
		lval_del(v);
		return lval_err("first element is not a builtintion");
	}

 	/* If so call builtin to get result */
	lval *result = f -> builtin(e, v);
	lval_del(f);
	return result;
}

lval *lval_eval(lenv *e,lval *v)
{
	if(v -> type == LVAL_SYM)
	{
		lval *x = lenv_get(e, v);
		lval_del(v);
		return x;
	}
	/* Evaluate Sexpression */
	if(v -> type == LVAL_SEXPR) {return lval_eval_sexpr(e, v);}

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

	lenv *e = lenv_new();
	lenv_add_builtins(e);
	
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
			
			lval *x = lval_eval(e, lval_read(r.output));
			lval_println(x);
			lval_del(x);
		
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

	lenv_del(e);	
	/* Undefine and  Delete our PARSERS */
	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
	return 0;

}
