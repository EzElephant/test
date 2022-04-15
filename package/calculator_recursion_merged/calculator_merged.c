#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// for lex
#define MAXLEN 256

// Token types
typedef enum
{
    UNKNOWN,
    END,
    ENDFILE,
    INT,
    ID,
    ADDSUB,
    MULDIV,
    ASSIGN,
    LPAREN,
    RPAREN,
    INCDEC,
    OR,
    XOR,
    AND
} TokenSet;

TokenSet getToken(void);
TokenSet curToken = UNKNOWN;
char lexeme[MAXLEN];

// Test if a token matches the current token
int match(TokenSet token);
// Get the next token
void advance(void);
// Get the lexeme of the current token
char *getLexeme(void);

// for parser
#define TBLSIZE 64
// Set PRINTERR to 1 to print error message while calling error()
// Make sure you set PRINTERR to 0 before you submit your code
#define PRINTERR 1

// Call this macro to print error message and exit the program
// This will also print where you called it in your program
#define error(errorNum)                                                       \
    {                                                                         \
        if (PRINTERR)                                                         \
            fprintf(stderr, "error() called at %s:%d: ", __FILE__, __LINE__); \
        err(errorNum);                                                        \
    }

// Error types
typedef enum
{
    UNDEFINED,
    MISPAREN,
    NOTNUMID,
    NOTFOUND,
    RUNOUT,
    NOTLVAL,
    DIVZERO,
    SYNTAXERR
} ErrorType;

// Structure of the symbol table
typedef struct
{
    int val;
    char name[MAXLEN];
} Symbol;

// Structure of a tree node
typedef struct _Node
{
    TokenSet data;
    int val, flag, reg;
    char lexeme[MAXLEN];
    struct _Node *left;
    struct _Node *right;
} BTNode;

int sbcount = 0, cnt;
Symbol table[TBLSIZE];

// Initialize the symbol table with builtin variables
void initTable(void);
// Get the value of a variable
int getval(char *str);
// Set the value of a variable
int setval(char *str, int val);
// Get the memory of a variable
int getmem(char *str);
// Make a new node according to token type and lexeme
BTNode *makeNode(TokenSet tok, const char *lexe);
// Free the syntax tree
void freeTree(BTNode *root);
BTNode *assign_expr(void);
BTNode *or_expr(void);
BTNode *or_expr_tail(BTNode *left);
BTNode *xor_expr(void);
BTNode *xor_expr_tail(BTNode *left);
BTNode *and_expr(void);
BTNode *and_expr_tail(BTNode *left);
BTNode *addsub_expr(void);
BTNode *addsub_expr_tail(BTNode *left);
BTNode *muldiv_expr(void);
BTNode *muldiv_expr_tail(BTNode *left);
BTNode *unary_expr(void);
BTNode *factor(void);

void statement(void);
// Print error message and exit the program
void err(ErrorType errorNum);

// for codeGen
// Print the syntax tree in prefix
void printPrefix(BTNode *root);
// dfs variables
int dfs(BTNode *root);
// code gendration
int codegen(BTNode *root);

/*============================================================================================
lex implementation
============================================================================================*/

TokenSet getToken(void)
{
    int i = 0;
    char c = '\0';

    while ((c = fgetc(stdin)) == ' ' || c == '\t')
        ;

    if (isdigit(c))
    {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN)
        {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    }
    else if (c == '+' || c == '-')
    {
        lexeme[0] = c;
        c = fgetc(stdin);
        if (c == lexeme[0])
        {
            lexeme[1] = lexeme[0];
            lexeme[2] = '\0';
            return INCDEC;
        }
        else
        {
            ungetc(c, stdin);
            lexeme[1] = '\0';
            return ADDSUB;
        }
    }
    else if (c == '*' || c == '/')
    {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    }
    else if (c == '\n')
    {
        lexeme[0] = '\0';
        return END;
    }
    else if (c == '=')
    {
        strcpy(lexeme, "=");
        return ASSIGN;
    }
    else if (c == '(')
    {
        strcpy(lexeme, "(");
        return LPAREN;
    }
    else if (c == ')')
    {
        strcpy(lexeme, ")");
        return RPAREN;
    }
    else if (isalpha(c) || c == '_')
    {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while ((isalpha(c) || isdigit(c) || c == '_') && i < MAXLEN)
        {
            lexeme[i] = c;
            i++;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return ID;
    }
    else if (c == EOF)
    {
        return ENDFILE;
    }
    else if (c == '|')
    {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return OR;
    }
    else if (c == '^')
    {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return XOR;
    }
    else if (c == '&')
    {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return AND;
    }
    else
    {
        return UNKNOWN;
    }
}

void advance(void)
{
    curToken = getToken();
}

int match(TokenSet token)
{
    if (curToken == UNKNOWN)
        advance();
    return token == curToken;
}

char *getLexeme(void)
{
    return lexeme;
}

/*============================================================================================
parser implementation
============================================================================================*/

void initTable(void)
{
    strcpy(table[0].name, "x");
    table[0].val = 0;
    strcpy(table[1].name, "y");
    table[1].val = 0;
    strcpy(table[2].name, "z");
    table[2].val = 0;
    sbcount = 3;
}

int getval(char *str)
{
    int i = 0;

    for (i = 0; i < sbcount; i++)
        if (strcmp(str, table[i].name) == 0)
            return table[i].val;

    if (sbcount >= TBLSIZE)
    {
        printf("EXIT 1\n");
        error(RUNOUT);
    }

    strcpy(table[sbcount].name, str);
    table[sbcount].val = 0;
    sbcount++;
    return (-1e9 + 87);
}

int setval(char *str, int val)
{
    int i = 0;

    for (i = 0; i < sbcount; i++)
    {
        if (strcmp(str, table[i].name) == 0)
        {
            table[i].val = val;
            return val;
        }
    }

    if (sbcount >= TBLSIZE)
    {
        printf("EXIT 1\n");
        error(RUNOUT);
    }

    strcpy(table[sbcount].name, str);
    table[sbcount].val = val;
    sbcount++;
    return val;
}

int getmem(char *str)
{
    for (int i = 0; i < sbcount; i++)
    {
        if (strcmp(str, table[i].name) == 0)
            return 4 * i;
    }
}

BTNode *makeNode(TokenSet tok, const char *lexe)
{
    BTNode *node = (BTNode *)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->val = 0;
    node->flag = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void freeTree(BTNode *root)
{
    if (root != NULL)
    {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

// statement := ENDFILE | END | expr END
void statement(void)
{
    BTNode *retp = NULL;

    if (match(ENDFILE))
    {
        printf("MOV r0 [0]\nMOV r1 [4]\nMOV r2 [8]\nEXIT 0\n");
        exit(0);
    }
    else if (match(END))
    {
        fprintf(stderr, ">> ");
        advance();
    }
    else
    {
        retp = assign_expr();
        if (match(END))
        {
            fprintf(stderr, "Prefix traversal: ");
            printPrefix(retp);
            fprintf(stderr, "\n");
            dfs(retp);
            cnt = 0;
            codegen(retp);
            freeTree(retp);
            fprintf(stderr, ">> ");
            advance();
        }
        else
        {
            printf("EXIT 1\n");
            error(SYNTAXERR);
        }
    }
}

// assign_expr := ID ASSIGN assign_expr | or_expr
BTNode *assign_expr(void)
{
    BTNode *retp = or_expr(), *left = NULL, *node;
    if (retp->data == ID && match(ASSIGN))
    {
        node = makeNode(ASSIGN, getLexeme());
        advance();
        node->left = retp;
        node->right = assign_expr();
        return node;
    }
    return retp;
}

// or_expr := xor_expr or_expr_tail
BTNode *or_expr(void)
{
    BTNode *node = xor_expr();
    return or_expr_tail(node);
}

// or_expr_tail := OR xor_expr or_expr_tail | NiL
BTNode *or_expr_tail(BTNode *left)
{
    BTNode *node = NULL;
    if (match(OR))
    {
        node = makeNode(OR, getLexeme());
        advance();
        node->left = left;
        node->right = xor_expr();
        return or_expr_tail(node);
    }
    else
        return left;
}

// xor_expr := and_expr xor_expr_tail
BTNode *xor_expr(void)
{
    BTNode *node = and_expr();
    return xor_expr_tail(node);
}

// xor_expr_tail := XOR and_expr xor_expr_tail | NiL
BTNode *xor_expr_tail(BTNode *left)
{
    BTNode *node = NULL;
    if (match(XOR))
    {
        node = makeNode(XOR, getLexeme());
        advance();
        node->left = left;
        node->right = and_expr();
        return xor_expr_tail(node);
    }
    else
        return left;
}

// and_expr := addsub_expr and_expr_tail
BTNode *and_expr(void)
{
    BTNode *node = addsub_expr();
    return and_expr_tail(node);
}

// and_expr_tail := AND addsub_expr and_expr_tail | NiL
BTNode *and_expr_tail(BTNode *left)
{
    BTNode *node = NULL;
    if (match(AND))
    {
        node = makeNode(AND, getLexeme());
        advance();
        node->left = left;
        node->right = addsub_expr();
        return and_expr_tail(node);
    }
    else
        return left;
}

// addsub_expr := muldiv_expr addsub_expr_tail
BTNode *addsub_expr(void)
{
    BTNode *node = muldiv_expr();
    return addsub_expr_tail(node);
}

// addsub_expr_tail := ADDSUB muldiv_expr addsub_expr_tail | NiL
BTNode *addsub_expr_tail(BTNode *left)
{
    BTNode *node = NULL;
    if (match(ADDSUB))
    {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        node->right = muldiv_expr();
        return addsub_expr_tail(node);
    }
    else
        return left;
}

// muldiv_expr := unary_expr muldiv_expr_tail
BTNode *muldiv_expr(void)
{
    BTNode *node = unary_expr();
    return muldiv_expr_tail(node);
}

// muldiv_expr_tail := MULDIV unary_expr muldiv_expr_tail | NiL
BTNode *muldiv_expr_tail(BTNode *left)
{
    BTNode *node = NULL;
    if (match(MULDIV))
    {
        node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        node->right = unary_expr();
        return muldiv_expr_tail(node);
    }
    else
        return left;
}

// unary_expr := ADDSUB unary_expr | factor
BTNode *unary_expr(void)
{
    BTNode *retp = NULL, *right = NULL;
    if (match(ADDSUB))
    {
        retp = makeNode(ADDSUB, getLexeme());
        advance();
        right = unary_expr();
        retp->right = right;
    }
    else
        retp = factor();
    return retp;
}

// factor := INT | ID | INCDEC ID | LPAREN assign_expr RPAREN
BTNode *factor(void)
{
    BTNode *retp = NULL, *left = NULL;

    if (match(INT))
    {
        retp = makeNode(INT, getLexeme());
        advance();
    }
    else if (match(ID))
    {
        retp = makeNode(ID, getLexeme());
        advance();
    }
    else if (match(INCDEC))
    {
        left = makeNode(INCDEC, getLexeme());
        advance();
        if (!match(ID))
        {
            printf("EXIT 1\n");
            error(SYNTAXERR);
        }
        else
        {
            retp = makeNode(ID, getLexeme());
            advance();
            retp->left = left;
            if (match(INCDEC))
            {
                printf("EXIT 1\n");
                error(SYNTAXERR);
            }
        }
    }
    else if (match(LPAREN))
    {
        advance();
        retp = assign_expr();
        if (match(RPAREN))
            advance();
        else
        {
            printf("EXIT 1\n");
            error(MISPAREN);
        }
    }
    else
    {
        printf("EXIT 1\n");
        error(NOTNUMID);
    }
    return retp;
}

void err(ErrorType errorNum)
{
    if (PRINTERR)
    {
        fprintf(stderr, "error: ");
        switch (errorNum)
        {
        case MISPAREN:
            fprintf(stderr, "mismatched parenthesis\n");
            break;
        case NOTNUMID:
            fprintf(stderr, "number or identifier expected\n");
            break;
        case NOTFOUND:
            fprintf(stderr, "variable not defined\n");
            break;
        case RUNOUT:
            fprintf(stderr, "out of memory\n");
            break;
        case NOTLVAL:
            fprintf(stderr, "lvalue required as an operand\n");
            break;
        case DIVZERO:
            fprintf(stderr, "divide by constant zero\n");
            break;
        case SYNTAXERR:
            fprintf(stderr, "syntax error\n");
            break;
        default:
            fprintf(stderr, "undefined error\n");
            break;
        }
    }
    exit(0);
}

/*============================================================================================
codeGen implementation
============================================================================================*/

void printPrefix(BTNode *root)
{
    if (root != NULL)
    {
        fprintf(stderr, "%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}

int dfs(BTNode *root)
{
    if (root != NULL)
        return root->flag = dfs(root->left) | dfs(root->right) | (root->data == ID);
    return 0;
}

int codegen(BTNode *root)
{
    int retval = 0, lv = 0, rv = 0;
    if (root != NULL)
    {
        switch (root->data)
        {
        case INCDEC:
            root->reg = cnt++;
            if (strcmp(root->lexeme, "++") == 0)
            {
                retval = 1;
            }
            else if (strcmp(root->lexeme, "--") == 0)
            {
                retval = -1;
            }
            printf("MOV r%d 1\n", root->reg);
            break;
        case ID:
            root->reg = cnt++;
            printf("MOV r%d [%d]\n", root->reg, getmem(root->lexeme));
            lv = codegen(root->left);
            retval = getval(root->lexeme);
            if (retval == (-1e9 + 87))
            {
                printf("EXIT 1\n");
                error(NOTFOUND);
            }
            if (lv != 0)
            {
                retval = setval(root->lexeme, retval + lv);
                if (lv == 1)
                    printf("ADD r%d r%d\n", root->reg, root->left->reg);
                else
                    printf("SUB r%d r%d\n", root->reg, root->left->reg);
                printf("MOV [%d] r%d\n", getmem(root->lexeme), root->reg);
                cnt--;
            }
            break;
        case INT:
            retval = atoi(root->lexeme);
            root->reg = cnt++;
            printf("MOV r%d %d\n", root->reg, retval);
            break;
        case ASSIGN:
            rv = codegen(root->right);
            root->reg = cnt++;
            retval = setval(root->left->lexeme, rv);
            printf("MOV r%d r%d\n", root->reg, root->right->reg);
            printf("MOV [%d] r%d\n", getmem(root->left->lexeme), root->reg);
            cnt--;
            root->reg = root->right->reg;
            break;
        case OR:
        case XOR:
        case AND:
        case ADDSUB:
        case MULDIV:
            lv = codegen(root->left);
            if (root->left == NULL)
            {
                root->left = makeNode(INT, "0");
                root->left->reg = cnt++;
                printf("MOV r%d 0\n", root->left->reg);
            }
            rv = codegen(root->right);
            if (strcmp(root->lexeme, "+") == 0)
            {
                retval = lv + rv;
                printf("ADD r%d r%d\n", root->left->reg, root->right->reg);
            }
            else if (strcmp(root->lexeme, "-") == 0)
            {
                retval = lv - rv;
                printf("SUB r%d r%d\n", root->left->reg, root->right->reg);
            }
            else if (strcmp(root->lexeme, "*") == 0)
            {
                retval = lv * rv;
                printf("MUL r%d r%d\n", root->left->reg, root->right->reg);
            }
            else if (strcmp(root->lexeme, "/") == 0)
            {
                if (rv == 0 && root->flag == 0)
                {
                    printf("EXIT 1\n");
                    error(DIVZERO);
                }
                if (rv != 0)
                    retval = lv / rv;
                printf("DIV r%d r%d\n", root->left->reg, root->right->reg);
            }
            else if (strcmp(root->lexeme, "|") == 0)
            {
                retval = lv | rv;
                printf("OR r%d r%d\n", root->left->reg, root->right->reg);
            }
            else if (strcmp(root->lexeme, "^") == 0)
            {
                retval = lv ^ rv;
                printf("XOR r%d r%d\n", root->left->reg, root->right->reg);
            }
            else if (strcmp(root->lexeme, "&") == 0)
            {
                retval = lv & rv;
                printf("AND r%d r%d\n", root->left->reg, root->right->reg);
            }
            root->reg = root->left->reg;
            cnt--;
            break;
        default:
            retval = 0;
        }
    }
    return retval;
}

/*============================================================================================
main
============================================================================================*/

int main()
{
    initTable();
    fprintf(stderr, ">> ");
    while (1)
    {
        statement();
    }
    return 0;
}
