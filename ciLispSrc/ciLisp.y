/*
program ::= s-expr EOL

s-expr ::=
     quit
   | number
   | symbol
   | ( func s_expr_list )
   | ( scope s_expr )
   | ( cond s_expr s_expr s_expr )

func ::=
   neg|abs|exp|sqrt|add|sub|mult|div|remainder|log|pow|max|min|exp2
   |cbrt|hypot|print|rand|read|equal|smaller|larger

scope ::= <empty> | ( let let_list )

let_list ::= let_elem | let_list let_elem

let_elem ::= ( type symbol s_expr )
type ::= <empty> | integer | real

number ::= digit+ [ . digit+ ]
symbol ::= letter+

letter ::= [a-zA-Z]
digit ::= 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
*/
%{
    #include "ciLisp.h"
%}

%union {
    double dval;
    char *sval;
    struct ast_node *astNode;
    struct symbol_table_node* symNode;
};

%token <sval> FUNC
%token <sval> SYMBOL
%token <sval> TYPE
%token <dval> NUMBER
%token LPAREN RPAREN EOL QUIT LET COND

%type <astNode> s_expr s_expr_list
%type <symNode> let_elem let_list scope

%%
program:
    s_expr EOL {
        fprintf(stderr, "yacc: program ::= s_expr EOL\n");
        if ($1) {
            printf("\n%lf", eval($1).value);
            //eval($1);
            freeNode($1);
        }
    };

scope:
    LPAREN LET let_list RPAREN {
        fprintf(stderr,"scope\n");
        $$ = $3;

    };
let_list:

    let_elem {
          fprintf(stderr,"let_elem\n");
          $$ = let_list($1, NULL);
    }
    | let_list let_elem{
            fprintf(stderr,"let_list let_elem\n");
            $$ = let_list($2, $1);
    };
let_elem:
    LPAREN TYPE SYMBOL s_expr RPAREN {
        fprintf(stderr,"LPAREN %s SYMBOL s_expr RPAREN\n", $2);
        $$ = let_elem($2, $3, $4);
    }
    | LPAREN SYMBOL s_expr RPAREN{
        fprintf(stderr,"LPAREN SYMBOL s_expr RPAREN\n");
        $$ = let_elem(NULL, $2, $3);
    };


s_expr:
    NUMBER {
        fprintf(stderr, "yacc: s_expr ::= NUMBER\n");
        $$ = number($1);
    }
    | LPAREN FUNC s_expr_list RPAREN{
        fprintf(stderr, "LPAREN func s_expr_list RPAREN\n");
        $$ = function($2,$3);
    }
    | LPAREN FUNC RPAREN{
        fprintf(stderr, "LPAREN FUNC RPAREN\n");
        $$ = function($2, NULL);
    }
    | SYMBOL {
          fprintf(stderr,"yacc: SYMBOL \n");
          $$ = symbol($1);
    }
    | LPAREN scope s_expr RPAREN{
        fprintf(stderr, "LPAREN SCOPE S_EXPR RPAREN\n");
        $$ = setScope($2,$3);
    }
    | LPAREN s_expr RPAREN {
        fprintf(stderr,"yacc: LPAREN s_expr RPAREN \n");
        $$ = $2;
    }
    | LPAREN COND s_expr_list RPAREN {
        fprintf(stderr, "yacc: s_expr ::= LPAREN COND s_expr_list RPAREN\n");
        $$ = conditional($3);
    }
    | QUIT {
        fprintf(stderr, "yacc: s_expr ::= QUIT\n");
        exit(EXIT_SUCCESS);
    }
    | error {
        fprintf(stderr, "yacc: s_expr ::= error\n");
        yyerror("unexpected token");
        $$ = NULL;
    };

s_expr_list:
    s_expr s_expr_list{
        fprintf(stderr, "s_expr s_expr_list\n");
        $$ = s_expr_list($1,$2);
    }
    | s_expr{
        fprintf(stderr, "s_expr\n");
        $$ = s_expr_list($1,NULL);
    };
%%

