/***************************************************************
*      scanner routine for Mini C language                    *
*                                   2003. 3. 10               *
***************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "Scanner.h"

extern FILE *sourceFile;                       // miniC source program


int superLetter(char ch);
int superLetterOrDigit(char ch);
void getNumber(char firstCharacter, struct tokenType *token);
int hexValue(char ch);
void lexicalError(int n);
char getEscapeSequenceValue(char* ch);
double getRealNum(int num, char* ch);
char *tokenName[] = {
	"!",        "!=",      "%",       "%=",     "%ident",   "%intnumber",
	/* 0          1           2         3          4          5        */
	"&&",       "(",       ")",       "*",      "*=",       "+",
	/* 6          7           8         9         10         11        */
	"++",       "+=",      ",",       "-",      "--",	    "-=",
	/* 12         13         14        15         16         17        */
	"/",        "/=",      ";",       "<",      "<=",       "=",
	/* 18         19         20        21         22         23        */
	"==",       ">",       ">=",      "[",      "]",        "eof",
	/* 24         25         26        27         28         29        */
	//   ...........    word symbols ................................. //
	/* 30         31         32        33         34         35        */
	"const",    "else",     "if",      "int",     "return",  "void",
	/* 36         37         38        39                              */
	"while",    "{",        "||",       "}",
	//   ...........    �߰��� ��� ................................. //
	/* 40         41         42        43         44         45        */
	"for",    "switch",     "case",      "break",     "goto", "continue", 
	/* 46         47         48			49                             */
	"double",     ":",       "string",   "realnumber"
};

char *keyword[NO_KEYWORD] = {
	"const",  "else",    "if",    "int",    "return",  "void",    "while", "for", "switch", "case", "break", "goto", "double", "continue"
};

enum tsymbol tnum[NO_KEYWORD] = {
	tconst,    telse,     tif,     tint,     treturn,   tvoid,     twhile,	tfor,	tswitch,	tcase,	tbreak,	tgoto,	tdouble, tcontinue
};

struct tokenType scanner()
{
	struct tokenType token;
	int i, index;
	char ch, id[ID_LENGTH];
	
	token.number = tnull;

	do {
		while (isspace(ch = fgetc(sourceFile)));		// state 1: skip blanks
		if (superLetter(ch)) {							// identifier or keyword
			i = 0;
			do {
				if (i < ID_LENGTH) id[i++] = ch;
				ch = fgetc(sourceFile);
			} while (superLetterOrDigit(ch));
			if (i >= ID_LENGTH) 
				lexicalError(1);
			id[i] = '\0';
			ungetc(ch, sourceFile);  // retract
									 // find the identifier in the keyword table
			for (index = 0; index < NO_KEYWORD; index++)
				if (!strcmp(id, keyword[index])) 
					break;

			if (index < NO_KEYWORD)    // found, keyword exit
				token.number = tnum[index];
			else {                     // not found, identifier exit
				token.number = tident;
				strcpy_s(token.value.id, id);
			}
		}  // end of identifier or keyword
		else if (isdigit(ch)) {							// number			
			//getNumber�Լ����� ���� token�� ���� �Է��ϵ��� ó���Ͽ���.
			getNumber(ch, &token);					
		}
		else if (ch == '\"'){							// string constant
			i = 0;
			ch = fgetc(sourceFile);
			do {
				if (ch == '\\') {						//execute escape sequence
					ch = fgetc(sourceFile);
					ch = getEscapeSequenceValue(&ch);	//escape sequence�Լ��� ������� ��
				}
				if (i < ID_LENGTH)				
					id[i++] = ch;
				ch = fgetc(sourceFile);					
			} while (ch != '\"');
			if (i >= ID_LENGTH) 
				lexicalError(1);
			id[i] = '\0';
			token.number = tstring;
			strcpy_s(token.value.id, id);			

		}
		// special character
		else switch (ch) {  
		case '.':		// .�� ���� ��쿡�� ������ �ȵǵ��� �ϰ�, .���Ŀ� �Ǽ��� ������ ��쿡�� �Ǽ��� �ν��ϵ��� �Ѵ�.
			ch = fgetc(sourceFile);
			if (isdigit(ch)) {
				double fnum;
				ungetc(ch, sourceFile);
				fnum = getRealNum(0, &ch);			//getRealNum�Լ��� ���Ͽ� �Ǽ����� ��´�.
				token.number = trealnumber;
				token.value.fnum = fnum;				
			}
			else {
				fprintf(stderr, "ERR\n");			//�ƴ� ��쿡 ���� ���
			}
				ungetc(ch, sourceFile);
			break;
		case '/':
			ch = fgetc(sourceFile);
			if (ch == '*') {			
				ch = fgetc(sourceFile);
				if (ch == '*') {		// ����ȭ �ּ�
					//����ȭ �ּ��� /**�� ȭ�鿡 ����� ��, ���Ŀ� ������ ���뵵 ��� ȭ�鿡 ����ϰ� */�� �����Ѵ�.
					printf("/**");
					do {
						do {
							ch = fgetc(sourceFile);
							putc(ch, stdout);
						} while (ch != '*');
						ch = fgetc(sourceFile);
						putc(ch, stdout);
					} while (ch != '/');
					printf("\n");
				}	
				else                         // text comment
					do {
						while (ch != '*') ch = fgetc(sourceFile);
						ch = fgetc(sourceFile);
					} while (ch != '/');
			}
			else if (ch == '/')		// line comment
				while (fgetc(sourceFile) != '\n');
			else if (ch == '=')  token.number = tdivAssign;
			else {
				token.number = tdiv;
				ungetc(ch, sourceFile); // retract
			}
			break;
		case '!':
			ch = fgetc(sourceFile);
			if (ch == '=')  token.number = tnotequ;
			else {
				token.number = tnot;
				ungetc(ch, sourceFile); // retract
			}
			break;
		case '%':
			ch = fgetc(sourceFile);
			if (ch == '=') {
				token.number = tremAssign;
			}
			else {
				token.number = tremainder;
				ungetc(ch, sourceFile);
			}
			break;
		case '&':
			ch = fgetc(sourceFile);
			if (ch == '&')  token.number = tand;
			else {
				lexicalError(2);
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '*':
			ch = fgetc(sourceFile);
			if (ch == '=')  token.number = tmulAssign;
			else {
				token.number = tmul;
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '+':
			ch = fgetc(sourceFile);
			if (ch == '+')  token.number = tinc;
			else if (ch == '=') token.number = taddAssign;
			else {
				token.number = tplus;
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '-':
			ch = fgetc(sourceFile);
			if (ch == '-')  token.number = tdec;
			else if (ch == '=') token.number = tsubAssign;
			else {
				token.number = tminus;
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '<':
			ch = fgetc(sourceFile);
			if (ch == '=') token.number = tlesse;
			else {
				token.number = tless;
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '=':
			ch = fgetc(sourceFile);
			if (ch == '=')  token.number = tequal;
			else {
				token.number = tassign;
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '>':
			ch = fgetc(sourceFile);
			if (ch == '=') token.number = tgreate;
			else {
				token.number = tgreat;
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '|':
			ch = fgetc(sourceFile);
			if (ch == '|')  token.number = tor;
			else {
				lexicalError(3);
				ungetc(ch, sourceFile);  // retract
			}
			break;
		case '(': token.number = tlparen;         break;
		case ')': token.number = trparen;         break;
		case ',': token.number = tcomma;          break;
		case ';': token.number = tsemicolon;      break;
		case '[': token.number = tlbracket;       break;
		case ']': token.number = trbracket;       break;
		case '{': token.number = tlbrace;         break;
		case '}': token.number = trbrace;         break;
		case ':': token.number = tcolon;          break;		//case���� ���� �ݷ� �߰�
		case EOF: token.number = teof;            break;
		default: {
			printf("Current character : %c", ch);
			lexicalError(4);
			break;
		}

		} // switch end
	} while (token.number == tnull);
	return token;
} // end of scanner

void lexicalError(int n)
{
	printf(" *** Lexical Error : ");
	switch (n) {
	case 1: printf("an identifier length must be less than 12.\n");
		break;
	case 2: printf("next character must be &\n");
		break;
	case 3: printf("next character must be |\n");
		break;
	case 4: printf("invalid character\n");
		break;
	}
}

int superLetter(char ch)
{
	if (isalpha(ch) || ch == '_') 
		return 1;
	else 
		return 0;
}

int superLetterOrDigit(char ch)
{
	if (isalnum(ch) || ch == '_') 
		return 1;
	else 
		return 0;
}

void getNumber(char firstCharacter, struct tokenType *token)
{
	int num = 0;
	double fnum = -1.0;
	int value;
	char ch;

	if (firstCharacter == '0') {
		ch = fgetc(sourceFile);
		if ((ch == 'X') || (ch == 'x')) {		// hexa decimal
			while ((value = hexValue(ch = fgetc(sourceFile))) != -1)
				num = 16 * num + value;
		}
		else if ((ch >= '0') && (ch <= '7'))	// octal
			do {
				num = 8 * num + (int)(ch - '0');
				ch = fgetc(sourceFile);
			} while ((ch >= '0') && (ch <= '7'));
		else if (ch == '.')		//�Ǽ� ó��
			fnum = getRealNum(0, &ch);
		else 
			num = 0;						// zero
	}
	else {									// decimal
		ch = firstCharacter;
		do {
			num = 10 * num + (int)(ch - '0');
			ch = fgetc(sourceFile);
		} while (isdigit(ch));
		if (ch == '.') {		//�Ҽ� ����
			fnum = getRealNum(num, &ch);
		}
	}
	ungetc(ch, sourceFile);  /*  retract  */
	if (fnum == -1.0) {		//fnum�� -1.0���� �ʱ�ȭ �Ǿ� ������, �Ǽ������� �� ��쿡�� ���� ���Ѵ�.
		token->number = tintnumber;
		token->value.num = num;
	}
	else {					//�Ǽ�
		token->number = trealnumber;
		token->value.fnum = fnum;
	}
}
double getRealNum(int num, char* ch) {
	/*
	num : . ������ ����
	ch : . ������ ����
	�Ǽ��κ��� ó���ϱ����� �Լ��� . ������ ���ڿ� ������ ���ڸ� �Է¹޴´�.
	1. ch�� ���ڰ� �ƴ� ��쿡�� ����������� ����Ѵ�.
	2. ch�� ������ ��쿡�� ������ ���� ����Ͽ� num�� �߰��ϸ� e�� ���� �����ش�.
	2.1 ���Ŀ� e �Ǵ� E�� �Էµ� ��쿡 �ε��Ҽ��� ������ ���ش�.

	������� 3.14e5�� �Է����Ͽ� �ִٸ�, getRealNum�Լ��� ȣ���Ҷ� num���� 3�� ����� ���� ���̸�, ch�� 1�� ����Ű�� ���� ���̤���.
	�̶� ���ڰ� ������ ���������� ch�� ��ĭ�� ���������� e�� ���ҽ�Ű��, num�� ����Ͽ� ���ڸ� ����ִ´�.
	�� e�� ���ö�, num���� 314�� ����� ���̸�, e�� -2�� ����� ���̴�.
	e�� ���� ���Ŀ� �ٽ� ���ڸ� �о�� fnum�̶�� ������ ���� �����ϸ�, fnum�� ���� e�� �����ش�.
	���� ���� ��Ʈ���� �� �о��� ��쿡, num���� 314��, e���� 3�� ����� ���̴�.
	���Ŀ� ��������� 314 * 10^3�� ��ȯ�Ѵ�.
	*/
	int fnum = 0;
	int e = 0;
	int sign = 1;
	*ch = fgetc(sourceFile);
	if (isdigit(*ch)) {	
		do {
			e--;									//e�� ���δ�.
			num = 10 * num + (int)(*ch - '0');		//num�� ����Ͽ� ���ڸ� �־��ش�.
			*ch = fgetc(sourceFile);
		} while (isdigit(*ch));
	}
	if (*ch == 'e' || *ch == 'E') {
		*ch = fgetc(sourceFile);
		if (*ch == '-') {
			sign = -1;
			*ch = fgetc(sourceFile);
		}
		else if (*ch == '+') {
			*ch = fgetc(sourceFile);
		}
		do {
			fnum = 10 * fnum + (int)(*ch - '0');
			*ch = fgetc(sourceFile);
		} while (isdigit(*ch));
		fnum *= sign;

	}
	fnum += e;
	return (double)num * pow(10.0, fnum);
}
int hexValue(char ch)
{
	switch (ch) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return (ch - '0');
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		return (ch - 'A' + 10);
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		return (ch - 'a' + 10);
	default: return -1;
	}
}

char getEscapeSequenceValue(char* ch) {
	//Escape Sequence�� ó���ϴ� �Լ�
	// 1. \���� 0~7������ ���ڰ� ������ ��쿡 ���� ���ڸ� 8������ ó���Ͽ� ��ȯ�Ѵ�.
	// 2. x �Ǵ� X�� ������ ��쿡�� ���� ���ڸ� 16������ ó���Ͽ� ��ȯ�Ѵ�.
	// 3. �� �ܿ� �ƽ�Ű escape sequence�� �ش��ϴ� ���ڰ� ���´ٸ� escape sequence�� ���� ��ȯ�Ѵ�.
	// 4. 1~3������ ������ ���� ��쿡�� \�ڿ� ������ ���ڸ� ��ȯ�Ѵ�.
	int count = 0;
	char num = 0;
	int value;
	if (*ch >= '0' && *ch <= '7') {			//8���� ó��
		do {
			num = 8 * num + (int)(*ch - '0');
			*ch = fgetc(sourceFile);
			count++;						//\�ڿ� ���� �������� 3�ڸ��� ���� �� �����Ƿ� count�� �̿��Ͽ� 3�ڸ��� ������� �ݺ����� �����Ѵ�.
		} while ((*ch >= '0') && (*ch <= '7') && count < 3);
		ungetc(*ch, sourceFile);		//retract
		return num;
	}
	else if ((*ch == 'X') || (*ch == 'x')) {		// hexa decimal
		while ((value = hexValue(*ch = fgetc(sourceFile))) == 0);		//haxValue�Լ��� �̿��Ͽ� 16���� ó��
		while(value != -1){			
			count++;
			num = 16 * num + value;
			value = hexValue(*ch = fgetc(sourceFile));
		}
		if (count > 2)			//16�������� 3�� �̻� �� ��� 1����Ʈ�� char�� ������� �� �� ����.
			fprintf(stderr, "���ڿ� ���� �ʹ� Ů�ϴ�\n");
			
		
		ungetc(*ch, sourceFile);
		return num;
	}
	else switch (*ch) {
	case 'a':	return 0x07;
	case 'b':	return 0x08;
	case 'f':	return 0x0c;
	case 'n':	return 0x0a;
	case 'r':	return 0x0d;
	case 't':	return 0x09;
	case 'v':	return 0x0b;
	case '\\':  return 0x5c;
	case '\'':	return 0x27;
	case '\"':	return 0x22;
	case '?':	return 0x3f;
	case 'e':	return 0x1b;
	default :	return *ch;
	}
}

void printToken(struct tokenType token)
{
	if (token.number == tident || token.number == tstring)		//tstring�� ��쿡�� ���� ������ ����Ѵ�.
		printf("number: %d, value: %s\n", token.number, token.value.id);
	else if (token.number == tintnumber)		//����
		printf("number: %d, value: %d\n", token.number, token.value.num);
	else if (token.number == trealnumber)		//�Ǽ�
		printf("number: %d, value: %lf\n", token.number, token.value.fnum);
	else
		printf("number: %d(%s)\n", token.number, tokenName[token.number]);

}