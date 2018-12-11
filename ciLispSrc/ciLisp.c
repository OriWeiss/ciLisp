#include "ciLisp.h"
#include <math.h>
#include <regex.h>
//#include <time.h>

void yyerror(char *s) {
    printf("\nERROR: %s\n", s);
    // note stderr that normally defaults to stdout, but can be redirected: ./src 2> src.log
    // CLion will display stderr in a different color from stdin and stdout
}
// scope    ::= ( let let_list )
// let_list ::= let_elem | let_list let elem
// let_elem ::= ( symbol s-expr )
//
// find out which function it is
//

//
// Check is a string is a number
//
DATA_TYPE verifyNumber(char *string)
{
    DATA_TYPE rv = NO_TYPE;

    regex_t regex;

    if (regcomp(&regex, "[+-]?[0-9]+(\\.[0-9]+)?", REG_EXTENDED) != 0)
        fprintf(stderr, "Could not compile regex!\n");
    else if (regexec(&regex, string, 0, NULL, 0) == 0)
        return strchr(string, '.') == NULL ? INTEGER_TYPE : REAL_TYPE;
    else
        yyerror("Invalid input!\n");

    regfree(&regex);

    return rv;
}


SYMBOL_TABLE_NODE * findSymbolNodeByName(char *name, SYMBOL_TABLE_NODE *list)
{
    while (list != NULL) {
        if (strcmp(name,  list->ident) == 0)
            return list;
        list = list->next;
    }
    return NULL;
}

SYMBOL_TABLE_NODE *findSymbolWithScope(char *name,AST_NODE *p){
    if (p == NULL)
        return NULL;
    SYMBOL_TABLE_NODE * node = findSymbolNodeByName(name,p->scope);
    if (node != NULL) // correct node has been found
        return node;
    return findSymbolWithScope(name,p->parent); // else go to parent and respective scope
}


OPER_TYPE resolveFunc(char *func)
{
    char *funcs[] = {
            "neg", "abs", "exp", "sqrt", "add", "sub",
            "mult", "div", "remainder", "log", "pow",
            "max", "min","exp2","cbrt","hypot","print",
            "rand", "read", "equal", "smaller", "larger", ""
    };

    int i = 0;
    while (funcs[i][0] != '\0')
    {
        if (strcmp(funcs[i], func) == 0)
            return i;
        i++;
    }
    //yyerror("invalid function");
    return INVALID_OPER;
}

DATA_TYPE resolveType(char* type)
{
    if(type == NULL)
        return NO_TYPE;
    if (strcmp(type, "integer") == 0)
        return INTEGER_TYPE;
    if (strcmp(type, "real") == 0)
        return REAL_TYPE;
    return NO_TYPE; //if no type or wrong type is given
}

//
// create a node for a number
//
AST_NODE *number(char *val)
{
    double value = atof(val);
    AST_NODE *p;
    size_t nodeSize;

    // allocate space for the fixed sie and the variable part (union)
    nodeSize = sizeof(AST_NODE) + sizeof(NUMBER_AST_NODE);
    if ((p = calloc(1,nodeSize)) == NULL)
        yyerror("out of memory");

    p->type = NUM_TYPE;
    p->data.number.value = value;
    p->data.number.val_type = verifyNumber(val);

    return p;
}

AST_NODE *conditional(AST_NODE * s_expr_list)
{
    AST_NODE * p = calloc(1, sizeof(AST_NODE) + sizeof(COND_AST_NODE));
    if (p == NULL)
        yyerror("out of memory");

    p->type = COND_TYPE;
    p->data.condition.cond = s_expr_list;
    p->data.condition.nonzero = p->data.condition.cond->next;
    p->data.condition.zero = p->data.condition.nonzero->next;

    return p;
}


AST_NODE *function(char *funcName, AST_NODE *s_expr_list)
{
    AST_NODE *p;
    size_t nodeSize;

    // allocate space for the fixed sie and the variable part (union)
    nodeSize = sizeof(AST_NODE) + sizeof(FUNCTION_AST_NODE);
    if ((p = calloc(1,nodeSize)) == NULL)
        yyerror("out of memory");
    p->type = FUNC_TYPE;
    p->data.function.name = funcName;
    p->data.function.opList = s_expr_list;
    if(strcmp(funcName,"rand")==0){
        int random = rand();
        char * stringRep="";
        sprintf(stringRep,"%d" ,random);
        p->data.function.opList = number(stringRep);
    }
    else if(strcmp(funcName,"read")==0){
        char temp;
        printf("read := ");
        scanf("%s", &temp);
        getc(stdin); // remove the number inputted
        p->data.function.opList = number(&temp);
    }

    return p;
}

AST_NODE *s_expr_list(AST_NODE *s_expr, AST_NODE *list) {
    if (list == NULL)
        return s_expr;

    s_expr->next = list;
    list = s_expr;

    return list;
}


void freeScope(SYMBOL_TABLE_NODE *node){
    if(node == NULL){
        return;
    }

    free(node->ident); //string
    node->ident = NULL;
    freeNode(node->val);  //AST
    node->val = NULL;
    freeScope(node->next); //scope
    node->next = NULL;
    free(node);
}

void freeNode(AST_NODE *p)
{
    if (!p)
        return;
    freeScope(p->scope);
    if (p->type == FUNC_TYPE)
    {
        free(p->data.function.name);
        AST_NODE *list = p->data.function.opList;
        while(list != NULL){
            AST_NODE *next = list->next;
            freeNode(list);
            list = next;
        }
    }
    else if(p->type == SYM_TYPE){
        free(p->data.symbol.name);
    }
    else if (p->type == COND_TYPE)
    {
        freeNode(p->data.condition.cond);
        freeNode(p->data.condition.zero);
        freeNode(p->data.condition.nonzero);
    }
    free(p);
}

// create a symbol node
AST_NODE *symbol(char* name)
{
    AST_NODE *p;
    size_t nodeSize;

    // allocate space for the fixed sie and the variable part (union)
    nodeSize = sizeof(AST_NODE) + sizeof(SYMBOL_AST_NODE);
    if ((p = calloc(1,nodeSize)) == NULL)
        yyerror("out of memory");

    p->type = SYM_TYPE;
    p->data.symbol.name = name;
    return p;
}

// add the new symbol to the list and return it
SYMBOL_TABLE_NODE* let_list(SYMBOL_TABLE_NODE *symbol, SYMBOL_TABLE_NODE *list)
{
    if(list == NULL)
        return symbol;

    SYMBOL_TABLE_NODE *node = findSymbolNodeByName(symbol->ident, list); // checking for re-declaration
    if (node == NULL)
    // if node = NULL, no symbol has been found with same name
    {
        symbol->next = list;
    }

    return symbol;
}

// create a new symbol and return it
SYMBOL_TABLE_NODE *let_elem(SYMBOL_TYPE symType, char* type, char* symbol, SYMBOL_TABLE_NODE *args, AST_NODE *s_expr)
{
    SYMBOL_TABLE_NODE *p;
    size_t nodeSize;

    // allocate space the symbol
    nodeSize = sizeof(SYMBOL_TABLE_NODE);
    if ((p = calloc(1,nodeSize)) == NULL)
        yyerror("out of memory");



    if(s_expr->scope !=NULL){ //
        SYMBOL_TABLE_NODE *sScope = s_expr->scope;
        while(true){
            if(sScope->next ==NULL){
                sScope->next = args;
                break;
            }
            sScope = sScope->next;
        }

    }
    else{
        s_expr->scope = args;
    }

    if(s_expr->type == NUM_TYPE && (s_expr->data.number.val_type == REAL_TYPE )&& (resolveType(type) == INTEGER_TYPE)){
        printf("Warning integer type is assigned to a real number\n");
    }
    p->type = symType;
    p->val_type = resolveType(type);
    p->ident = symbol;
    p->args = args;
    p->val = s_expr;
    if(s_expr->type == NUM_TYPE && (s_expr->data.number.val_type == REAL_TYPE )&& (resolveType(type) == INTEGER_TYPE)){

    }
    return p;
}

SYMBOL_TABLE_NODE *arg_list (char *symbol, SYMBOL_TABLE_NODE *list){
    SYMBOL_TABLE_NODE *p;
    size_t nodeSize;

    // allocate space for the stack
    nodeSize = sizeof(SYMBOL_TABLE_NODE);
    if ((p = calloc(1,nodeSize)) == NULL)
        yyerror("out of memory");

    if(list == NULL) {
        p->ident = symbol;
        p->type = ARG_TYPE;
        return p;
    }
    SYMBOL_TABLE_NODE *node = findSymbolNodeByName(symbol, list); //check to see if variable already exists
    if (node == NULL) {
        p->next= list;
        p->ident = symbol;
        p->type = ARG_TYPE;
    }
    return p;
}


AST_NODE *setScope(SYMBOL_TABLE_NODE *scope, AST_NODE *sExpr){

    sExpr->scope = scope;
    return sExpr;
}

void setParent(AST_NODE *p){
    if(p->type == FUNC_TYPE){
        AST_NODE *list = p->data.function.opList;
        while (list != NULL) {
            list->parent = p;
            list = list->next;
        }

    }
    else if(p->type == COND_TYPE){
        p->data.condition.zero->parent = p;
        p->data.condition.nonzero->parent = p;
        p->data.condition.cond->parent = p;
    }
}

RETURN_VALUE eval_func(OPER_TYPE operation, RETURN_VALUE op1, RETURN_VALUE op2)
{

    switch(operation) {
        case NEG:
            if(op1.type== INTEGER_TYPE && op2.type == INTEGER_TYPE){
                return (RETURN_VALUE){INTEGER_TYPE, round(-op1.value)};
            }
            else{
                return (RETURN_VALUE){REAL_TYPE, -op1.value};
            }
        case ABS:
            return (RETURN_VALUE){REAL_TYPE, fabs(op1.value)};
        case EXP:
            return (RETURN_VALUE){REAL_TYPE, exp(op1.value)};
        case SQRT:
            return (RETURN_VALUE){REAL_TYPE, sqrt(op1.value)};
        case ADD:
            if(op1.type== INTEGER_TYPE && op2.type == INTEGER_TYPE){
                return (RETURN_VALUE){INTEGER_TYPE, round(op1.value + op2.value)};
            }
            else{
                return (RETURN_VALUE){REAL_TYPE, op1.value + op2.value};
            }
        case SUB:
            if(op1.type== INTEGER_TYPE && op2.type == INTEGER_TYPE){
                return (RETURN_VALUE){INTEGER_TYPE, round(op1.value - op2.value)};
            }
            else{
                return (RETURN_VALUE){REAL_TYPE, op1.value - op2.value};
            }
        case MULT:
            if(op1.type== INTEGER_TYPE && op2.type == INTEGER_TYPE){
                return (RETURN_VALUE){INTEGER_TYPE, round(op1.value * op2.value)};
            }
            else{
                return (RETURN_VALUE){REAL_TYPE, op1.value * op2.value};
            }
        case DIV:
            if (op2.value == 0.0) {
                yyerror("Error, division by 0");
                return (RETURN_VALUE){NO_TYPE, 0.0};
            }
            return (RETURN_VALUE){REAL_TYPE, op1.value / op2.value};
        case REMAINDER:
            return (RETURN_VALUE){REAL_TYPE, remainder(op1.value, op2.value)};
        case LOG:
            return (RETURN_VALUE){REAL_TYPE, log10(op1.value)};
        case POW:
            return (RETURN_VALUE){REAL_TYPE, pow(op1.value,op2.value)};
        case MAX:
            return (RETURN_VALUE){REAL_TYPE, fmax(op1.value, op2.value)};
        case MIN:
            return (RETURN_VALUE){REAL_TYPE, fmin(op1.value, op2.value)};
        case EXP2:
            return (RETURN_VALUE){REAL_TYPE, exp2(op1.value)};
        case CBRT:
            return (RETURN_VALUE){REAL_TYPE, cbrt(op1.value)};
        case HYPOT:
            return (RETURN_VALUE){REAL_TYPE, hypot(op1.value, op2.value)};
        case PRINT:
            if(op1.type == INTEGER_TYPE){
                printf(" %d", (int)op1.value);
            }
            else { //if type is a double
                printf(" %lf", op1.value);
            }
            return op1;
        case RAND:
            return op1;
        case READ:
            return op1;
        case EQUAL:
            return (RETURN_VALUE){NO_TYPE, (op1.value) == (op2.value)};
        case SMALLER:
            return (RETURN_VALUE){NO_TYPE, (op1.value) < (op2.value)};
        case LARGER:
            return (RETURN_VALUE){NO_TYPE, (op1.value) > (op2.value)};

        default:
            yyerror("NOT A FUNCTION");
    }
    return (RETURN_VALUE){NO_TYPE, 0.0};
}

bool realToInteger(DATA_TYPE lhs, DATA_TYPE rhs)
{
    return lhs == INTEGER_TYPE && rhs == REAL_TYPE;
}

bool integerToReal(DATA_TYPE lhs, DATA_TYPE rhs)
{
    return realToInteger(rhs, lhs);
}

RETURN_VALUE typeConversions(RETURN_VALUE val, DATA_TYPE val_type, char * name)
{
    if (realToInteger(val_type, val.type))
    {
        val.type = REAL_TYPE;
        printf("WARNING: incompatible type assignment for variable %s\n", name);
    }
        //implicit conversion to real: (node)real -> (eval)integer
    else if (integerToReal(val_type, val.type))
    {
        val.type = REAL_TYPE;
    }
    else val.type = val_type;

    return val;
}

RETURN_VALUE evalSymbol(char *name, AST_NODE *symbol)
{


    //try to find node in this scope.
    SYMBOL_TABLE_NODE * node = findSymbolWithScope(name, symbol);
    RETURN_VALUE val = {NO_TYPE, 0.0};
    //if in this scope
    if (node != NULL) {
        if(node->type == ARG_TYPE){
            val = eval(node->stack->val);
        }
        else {
            val = eval(node->val);
        }
        //incompatible types: (node)integer -> (eval)real
        if (node->val_type == INTEGER_TYPE)
            val.value = round(val.value);
        return typeConversions(val, node->val_type, name);
    }


    //no more scopes to check, DNE
    printf("Could not find the symbol %s\n",name);
    return (RETURN_VALUE) {NO_TYPE, 0.0};
}

int countList(AST_NODE *opList){
    int i = 0;
    AST_NODE *curr = opList;
    while(curr!=NULL){
        i++;
        curr = curr->next;
    }
    return i;
}




RETURN_VALUE userFunction(AST_NODE *p)
{
    SYMBOL_TABLE_NODE *lambda = NULL;


    lambda = findSymbolWithScope(p->data.function.name,p);

    int numOfArgs = 0;
    SYMBOL_TABLE_NODE *args  = lambda->args;
    while(args!=NULL){ //count args
        numOfArgs++;
        args=args->next;
    }

    AST_NODE *opList = p->data.function.opList;
    int numOfOps = countList(opList);

    if(numOfOps > numOfArgs){
        //BAD
        yyerror("There are more operands than arguments");
        return (RETURN_VALUE){ NO_TYPE, 0.0 };
    }
    else if (numOfOps < numOfArgs){
        //WARNING
        printf("Warning there are more arguments than operands\n");
    }
    args = lambda->args;
    opList = p->data.function.opList;
    //pushing elements onto stack
    while(args!=NULL){
        STACK_NODE *temp = args->stack;
        args->stack = calloc(1, sizeof(STACK_NODE));
        args->stack->next = temp;
        args->stack->val = opList;

        args = args->next;
        opList = opList->next;
    }


    RETURN_VALUE val = eval(lambda->val);
    args = lambda->args; //reset args
     //added args stack !=NULL
    STACK_NODE *temp = args->stack;
    args->stack = args->stack->next;
    free(temp);

    return val;
}


RETURN_VALUE specialFuncEval(char *name, AST_NODE *p){


    RETURN_VALUE val = {NO_TYPE, 0.0};
    AST_NODE *opList = p->data.function.opList;
    OPER_TYPE type = resolveFunc(name);
    if(type == INVALID_OPER){


        val = userFunction(p);

    }
    else {


        AST_NODE *curr = opList;
        int listLength = countList(opList);
        if (type == ADD || type == MULT) {
            if (listLength < 2) { //error handling too few arguments
                printf("ERROR: too few parameters for the function %s", name);
                return (RETURN_VALUE) {REAL_TYPE, 0.0};
            }
            val = eval_func(type, eval(curr), eval(curr->next));
            curr = curr->next->next;
            while (curr != NULL) {
                val = eval_func(type, val, eval(curr));
                curr = curr->next;
            }
        } else if (type == PRINT) {
            printf("=>");
            while (curr != NULL) {
                val = eval_func(type, eval(curr), val);
                curr = curr->next;
            }
        } else {
            if (listLength > 2) { //error handling too few arguments
                printf("WARNING: too many parameters for the function %s", name);
                return (RETURN_VALUE) {REAL_TYPE, 0.0};
            }
            val = eval_func(type, eval(curr), eval(curr->next));

        }
    }
    return val;

}

RETURN_VALUE evalCondition(AST_NODE * p)
{
    RETURN_VALUE val = eval(p->data.condition.cond);
    if (val.value == 0.0)
        return eval(p->data.condition.zero);
    else
        return eval(p->data.condition.nonzero);
}

//
// evaluate an abstract syntax tree
//
// p points to the root
//
RETURN_VALUE eval(AST_NODE *p)
{
    if (!p)
        return (RETURN_VALUE){REAL_TYPE, 0.0};

    setParent(p);

    switch(p->type){
        case NUM_TYPE:
            return (RETURN_VALUE){NO_TYPE, p->data.number.value };
        case SYM_TYPE:
            return evalSymbol(p->data.symbol.name,p);
        case FUNC_TYPE:
            return specialFuncEval(p->data.function.name,p);
        case COND_TYPE:
            return evalCondition(p);
        default:
            return (RETURN_VALUE){NO_TYPE, 0.0}; //shouldn't get here.
    }
}
