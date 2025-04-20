#include "../defines.h"
#include "../core/dstring.h"

enum TokenType {
	Token_Unknown,
	Token_OpenParen, 
	Token_CloseParen,
	Token_OpenBrace,
	Token_CloseBrace,
	Token_OpenBracket,
	Token_CloseBracket,
	Token_EndOfStream,
	Token_Number, 
	Token_Colon,
	Token_Semicolon,
	Token_Asterisk,
	Token_String,
	Token_Identifier,
	Token_Introspect,
	Token_Struct,
	Token_Error
};

struct Token {
	TokenType type;
	char* start;
	i32 length;
	i32 line;
};

struct Scanner {
	char* start;
	char* current;
	int line;
};

Scanner scanner;


void InitScanner(char* src) {
	scanner.start = src;
	scanner.current = src;
	scanner.line = 1;
}

static Token MakeToken(TokenType type) {
	Token token = {
		.type = type,
		.start = scanner.start,
		.length = scanner.current - scanner.start,
		.line = scanner.line
	};
	return token;
}

static Token ErrorToken(char* message) {
	Token token = {
		.type = Token_Error,
		.start = message,
		.length = StringLength((u8*)message),
		.line = scanner.line
	};
	return token;
}

static inline char ScannerPeek() {
	return *(scanner.current);
}

static inline char ScannerPeekNext() {
	return *(scanner.current + 1);
}

static inline b8 ScannerIsAtEnd() {
	return ScannerPeek() == '\0';
}

static inline char ScannerAdvance() {
	char result = ScannerPeek();
	scanner.current++;
	return result;
}

static b8 ScannerMatch(char expected) {
	if (ScannerIsAtEnd()) {
		return false;
	}
	if (ScannerPeek() != expected) {
		return false;
	}
	scanner.current++;
	return true;
}

static void SkipWhiteSpace() {
	for (;;) {
		char curr = ScannerPeek();
		if (IsWhiteSpace(curr)) {
			ScannerAdvance();
		} else {
			return;
		}
	}
}

static Token String() {
	while (ScannerPeek() != '"' && ScannerPeek() != '\'' && !ScannerIsAtEnd()) {
		if (ScannerPeek() == '\n')
			scanner.line++;
		ScannerAdvance();
	}
	if (ScannerIsAtEnd())
		return ErrorToken("Unterminated string.");
	ScannerAdvance();
	return MakeToken(Token_String);
}

static TokenType CheckKeyword(i32 start, i32 length, char* rest, TokenType type) {
	if (scanner.current - scanner.start == start + length && StringsEquali(scanner.start + start, rest)) {
		return type;
	}
	return Token_Identifier;
}

static Token ScannerNumber() {
	while (IsDigit(ScannerPeek()))
		scanner.current++;
	if (ScannerPeek() == '.' && IsDigit(ScannerPeekNext())) {
		scanner.current++;
		while (IsDigit(ScannerPeek()))
			scanner.current++;
	}
	return MakeToken(Token_Number);
}

static TokenType IdentifierType() {
	switch (*scanner.start) {
		case 'i': return CheckKeyword(1, 9, "ntrospect", Token_Introspect);
	}
	return Token_Identifier;
}

static Token ScannerIdentifier() {
	while (IsAlNum(ScannerPeek())) {
		scanner.current++;
	}
	return MakeToken(IdentifierType());
}

static Token ScanToken() {
	SkipWhiteSpace();
	scanner.start = scanner.current;

	if (ScannerIsAtEnd()) {
		return MakeToken(Token_EndOfStream);
	}
	char c = ScannerAdvance();
	if (IsAlpha(c))
		return ScannerIdentifier();
	if (IsDigit(c))
		return ScannerNumber();
	switch (c) {
		case '(': return MakeToken(Token_OpenParen);
		case ')': return MakeToken(Token_CloseParen);
		case '{': return MakeToken(Token_OpenBrace);
		case '}': return MakeToken(Token_CloseBrace);
		case '[': return MakeToken(Token_OpenBracket);
		case ']': return MakeToken(Token_CloseBracket);
		case '*': return MakeToken(Token_Asterisk);
		case '"': return String();
		case ':': return MakeToken(Token_Colon);
		case ';': return MakeToken(Token_Semicolon);
	}

	return ErrorToken("Unexpected character.");
}

static Token* ScanTokens() {
	while (!ScannerIsAtEnd()) {
		scanner.start = scanner.current;
		ScanToken();
	}
}

static void ParseIntrospectable() {

}

static void ParseStruct() {

}

static void ParseStructMember() {
	
}

#include <stdio.h>
#include <stdlib.h>

char* ReadEntireFileAndNullTerminate(char* filename) {
	char* result = 0;
	FILE* file = fopen(filename, "r");
	if (file) {
		fseek(file, 0, SEEK_END);
		int file_size = ftell(file); 
		fseek(file, 0, SEEK_SET);
		result = (char*)malloc(file_size + 1);
		fread(result, file_size, 1, file);
		result[file_size] = 0;
		fclose(file);
	}
	return result;
}

int main(int arg_count, char** args) {
	char* file_contents = ReadEntireFileAndNullTerminate("../../../src/renderer/renderer.h");
	InitScanner(file_contents);
	ScanTokens();
}