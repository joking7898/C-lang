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
	//   ...........    추가된 기능 ................................. //
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
			//getNumber함수에서 직접 token에 값을 입력하도록 처리하였다.
			getNumber(ch, &token);					
		}
		else if (ch == '\"'){							// string constant
			i = 0;
			ch = fgetc(sourceFile);
			do {
				if (ch == '\\') {						//execute escape sequence
					ch = fgetc(sourceFile);
					ch = getEscapeSequenceValue(&ch);	//escape sequence함수의 결과값이 들어감
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
		case '.':		// .만 나온 경우에는 실행이 안되도록 하고, .이후에 실수가 나오는 경우에는 실수를 인식하도록 한다.
			ch = fgetc(sourceFile);
			if (isdigit(ch)) {
				double fnum;
				ungetc(ch, sourceFile);
				fnum = getRealNum(0, &ch);			//getRealNum함수를 통하여 실수값을 얻는다.
				token.number = trealnumber;
				token.value.fnum = fnum;				
			}
			else {
				fprintf(stderr, "ERR\n");			//아닌 경우에 에러 출력
			}
				ungetc(ch, sourceFile);
			break;
		case '/':
			ch = fgetc(sourceFile);
			if (ch == '*') {			
				ch = fgetc(sourceFile);
				if (ch == '*') {		// 문서화 주석
					//문서화 주석은 /**를 화면에 출력한 후, 이후에 나오는 내용도 모두 화면에 출력하고 */로 종료한다.
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
		case ':': token.number = tcolon;          break;		//case문을 위한 콜론 추가
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
		else if (ch == '.')		//실수 처리
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
		if (ch == '.') {		//소수 연산
			fnum = getRealNum(num, &ch);
		}
	}
	ungetc(ch, sourceFile);  /*  retract  */
	if (fnum == -1.0) {		//fnum은 -1.0으로 초기화 되어 있으며, 실수연산을 한 경우에만 값이 변한다.
		token->number = tintnumber;
		token->value.num = num;
	}
	else {					//실수
		token->number = trealnumber;
		token->value.fnum = fnum;
	}
}
double getRealNum(int num, char* ch) {
	/*
	num : . 이전의 숫자
	ch : . 이후의 문자
	실수부분을 처리하기위한 함수로 . 이전의 숫자와 다음의 문자를 입력받는다.
	1. ch가 숫자가 아닌 경우에는 요약형식으로 취급한다.
	2. ch가 숫자인 경우에는 이후의 값을 계속하여 num에 추가하며 e의 값을 내려준다.
	2.1 이후에 e 또는 E가 입력된 경우에 부동소수점 연산을 해준다.

	예를들어 3.14e5가 입력파일에 있다면, getRealNum함수를 호출할때 num에는 3이 저장되 있을 것이며, ch는 1을 가르키고 있을 것이ㅏㄷ.
	이때 숫자가 나오지 않을때까지 ch를 한칸씩 움직여가며 e를 감소시키고, num에 계속하여 숫자를 집어넣는다.
	즉 e가 나올때, num에는 314가 저장될 것이며, e는 -2가 저장될 것이다.
	e가 나온 이후에 다시 숫자를 읽어가며 fnum이라는 변수에 값을 저장하며, fnum의 값을 e와 더해준다.
	따라서 위의 스트링을 다 읽었을 경우에, num에는 314가, e에는 3이 저장될 것이다.
	이후에 결과값으로 314 * 10^3을 반환한다.
	*/
	int fnum = 0;
	int e = 0;
	int sign = 1;
	*ch = fgetc(sourceFile);
	if (isdigit(*ch)) {	
		do {
			e--;									//e를 줄인다.
			num = 10 * num + (int)(*ch - '0');		//num에 계속하여 숫자를 넣어준다.
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
	//Escape Sequence를 처리하는 함수
	// 1. \이후 0~7까지의 숫자가 나오는 경우에 뒤의 문자를 8전수로 처리하여 반환한다.
	// 2. x 또는 X가 나오는 경우에는 뒤의 문자를 16전수로 처리하여 반환한다.
	// 3. 그 외에 아스키 escape sequence에 해당하는 문자가 나온다면 escape sequence의 값을 반환한다.
	// 4. 1~3과정에 속하지 않은 경우에는 \뒤에 나오는 문자를 반환한다.
	int count = 0;
	char num = 0;
	int value;
	if (*ch >= '0' && *ch <= '7') {			//8진수 처리
		do {
			num = 8 * num + (int)(*ch - '0');
			*ch = fgetc(sourceFile);
			count++;						//\뒤에 오는 팔진수는 3자리를 넘을 수 없으므로 count를 이용하여 3자리가 넘은경우 반복문을 종료한다.
		} while ((*ch >= '0') && (*ch <= '7') && count < 3);
		ungetc(*ch, sourceFile);		//retract
		return num;
	}
	else if ((*ch == 'X') || (*ch == 'x')) {		// hexa decimal
		while ((value = hexValue(*ch = fgetc(sourceFile))) == 0);		//haxValue함수를 이용하여 16진수 처리
		while(value != -1){			
			count++;
			num = 16 * num + value;
			value = hexValue(*ch = fgetc(sourceFile));
		}
		if (count > 2)			//16진수값이 3개 이상 들어간 경우 1바이트의 char에 결과값이 들어갈 수 없다.
			fprintf(stderr, "문자에 비해 너무 큽니다\n");
			
		
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
	if (token.number == tident || token.number == tstring)		//tstring인 경우에도 안의 내용을 출력한다.
		printf("number: %d, value: %s\n", token.number, token.value.id);
	else if (token.number == tintnumber)		//정수
		printf("number: %d, value: %d\n", token.number, token.value.num);
	else if (token.number == trealnumber)		//실수
		printf("number: %d, value: %lf\n", token.number, token.value.fnum);
	else
		printf("number: %d(%s)\n", token.number, tokenName[token.number]);

}