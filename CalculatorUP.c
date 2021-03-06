#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum TokenType {
	NUMBER = 'N', LOG = 'L',
	PLUS = '+', MINUS = '-',
	MUL = '*', DIV = '/',
	POW = '^', LP = '(',
	RP = ')'
};

typedef struct _Token {
	enum TokenType type;
	double value;
	struct _Token* next;
} Token;

void freeList(Token* token) {
	while (token != NULL) {
		Token* next = token->next;
		free(token);
		token = next;
	}
}

int parseToken(char** str, Token** token) {
	*token = (Token*)malloc(sizeof(Token));
	while (**str && (**str == ' ' || **str == '\n'))
		++(*str);
	switch (**str) {
	case '\0':
		free(*token);
		*token = NULL;
		return 1; // OK

	case '+':
	case '-':
	case '*':
	case '/':
	case '^':
	case '(':
	case ')':
		(*token)->type = **str;
		++(*str);
		break;

	default: {
		char dec_point = localeconv()->decimal_point[0];
		if (**str == dec_point || (**str >= '0' && **str <= '9')) {
			(*token)->type = NUMBER;
			sscanf_s(*str, "%lf", &((*token)->value));
			do {
				++(*str);
			} while (**str == dec_point || (**str >= '0' && **str <= '9'));
		}
		else if (strncmp(*str, "log", 3) == 0) {
			(*token)->type = LOG;
			*str += 3;
		}
		else {
			free(*token);
			*token = NULL;
			return 0; // Unexpected token
		}
		break;
	}
	}
	return 1; // OK
}

Token* lexer(char** str) {
	Token* first;
	parseToken(str, &first);
	Token* current = first;
	while (current) {
		Token* next;
		if (!parseToken(str, &next)) {
			current->next = NULL;
			freeList(first);
			return NULL;
		}
		current->next = next;
		current = next;
	}
	return first;
}

Token* parseLog(Token* token);
Token* parseExpression(Token* token);

Token* parsePrimary(Token* token) {
	if (!token) return NULL;
	if (token->type == NUMBER) {
		return token;
	}
	else if (token->type == MINUS) {
		Token* next = token->next;
		if (!next) return NULL;
		if (next->type == LP) {
			next = parseExpression(next, 0);
		}
		else if (next->type == LOG) {
			next = parseLog(next);
		}
		if (next && next->type == NUMBER) {
			*token = *next;
			token->value = -(token->value);
			free(next);
			return token;
		}
		else return NULL;
	}
	else if (token->type == LP) {
		return parseExpression(token, 0);
	}
	else return NULL;
}

Token* parsePow(Token* lhs) {
	lhs = parsePrimary(lhs);
	if (!lhs) return NULL;
	Token* token = lhs->next;
	if (token && token->type == POW) {
		Token* rhs = parsePow(token->next);
		if (!rhs) return NULL;
		lhs->type = NUMBER;
		lhs->value = pow(lhs->value, rhs->value);
		lhs->next = rhs->next;
		free(token);
		free(rhs);
	}
	return lhs;
}

Token* parseLog(Token* token) {
	if (!token) return NULL;
	if (token->type == LOG) {
		Token* arg = parsePow(token->next);
		if (!arg) return NULL;
		token->type = NUMBER;
		token->value = log(arg->value);
		token->next = arg->next;
		free(arg);
		return token;
	}
	else {
		return parsePow(token);
	}
}

Token* parseMul(Token* token) {
	Token* lhs = parseLog(token);
	if (!lhs) return NULL;
	token = lhs->next;
	if (token && (token->type == MUL || token->type == DIV)) {
		Token* rhs = parseLog(token->next);
		if (!rhs) return NULL;
		lhs->type = NUMBER;
		if (token->type == MUL) {
			lhs->value = (lhs->value * rhs->value);
		}
		else {
			lhs->value = (lhs->value / rhs->value);
		}
		lhs->next = rhs->next;
		free(token);
		free(rhs);
		return parseMul(lhs);
	}
	else {
		return lhs;
	}
}

Token* parseSum(Token* token) {
	Token* lhs = parseMul(token);
	if (!lhs) return NULL;
	token = lhs->next;
	if (token && (token->type == PLUS || token->type == MINUS)) {
		Token* rhs = parseMul(token->next);
		if (!rhs) return NULL;
		lhs->type = NUMBER;
		if (token->type == PLUS) {
			lhs->value = (lhs->value + rhs->value);
		}
		else {
			lhs->value = (lhs->value - rhs->value);
		}
		lhs->next = rhs->next;
		free(token);
		free(rhs);
		return parseSum(lhs);
	}
	else {
		return lhs;
	}
}

Token* parseExpression(Token* token, int ignorePars) {
	if (!token) return NULL;
	if (!ignorePars && token->type == LP) {
		Token* result = parseSum(token->next);
		if (!result || !(result->next)) return NULL;
		token->type = result->type;
		token->value = result->value;
		token->next = result->next->next;
		free(result->next);
		free(result);
	}
	else {
		token = parseSum(token);
	}
	return token;
}

double eval(char* str, double* res) {
	Token* tokens = lexer(&str);
	Token* result = parseExpression(tokens, 1);
	if (!result || result->type != NUMBER || result->next != NULL) {
		freeList(tokens);
		return 0;
	}
	*res = result->value;
	freeList(result);
	return 1;
}

int main() {
	setlocale(LC_ALL, "Russian");
	char str[256];
	while (1) {
		printf("������� ���������: ");
		fgets(str, sizeof(str), stdin);
		if (strcmp(str, "exit\n") == 0) break;
		double result;
		if (eval(str, &result)) {
			printf("�����: %lf\n", result);
		}
		else {
			puts("������ � ���������");
		}
	}
}
