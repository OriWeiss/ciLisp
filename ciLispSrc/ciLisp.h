#ifndef __cilisp_h_
#define __cilisp_h_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <time.h> //for seeding random numbers.
#include <stdbool.h>

#include "ciLispParser.h"

int yyparse(void);

int yylex(void);

void yyerror(char *);

typedef enum oper { // must be in sync with funcs in resolveFunc()
    NEG,
    ABS,
    EXP,
    SQRT,
    ADD,
    SUB,
    MULT,
    DIV,
    REMAINDER,
    LOG,
    POW,
    MAX,
    MIN,
    EXP2,
    CBRT,
    HYPOT,
    PRINT,
    RAND,
    READ,
    EQUAL,
    SMALLER,
    LARGER,
    INVALID_OPER=255
} OPER_TYPE;

typedef enum {
    VARIABLE_TYPE,
    LAMBDA_TYPE,
    ARG_TYPE
} SYMBOL_TYPE;

typedef enum {
    NO_TYPE,
    INTEGER_TYPE,
    REAL_TYPE
} DATA_TYPE;

typedef enum {
    NUM_TYPE, 
    FUNC_TYPE, 
    SYM_TYPE, 
    COND_TYPE
} AST_NODE_TYPE;

typedef struct {
    double value;
    DATA_TYPE val_type;
} NUMBER_AST_NODE;

typedef struct stack_node {
    struct ast_node *val;
    struct stack_node *next;
} STACK_NODE;

typedef struct {
    char *name;
    struct ast_node *opList; //was op1 and op2
} FUNCTION_AST_NODE;

typedef struct return_value {
    DATA_TYPE type;
    double value;
} RETURN_VALUE;

typedef struct symbol_table_node {
    SYMBOL_TYPE type;
    DATA_TYPE val_type;
    struct symbol_table_node *args;
    char *ident;
    struct ast_node *val;
    STACK_NODE *stack;
    struct symbol_table_node *next;
} SYMBOL_TABLE_NODE;

typedef struct symbol_ast_node {
    char *name;
} SYMBOL_AST_NODE;

typedef struct cond_ast_node {
  struct ast_node *cond;
  struct ast_node *zero;
  struct ast_node *nonzero;
} COND_AST_NODE;

typedef struct ast_node {
    AST_NODE_TYPE type;
    SYMBOL_TABLE_NODE *scope;
    struct ast_node *parent;
    union {
        NUMBER_AST_NODE number;
        FUNCTION_AST_NODE function;
        SYMBOL_AST_NODE symbol;
        COND_AST_NODE condition;
    } data;
    struct ast_node *next;
} AST_NODE;

//
// Check is a string is a number
//
DATA_TYPE verifyNumber(char *string);






AST_NODE *number(char *val);

AST_NODE *conditional(AST_NODE *s_expr_list);

void freeNode(AST_NODE *p);

RETURN_VALUE eval(AST_NODE *);

void freeScope(SYMBOL_TABLE_NODE *node);

AST_NODE *symbol(char* name);

SYMBOL_TABLE_NODE* let_list(SYMBOL_TABLE_NODE *symbol, SYMBOL_TABLE_NODE *list);

SYMBOL_TABLE_NODE* let_elem(SYMBOL_TYPE symType, char* type, char* symbol, SYMBOL_TABLE_NODE *args, AST_NODE *s_expr);

AST_NODE *setScope(SYMBOL_TABLE_NODE *scope, AST_NODE *sExpr);

RETURN_VALUE eval_func(OPER_TYPE operation, RETURN_VALUE op1 ,RETURN_VALUE op2);

RETURN_VALUE evalSymbol(char *name,AST_NODE *sybmol);

void setParent(AST_NODE *p);

AST_NODE *s_expr_list(AST_NODE *s_expr, AST_NODE *list);

AST_NODE *function(char *funcName, AST_NODE *s_expr_list);

RETURN_VALUE specialFuncEval(char *name, AST_NODE *opList);

SYMBOL_TABLE_NODE *arg_list (char *symbol, SYMBOL_TABLE_NODE *list);

#endif

//((let (f lambda (x y) (add x y)))(f (sub 5 2) (mult 2 3))