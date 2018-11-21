%option noyywrap
%option nounput
%option noinput

%{
    #include "ciLisp.h"
%}

digit [0-9]
number {digit}+(\.{digit}+)?
func "neg"|"abs"|"exp"|"sqrt"|"add"|"sub"|"mult"|"div"|"remainder"|"log"|"pow"|"max"|"min"|"exp2"|"cbrt"|"hypot"|"print"|"rand"|"read"|"equal"|"smaller"|"larger"
letter [a-zA-Z]
symbol {letter}+
type "integer"|"real"
%%


"let" {
    fprintf(stderr, "lex: LET\n");
    return LET;
    }
"cond" {
    fprintf(stderr, "lex: COND\n");
    return COND;
}

"quit" {
    return QUIT;
    }
{number} {
    yylval.dval = strtod(yytext, NULL);
    fprintf(stderr, "lex: NUMBER dval = %lf\n", yylval.dval);
    return NUMBER;
    }
{type} {
    yylval.sval = (char *) malloc(strlen(yytext)*sizeof(char));
    strcpy(yylval.sval, yytext);
    fprintf(stderr, "lex: TYPE = %s\n", yylval.sval);
    return TYPE;
    }

{func} {
    yylval.sval = (char *) malloc(strlen(yytext)*sizeof(char));
    strcpy(yylval.sval, yytext);
    fprintf(stderr, "lex: FUNC sval = %s\n", yylval.sval);
    return FUNC;
    }

"(" {
    fprintf(stderr, "lex: LPAREN\n");
    return LPAREN;
    }

")" {
    fprintf(stderr, "lex: RPAREN\n");
    return RPAREN;
    }
{symbol} {
    yylval.sval = (char *) malloc(strlen(yytext)*sizeof(char));
    strcpy(yylval.sval, yytext);
    fprintf(stderr, "lex: SYMBOL sval = %s\n", yylval.sval);
    return SYMBOL;
    }


[\n] {
    fprintf(stderr, "lex: EOL\n");
    YY_FLUSH_BUFFER;
    return EOL;
    }

[ |\t] ; /* skip whitespace */


. { // anything else
    printf("ERROR: invalid character: >>%s<<\n", yytext);
    }

%%

//TODO: delete this!
ssize_t getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp)
{
	char *ptr, *eptr;


	if (*buf == NULL || *bufsiz == 0) {
		*bufsiz = BUFSIZ;
		if ((*buf = malloc(*bufsiz)) == NULL)
			return -1;
	}

	for (ptr = *buf, eptr = *buf + *bufsiz;;) {
		int c = fgetc(fp);
		if (c == -1) {
			if (feof(fp))
				return ptr == *buf ? -1 : ptr - *buf;
			else
				return -1;
		}
		*ptr++ = c;
		if (c == delimiter) {
			*ptr = '\0';
			return ptr - *buf;
		}
		if (ptr + 2 >= eptr) {
			char *nbuf;
			size_t nbufsiz = *bufsiz * 2;
			ssize_t d = ptr - *buf;
			if ((nbuf = realloc(*buf, nbufsiz)) == NULL)
				return -1;
			*buf = nbuf;
			*bufsiz = nbufsiz;
			eptr = nbuf + nbufsiz;
			ptr = nbuf + d;
		}
	}
}

ssize_t getline(char **buf, size_t *bufsiz, FILE *fp)
{
	return getdelim(buf, bufsiz, '\n', fp);
}

/*
 * DO NOT CHANGE THE FOLLOWING CODE CRITICAL FOR REPL!
 */
int main(void) {

    freopen("/dev/null", "w", stderr); // except for this line that can be uncommented to throw away debug printouts
    //srand(time(NULL)); //actually seeding random.
    srand(7); //7 is my favorite number!~~
    
    char *s_expr_str = NULL;
    size_t s_expr_str_len = 0;
    YY_BUFFER_STATE buffer;
    while (true) {
        printf("\n> ");
        getline(&s_expr_str, &s_expr_str_len, stdin);
        s_expr_str[s_expr_str_len++] = '\0';
        s_expr_str[s_expr_str_len++] = '\0';
        buffer = yy_scan_buffer(s_expr_str, s_expr_str_len);
        yyparse();
        yy_delete_buffer(buffer);
    }
}

//((let (a 100)) (cond (smaller (rand) 100) (add a 2) (sub a 2)))