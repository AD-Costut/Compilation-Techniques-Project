//https://web.archive.org/web/20201013084641/https://sites.google.com/site/razvanaciu/compilation-techniques
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

enum
{
    ID,CT_INT,CT_REAL,CT_CHAR,CT_STRING,COMMA,SEMICOLON,
    LPAR,RPAR,LBRACKET,RBRACKET,LACC,RACC,ADD,SUB,MUL,DIV,
    DOT,AND,OR,NOT,ASSIGN,EQUAL,NOTEQ,LESS,LESSEQ,GREATER,GREATEREQ,
    END,STRUCT,WHILE,IF,VOID,ELSE,FOR,
    BREAK,RETURN,DOUBLE,INT
};

int line = 1;
int crtDepth = 0;

typedef struct _Token
{
    int code; // Code (name)
    union
    {
        char *text; // Used for ID, CT_STRING (dynamically allocated)
        long int i; // Used for CT_INT, CT_CHAR
        double r;   // Used for CT_REAL
    };
    int line;            // Input file line
    struct _Token *next; // Link to the next token
} Token;

Token *tokens = NULL;
Token *lastToken = NULL;
Token *crtTk;
Token *consumedTk;

#define SAFEALLOC(var, Type)                          \
    if ((var = (Type *)malloc(sizeof(Type))) == NULL) \
        err("not enough memory");

void err(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, va);
    fputc('\n', stderr);
    va_end(va);
    exit(-1);
}

char *createString(const char *start, const char *end)
{
    size_t length = end - start;
    char *str = (char *)malloc(length + 1);
    if (str == NULL)
    {
        err("not enough memory");
    }
    strncpy(str, start, length);
    str[length] = '\0';
    return str;
}

void tkerr(const Token *tk, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    fprintf(stderr, "error in line %d: ", tk->line);
    vfprintf(stderr, fmt, va);
    fputc('\n', stderr);
    va_end(va);
    exit(-1);
}

char escaped(char ch)
{
    char escapedCh;
    switch (ch)
    {
    case 'a':
        escapedCh = '\a';
        break;
    case 'b':
        escapedCh = '\b';
        break;
    case 'f':
        escapedCh = '\f';
        break;
    case 'n':
        escapedCh = '\n';
        break;
    case 'r':
        escapedCh = '\r';
        break;
    case 't':
        escapedCh = '\t';
        break;
    case '0':
        escapedCh = '\0';
        break;
    }
    return escapedCh;
}

Token *addTk(int code)
{
    Token *tk;
    SAFEALLOC(tk, Token);
    tk->code = code;
    tk->line = line;
    tk->next = NULL;
    if (lastToken)
    {
        lastToken->next = tk;
    }
    else
    {
        tokens = tk;
    }
    lastToken = tk;
    return tk;
}


int getNextToken(char *input)
{
    int state = 0;
    char ch;
    char *pStartCh, *pCrtCh = input;
    Token *tk;
    while (1)
    {
        ch = (*pCrtCh);
        switch (state)
        {
        case 0:
            pStartCh = pCrtCh;
            if (ch == '\n')
            {
                line++;
                pCrtCh++;
            }
            else if (isalpha(ch) || ch == '_')
            {
                pCrtCh++;
                state = 1;
            }
            else if (ch >= '1' && ch <= '9')
            {
                pCrtCh++;
                state = 3;
            }
            else if (ch == '0')
            {
                pCrtCh++;
                state = 5;
            }
            else if(ch == ' ' || ch == '\t' || ch == '\r') {
				pCrtCh++;
			}
            else if (ch == '/') {
                pCrtCh++;
                state = 20;
            }
            else if(ch == '\'') {
					pCrtCh++;
					state = 14;
				}
			else if(ch == '\"') {
					pCrtCh++;
					state = 17;
				}
            else if (ch == ',')
            {
                pCrtCh++;
                addTk(COMMA);
            }
            else if (ch == ';')
            {
                pCrtCh++;
                addTk(SEMICOLON);
            }
            else if (ch == '(')
            {
                pCrtCh++;
                addTk(LPAR);
            }
            else if (ch == ')')
            {
                pCrtCh++;
                addTk(RPAR);
            }
            else if (ch == '[')
            {
                pCrtCh++;
                addTk(LBRACKET);
            }
            else if (ch == ']')
            {
                pCrtCh++;
                addTk(RBRACKET);
            }
            else if (ch == '{')
            {
                pCrtCh++;
                addTk(LACC);
            }
            else if (ch == '}')
            {
                pCrtCh++;
                addTk(RACC);
            }
            else if (ch == '+')
            {
                pCrtCh++;
                addTk(ADD);
            }
            else if (ch == '-')
            {
                pCrtCh++;
                addTk(SUB);
            }
            else if (ch == '*')
            {
                pCrtCh++;
                addTk(MUL);
            }
            else if (ch == '.')
            {
                pCrtCh++;
                addTk(DOT);
            }
            else if (ch == '&')
            {
                pCrtCh++;
                ch = *pCrtCh;
                if (ch == '&')
                {
                    pCrtCh++;
                    addTk(AND);
                }
                else {
                    tkerr(tk, "Expected binary operator column %s\n", pStartCh);
                }
            }
            else if (ch == '|')
            {
                pCrtCh++;
                ch = *pCrtCh;
                if (ch == '|')
                {
                    pCrtCh++;
                    addTk(OR);
                }
                else
                    tkerr(tk, "Expected binary operator column %s\n", pCrtCh);
            }
            else if (ch == '!')
            {
                pCrtCh++;
                ch = *pCrtCh;
                if (ch == '=')
                {
                    pCrtCh++;
                    addTk(NOTEQ);
                }
                else
                {
                    addTk(NOT);
                }
            }
            else if (ch == '=')
            {
                pCrtCh++;
                ch = *pCrtCh;
                if (ch == '=')
                {
                    pCrtCh++;
                    addTk(EQUAL);
                }
                else
                {
                    addTk(ASSIGN);
                }
            }
            else if (ch == '<')
            {
                pCrtCh++;
                ch = *pCrtCh;
                if (ch == '=')
                {
                    pCrtCh++;
                    addTk(LESSEQ);
                }
                else
                {
                    addTk(LESS);
                }
            }
            else if (ch == '>')
            {
                pCrtCh++;
                ch = *pCrtCh;
                if (ch == '=')
                {
                    pCrtCh++;
                    addTk(GREATEREQ);
                }
                else
                {
                    addTk(GREATER);
                }
            }
            else if (ch == '\0')
            {
                addTk(END);
                return 0;
            }
            else {
                printf("Intra in else-ul asta de la sfarsit de la cazul 0\n");
                tkerr(tk, "Error");
            }
            break;
        case 1:
				if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_') {
					if(!memcmp(pStartCh, "break", 5)) {
						tk=addTk(BREAK);
						pCrtCh+= 3;
						state = 0;
					} else if(!memcmp(pStartCh, "char", 4)) {
						tk = addTk(CT_CHAR);
						pCrtCh+= 2;
						state = 0;
					} else if(!memcmp(pStartCh, "double", 6)) {
						tk = addTk(DOUBLE);
						pCrtCh+= 4;
						state = 0;
					} else if(!memcmp(pStartCh, "else", 4)) {
						tk = addTk(ELSE);
						pCrtCh+= 2;
						state = 0;
					} else if(!memcmp(pStartCh, "for", 3)) {
						tk = addTk(FOR);
						pCrtCh+= 1;
						state = 0;
					} else if(!memcmp(pStartCh, "if", 2)) {
						tk = addTk(IF);
						state = 0;
					} else if(!memcmp(pStartCh, "int", 3)) {
						tk = addTk(INT);
						pCrtCh+= 2;
						state = 0;
					} else if(!memcmp(pStartCh, "return", 6)) {
						tk = addTk(RETURN);
						pCrtCh+= 4;
						state = 0;
					} else if(!memcmp(pStartCh, "struct", 6)) {
						tk = addTk(STRUCT);
						pCrtCh+= 4;
						state = 0;
					} else if(!memcmp(pStartCh, "void", 4)) {
						tk = addTk(VOID);
						pCrtCh+= 2;
						state = 0;
					} else if(!memcmp(pStartCh, "while", 5)) { 
						tk = addTk(WHILE);
						pCrtCh+= 3;
						state = 0;
					}
					pCrtCh++;
				}
				else {
					state = 2;
				}
				break;
        case 2:
            tk = addTk(ID);
            char *str = createString(pStartCh, pCrtCh);
            tk->text = str;
            state = 0;
            break;
        case 3:
            if (ch >= '0' && ch <= '9')
            {
                pCrtCh++;
                state = 3;
            }
            else if (ch == '.')
            {
                pCrtCh++;
                state = 8;
            }
            else if (ch == 'e' || ch == 'E')
            {
                pCrtCh++;
                state = 10;
            }
            else
                state = 4;
            break;
        case 4:
            tk = addTk(CT_INT);
            tk->i = strtol(pStartCh, NULL, 0);
            state = 0;
            break;
        case 5:
            if (ch >= '0' && ch <= '7')
            {
                pCrtCh++;
            }
            else if (ch == 'x' || ch == 'X')
            {
                pCrtCh++;
                state = 6;
            }
            else if (ch == '.')
            {
                pCrtCh++;
                state = 8;
            }
            else if (ch == 'e' || ch == 'E')
            {
                pCrtCh++;
                state = 10;
            }
            else if (ch >= 8 && ch <= 9)
            {
                pCrtCh++;
                state = 7;
            }
            else
                state = 4;
            break;
        case 6:
            if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))
            {
                pCrtCh++;
                state = 6;
            }
            else
            {
                state = 4;
            }
            break;
        case 7:
            if(ch == '.')
            {
                pCrtCh++;
                state = 8;
            }
            else if (ch == 'e' || ch == 'E')
            {
                pCrtCh++;
                state = 10;
            }
        case 8:
            if (ch >= '0' && ch <= '9') {
                pCrtCh++;
                state = 9;
            } else {
                tkerr(tk, "After '.' a digit is expected");
            }
            break;
        case 9:
            if (ch >= '0' && ch <= '9') {
                pCrtCh++;
            } else if (ch == 'e' || ch == 'E') {
                pCrtCh++;
                state = 10;
            } else {
                state = 13;
            }
            break;
        case 10:
            if (ch == '+' || ch == '-') {
                pCrtCh++;
                state = 11;
            } else if (ch >= '0' && ch <= '9') {
                pCrtCh++;
                state = 12;
            } else {
                tkerr(tk, "State 10: Expected number, + or - sign");
            }
            break;
        case 11:
            if (ch >= '0' && ch <= '9') {
                pCrtCh++;
                state = 12;
            } else {
                tkerr(tk, "State 11: Expected digit after exponent sign");
            }
            break;
        case 12:
            if (ch >= '0' && ch <= '9') {
                pCrtCh++;
                state = 12;
            } else {
                state = 13;
            }
            break;
        case 13:
            tk = addTk(CT_REAL);
            tk->r = strtod(pStartCh, NULL);
            state = 0;
            break;
        case 14:
            if (ch == '\\')
            {
                pCrtCh++;
                state = 15;
            }
            else if (ch != '\\' && ch != '\'')
            {
                pCrtCh++;
                state = 16;
            }
            break;
        case 15:
            if (strchr("abfnrtv'?\"\\0", ch))
            {
                pCrtCh++;
                state = 16;
            }
            else
            {
                tkerr(tk, "State 15: Char expected\n");
            }
            break;
        case 16:
            if (ch == '\'')
            {
                pCrtCh++;
                state = 0;
                tk = addTk(CT_CHAR);
                char c[2];
                c[0] = *(pCrtCh - 2);
                c[1] = '\0';
                char *p;
                if ((p = strchr(c, '\\')) != NULL)
                {
                    memmove(p, p + 1, strlen(p));
                    if ((*p != '\'') && (*p != '\?') && (*p != '\"') && (*p != '\\'))
                        *p = escaped(*p);
                }
                tk->text = c;
                char *str = createString(pStartCh + 1, pCrtCh - 1);
            }
            else
                tkerr(tk, "State 16: Expected character");
            break;

        case 17:
            if (ch == '\\')
            {
                pCrtCh++;
                state = 18;
            }
            else if (ch != '\\' && ch != '\'')
            {
                state = 19;
            }
            break;
        case 18:
            if (strchr("abfnrtv'?\"\\0", ch))
            {
                pCrtCh++;
                state = 19;
            }
            else
                tkerr(tk, "Not escaped char\n");
            break;
        case 19:
            if (ch == '\"')
            {
                tk = addTk(CT_STRING);
                char *str = createString(pStartCh + 1, pCrtCh);
                char *p;
                while ((p = strchr(str, '\\')) != NULL)
                {
                    memmove(p, p + 1, strlen(p));
                    if ((*p != '\'') && (*p != '\?') && (*p != '\"') && (*p != '\\'))
                        *p = escaped(*p);
                }
                pCrtCh++;
                tk->text = str;
                state = 0;

            }
            else
            {
                state = 17;
                pCrtCh++;
            }
            break;
        
        case 20:
				if(ch == '*') {
					pCrtCh++;
					state = 21; 
				}
				else if(ch == '/'){
					pCrtCh++;
					state = 23; 
				}
				else {
					addTk(DIV);
					state = 0;
				}
				break;
			case 21:
				if(ch == '*'){
					pCrtCh++;
					state = 22;
				}
				else if (ch == '\n'){
					pCrtCh++;
					line++;
				}
				else{
					pCrtCh++;
                    state = 21;
                }
				break;
			case 22:
				if(ch == '/') {
					pCrtCh++;
					state = 0; 
				}
				else if(ch == '*') {
					pCrtCh++;
                    state = 22;
				}
				else if (ch != '*' && ch != '/'){
					pCrtCh++;
					state = 21;
				}
				else
					tkerr(tk, "Error from state 22");
				break;
			case 23:
				if(ch !='\n' && ch !='\r' && ch !='\0'){
					pCrtCh++;
				}
				else if(ch=='\n') {
					pCrtCh++;
					state = 0;
					line++;
				}
				else
				{
					pCrtCh++;
					state = 0;
				} 
				break;
        default:
            printf("Error state %i value %c", state, ch);
            tkerr(tk, "Error\n");
        }
    }
    return 0;
}

int open_file(char *filename)
{
    int file = open(filename, O_RDWR);
    if (file == -1)
    {
        perror("Error opening file");
        return -1;
    }
    return file;
}


int consume(int code)
{
    if (crtTk->code == code)
    {
        consumedTk = crtTk;
        crtTk = crtTk->next;
        return 1;
    }
    return 0;
}

// unit: ( declStruct | declFunc | declVar )* END
int unit()
{
    crtTk = tokens;
    while (1)
    {
        if (declStruct())
        {
        }
        else if (declFunc())
        {
        }
        else if (declVar())
        {
        }
        else
            break;
    }
    if (!consume(END))
        tkerr(crtTk, "missing END token");
    return 1;
}

// declStruct: STRUCT ID LACC declVar* RACC SEMICOLON
int declStruct()
{
    Token *startTk = crtTk;
    if (!consume(STRUCT))
        return 0;
    if (!consume(ID))
        tkerr(crtTk, "ID expected after struct");
    if (!consume(LACC))
    {
        crtTk = startTk;
        return 0;
    }
    while (1)
    {
        if (declVar())
        {
        }
        else
            break;
    }
    if (!consume(RACC))
        tkerr(crtTk, "Missing { in struct declaration");
    if (!consume(SEMICOLON))
        tkerr(crtTk, "Missing ; in struct declaration");
    return 1;
}

// declVar:  typeBase ID arrayDecl? ( COMMA ID arrayDecl? )* SEMICOLON
int declVar()
{
    Token *startTk = crtTk;
    if (!typeBase())
        return 0;
    if (!consume(ID))
        tkerr(crtTk, "ID expected after type base");
    if (!arrayDecl())
    {
    }
    while (1)
    {
        if (!consume(COMMA))
            break;
        if (!consume(ID))
            tkerr(crtTk, "ID expected");
        if (!arrayDecl())
        {
        }
    }
    if (!consume(SEMICOLON))
    {
        crtTk = startTk;
        return 0;
    }
    return 1;
}

// typeBase: INT | DOUBLE | CHAR | STRUCT ID
int typeBase()
{
    if (consume(INT))
    {
    }
    else if (consume(DOUBLE))
    {
    }
    else if (consume(CT_CHAR))
    {
    }
    else if (consume(STRUCT))
    {
        if (!consume(ID))
            tkerr(crtTk, "ID expected after struct");
    }
    else
        return 0;
    return 1;
}

// arrayDecl: LBRACKET expr? RBRACKET
int arrayDecl()
{
    if (!consume(LBRACKET))
        return 0;
    if (expr())
    {
    }
    if (!consume(RBRACKET))
        tkerr(crtTk, "missing ] from array declaration");
    return 1;
}

// typeName: typeBase arrayDecl?
int typeName()
{
    if (!typeBase())
        return 0;
    if (!arrayDecl())
    {
    }
    return 1;
}

// declFunc: ( typeBase MUL? | VOID ) ID
//                         LPAR ( funcArg ( COMMA funcArg )* )? RPAR
//                         stmCompound
int declFunc()
{
    Token *startTk = crtTk;

    if (typeBase())
    {
        if (consume(MUL))
        {
        }
    }
    else if (consume(VOID))
    {
    }
    else
        return 0;
    if (!consume(ID))
    {
        crtTk = startTk;
        return 0;
    }
    if (!consume(LPAR))
    {
        crtTk = startTk;
        return 0;
    }
    if (funcArg())
    {
        while (1)
        {
            if (consume(COMMA))
            {
                if (!funcArg())
                    tkerr(crtTk, "missing func arg in stm");
            }
            else
                break;
        }
    }
    if (!consume(RPAR))
        tkerr(crtTk, "missing ) in func declaration");
    if (!stmCompound())
        tkerr(crtTk, "compound statement expected");

    return 1;
}

// funcArg: typeBase ID arrayDecl?
int funcArg()
{
    if (!typeBase())
        return 0;
    if (!consume(ID))
        tkerr(crtTk, "ID missing in function declaration");
    if (!arrayDecl())
    {
    }
    return 1;
}

// stm: stmCompound
//            | IF LPAR expr RPAR stm ( ELSE stm )?
//            | WHILE LPAR expr RPAR stm
//            | FOR LPAR expr? SEMICOLON expr? SEMICOLON expr? RPAR stm
//            | BREAK SEMICOLON
//            | RETURN expr? SEMICOLON
//            | expr? SEMICOLON
int stm()
{
    if (stmCompound())
    {
    }
    else if (consume(IF))
    {
        if (!consume(LPAR))
            tkerr(crtTk, "missing ( after if");
        if (!expr())
            tkerr(crtTk, "Expected expression after ( ");
        if (!consume(RPAR))
            tkerr(crtTk, "missing ) after if");
        if (!stm())
            tkerr(crtTk, "Expected statement after if ");
        if (consume(ELSE))
        {
            if (!stm())
                tkerr(crtTk, "Expected statement after else ");
        }
    }
    else if (consume(WHILE))
    {
        if (!consume(LPAR))
            tkerr(crtTk, "missing ( after while");
        if (!expr())
            tkerr(crtTk, "Expected expression after ( ");
        if (!consume(RPAR))
            tkerr(crtTk, "missing ) after while");
        if (!stm())
            tkerr(crtTk, "Expected statement after while ");
    }
    else if (consume(FOR))
    {
        if (!consume(LPAR))
            tkerr(crtTk, "missing ( after for");
        expr();
        if (!consume(SEMICOLON))
            tkerr(crtTk, "missing ; in for");
        expr();
        if (!consume(SEMICOLON))
            tkerr(crtTk, "missing ; in for");
        expr();
        if (!consume(RPAR))
            tkerr(crtTk, "missing ) after for");
        if (!stm())
            tkerr(crtTk, "Expected statement after for ");
    }
    else if (consume(BREAK))
    {
        if (!consume(SEMICOLON))
            tkerr(crtTk, "missing ; after break");
    }
    else if (consume(RETURN))
    {
        expr();
        if (!consume(SEMICOLON))
            tkerr(crtTk, "missing ; after return");
    }
    else if (expr())
    {
        if (!consume(SEMICOLON))
            tkerr(crtTk, "missing ; after expression in statement");
    }
    else if (consume(SEMICOLON))
    {
    }
    else
        return 0;
    return 1;
}

// stmCompound: LACC ( declVar | stm )* RACC
int stmCompound()
{
    if (!consume(LACC))
        return 0;
    while (1)
    {
        if (declVar())
        {
        }
        else if (stm())
        {
        }
        else
            break;
    }
    if (!consume(RACC))
        tkerr(crtTk, "Expected } in compound statement");
    return 1;
}

// expr: exprAssign
int expr()
{
    if (!exprAssign())
        return 0;
    return 1;
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
int exprAssign()
{
    Token *startTk = crtTk;
    if (exprUnary())
    {
        if (consume(ASSIGN))
        {
            if (!exprAssign())
                tkerr(crtTk, "Expected assign in expression");
            return 1;
        }
        crtTk = startTk;
    }
    if (exprOr())
    {
    }
    else
        return 0;
    return 1;
}

// exprOr: exprOr OR exprAnd | exprAnd
// Remove left recursion:
//     exprOr: exprAnd exprOr1
//     exprOr1: OR exprAnd exprOr1
int exprOr()
{
    if (!exprAnd())
        return 0;
    exprOr1();
    return 1;
}

void exprOr1()
{
    if (consume(OR))
    {
        if (!exprAnd())
            tkerr(crtTk, "missing expression after OR");
        exprOr1();
    }
}

// exprAnd: exprAnd AND exprEq | exprEq
// Remove left recursion:
//     exprAnd: exprEq exprAnd1
//     exprAnd1: AND exprEq exprAnd1
int exprAnd()
{
    if (!exprEq())
        return 0;
    exprAnd1();
    return 1;
}

void exprAnd1()
{
    if (consume(AND))
    {
        if (!exprEq())
            tkerr(crtTk, "missing expression after AND");
        exprAnd1();
    }
}

// exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel
// Remove left recursion:
//     exprEq: exprRel exprEq1
//     exprEq1: ( EQUAL | NOTEQ ) exprRel exprEq1
int exprEq()
{
    if (!exprRel())
        return 0;
    exprEq1();
    return 1;
}

void exprEq1()
{
    if (consume(EQUAL))
    {
    }
    else if (consume(NOTEQ))
    {
    }
    else
        return;
    if (!exprRel())
        tkerr(crtTk, "missing expressiong after =");
    exprEq1();
}

// exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd
// Remove left recursion:
//     exprRel: exprAdd exprRel1
//     exprRel1: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRel1
int exprRel()
{
    if (!exprAdd())
        return 0;
    exprRel1();
    return 1;
}

void exprRel1()
{
    if (consume(LESS))
    {
    }
    else if (consume(LESSEQ))
    {
    }
    else if (consume(GREATER))
    {
    }
    else if (consume(GREATEREQ))
    {
    }
    else
        return;
    if (!exprAdd())
        tkerr(crtTk, "missing expression after relationship");
    exprRel1();
}

// exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul
// Remove left recursion:
//     exprAdd: exprMul exprAdd1
//     exprAdd1: ( ADD | SUB ) exprMul exprAdd1
int exprAdd()
{
    if (!exprMul())
        return 0;
    exprAdd1();
    return 1;
}

void exprAdd1()
{
    if (consume(ADD))
    {
    }
    else if (consume(SUB))
    {
    }
    else
        return;
    if (!exprMul())
        tkerr(crtTk, "missing expressiong after + or -");
    exprAdd1();
}

// exprMul: exprMul ( MUL | DIV ) exprCast | exprCast
// Remove left recursion:
//     exprMul: exprCast exprMul1
//     exprMul1: ( MUL | DIV ) exprCast exprMul1
int exprMul()
{
    if (!exprCast())
        return 0;
    exprMul1();
    return 1;
}

void exprMul1()
{
    if (consume(MUL))
    {
    }
    else if (consume(DIV))
    {
    }
    else
        return;
    if (!exprCast())
        tkerr(crtTk, "missing expressiong after * or /");
    exprMul1();
}

// exprCast: LPAR typeName RPAR exprCast | exprUnary
int exprCast()
{
    Token *startTk = crtTk;
    if (consume(LPAR))
    {
        if (typeName())
        {
            if (consume(RPAR))
            {
                if (exprCast())
                {
                    return 1;
                }
            }
        }
        crtTk = startTk;
    }
    if (exprUnary())
    {
    }
    else
        return 0;
    return 1;
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
int exprUnary()
{
    if (consume(SUB))
    {
        if (!exprUnary())
            tkerr(crtTk, "missing unary expression after -");
    }
    else if (consume(NOT))
    {
        if (!exprUnary())
            tkerr(crtTk, "missing unary expression after !");
    }
    else if (exprPostfix())
    {
    }
    else
        return 0;
    return 1;
}

// exprPostfix: exprPostfix LBRACKET expr RBRACKET
//            | exprPostfix DOT ID
//            | exprPrimary
// Remove left recursion:
//     exprPostfix: exprPrimary exprPostfix1
//     exprPostfix1: ( LBRACKET expr RBRACKET | DOT ID ) exprPostfix1
int exprPostfix()
{
    if (!exprPrimary())
        return 0;
    exprPostfix1();
    return 1;
}

void exprPostfix1()
{
    if (consume(LBRACKET))
    {
        if (!expr())
            tkerr(crtTk, "missing expression after (");
        if (!consume(RBRACKET))
            tkerr(crtTk, "missing ) after expression");
    }
    else if (consume(DOT))
    {
        if (!consume(ID))
            tkerr(crtTk, "error consuming");
    }
    else
        return;
    exprPostfix1();
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
//            | CT_INT
//            | CT_REAL
//            | CT_CHAR
//            | CT_STRING
//            | LPAR expr RPAR
int exprPrimary()
{
    Token *startTk = crtTk;
    if (consume(ID))
    {
        if (consume(LPAR))
        {
            if (expr())
            {
                while (1)
                {
                    if (!consume(COMMA))
                        break;
                    if (!expr())
                        tkerr(crtTk, "missing expression after , in primary expression");
                }
            }
            if (!consume(RPAR))
                tkerr(crtTk, "missing )");
        }
    }
    else if (consume(CT_INT))
    {
    }
    else if (consume(CT_REAL))
    {
    }
    else if (consume(CT_CHAR))
    {
    }
    else if (consume(CT_STRING))
    {
    }
    else if (consume(LPAR))
    {
        if (!expr())
        {
            crtTk = startTk;
            return 0;
        }
        if (!consume(RPAR))
            tkerr(crtTk, "missing ) after expression");
    }
    else
        return 0;
    return 1;
}

typedef struct _Symbol Symbol;

typedef struct{
	Symbol **begin; 	// the beginning of the symbols, or NULL
	Symbol **end;		// the position after the last symbol
	Symbol **after;		// the position after the allocated space
} Symbols;

Symbols symbols;

enum { TB_INT, TB_DOUBLE, TB_CHAR, TB_STRUCT, TB_VOID };

typedef struct {
    int typeBase;   // TB_*
    Symbol *s;      // struct definition for TB_STRUCT
    int nElements;  // >0 array of given size, 0=array without size, <0 non array
} Type;

enum { CLS_VAR, CLS_FUNC, CLS_EXTFUNC, CLS_STRUCT };
enum { MEM_GLOBAL, MEM_ARG, MEM_LOCAL };

typedef struct _Symbol {
    const char *name;  // a reference to the name stored in a token
    int cls;           // CLS_*
    int mem;           // MEM_*
    Type type;
    int depth;         // 0-global, 1-in function, 2... - nested blocks in function
    union {
        Symbols args;    // used only for functions
        Symbols members; // used only for structs
    };
} Symbol;

struct _Symbol;
typedef struct _Symbol Symbol;

void initSymbols(Symbols *symbols) {
    symbols->begin = NULL;
    symbols->end = NULL;
    symbols->after = NULL;
}
Symbol *addSymbol(Symbols *symbols,const char *name,int cls) {
	Symbol *s;
	if(symbols->end==symbols->after){ // create more room
		int count = symbols->after-symbols->begin;
		int n = count*2; // double the room
		if(n==0)
			n=1; // needed for the initial case
		symbols->begin=(Symbol*)realloc(symbols->begin, n*sizeof(Symbol));
		if(symbols->begin==NULL)
			err("not enough memory");
		symbols->end = symbols->begin+count;
		symbols->after = symbols->begin+n;
	}
	SAFEALLOC(s,Symbol);
	*symbols->end++ = s;
	s->name = name;
	s->cls = cls;
	s->depth = crtDepth;

	return s;
}

int main(int argc, char **argv) {
    struct stat st;
    int size;
    int fd;

    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    fd = open_file(argv[1]);
    if (fd == -1) {
        printf("Unable to open file\n");
        return -1;
    }
    

    if (stat(argv[1], &st) == 0)
        size = st.st_size;
    else {
        close(fd);
        return -1;
    }

    char *myString = (char *)malloc(size + 1);
    if (myString == NULL) {
        printf("Memory allocation error\n");
        close(fd);
        return -1;
    }

    ssize_t last = read(fd, myString, size);
    if (last <= 0) {
        printf("Error reading file\n");
        close(fd);
        free(myString);
        return -1;
    }

    myString[last] = '\0';

    puts(myString);
    getNextToken(myString);
    Token *aux = tokens;
    while (aux != NULL) {
        // printf("Code %d ", aux->code);
        if ((aux->code == ID))
            printf("%d Identifier %s \n",aux->line, aux->text);
        else if (aux->code == CT_CHAR)
            printf("%d character %s\n", aux->line ,aux->text);
        else if (aux->code == CT_STRING)
            printf("%d string %s\n", aux->line, aux->text);
        else if (aux->code == CT_INT)
            printf("%d integer value %ld \n", aux->line, aux->i);
        else if (aux->code == CT_REAL)
            printf("%d float value %f \n", aux->line, aux->r);
        aux = aux->next;
    }

    printf("Read %zd bytes from the file '%s'\n", last, argv[1]);

    if (unit()) {
        printf("The syntax is correct!\n");
    }

    // initSymbols(&symbols);

    close(fd);
    free(myString);

    return 0;
}