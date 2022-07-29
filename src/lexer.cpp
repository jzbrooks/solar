#include "lexer.hpp"

#include <cctype>
#include <sstream>
#include <unordered_map>

static SourcePosition get_position(const Lexer &lexer) {
  return {lexer.line + 1, lexer.offset % (lexer.line + 1)};
}

static std::unordered_map<std::string, Token::Kind> reserved_words{
    // NOLINT(cert-err58-cpp)
    {"else", Token::Kind::ELSE}, {"func", Token::Kind::FUNC},
    {"if", Token::Kind::IF},     {"return", Token::Kind::RETURN},
    {"var", Token::Kind::VAR},
};

Token Lexer::next() {
  Token token; // NOLINT(cppcoreguidelines-pro-type-member-init)

  eat_whitespace();

  auto position = get_position(*this);

  auto ch = input->at(offset);

  if (offset >= input->size() || ch == EOF) {
    return {Token::Kind::END, "", position};
  }

  switch (ch) {
  case '+':
    token = {Token::Kind::PLUS, extractLexeme(1), position};
    break;
  case '-':
    if (match('>')) {
      token = {Token::Kind::ARROW, extractLexeme(2), position};
    } else {
      token = {Token::Kind::MINUS, extractLexeme(1), position};
    }
    break;
  case '*':
    token = {Token::Kind::STAR, extractLexeme(1), position};
    break;
  case '/':
    token = {Token::Kind::SLASH, extractLexeme(1), position};
    break;
  case '=':
    if (match('=')) {
      token = {Token::Kind::EQUAL, extractLexeme(2), position};
    } else {
      token = {Token::Kind::ASSIGN, extractLexeme(1), position};
    }
    break;
  case '<':
    if (match('=')) {
      token = {Token::Kind::LESS_EQUAL, extractLexeme(2), position};
    } else {
      token = {Token::Kind::LESS, extractLexeme(1), position};
    }
    break;
  case '>':
    if (match('=')) {
      token = {Token::Kind::GREATER_EQUAL, extractLexeme(2), position};
    } else {
      token = {Token::Kind::GREATER, extractLexeme(1), position};
    }
    break;
  case '(':
    token = {Token::Kind::LPAREN, extractLexeme(1), position};
    break;
  case ')':
    token = {Token::Kind::RPAREN, extractLexeme(1), position};
    break;
  case '{':
    token = {Token::Kind::LBRACE, extractLexeme(1), position};
    break;
  case '}':
    token = {Token::Kind::RBRACE, extractLexeme(1), position};
    break;
  case '[':
    token = {Token::Kind::LBRACKET, extractLexeme(1), position};
    break;
  case ']':
    token = {Token::Kind::RBRACKET, extractLexeme(1), position};
    break;
  case ',':
    token = {Token::Kind::COMMA, extractLexeme(1), position};
    break;
  case ':':
    token = {Token::Kind::COLON, extractLexeme(1), position};
    break;
  case '!':
    if (match('=')) {
      token = {Token::Kind::NOT_EQUAL, extractLexeme(2), position};
    } else {
      token = {Token::Kind::NEGATE, extractLexeme(1), position};
    }
    break;
  case '"':
    token = read_string();
    break;
  default:
    if (isalpha(ch) || ch == '_') {
      token = read_word();
    } else if (isdigit(ch)) {
      token = read_number();
    } else {
      token = {Token::Kind::INVALID, extractLexeme(1), position};
    }
    break;
  }

  offset += token.lexeme.size();

  return token;
}

std::string Lexer::extractLexeme(size_t length) const {
  return {input->data() + offset, length};
}

void Lexer::eat_whitespace() {
  for (auto it = input->begin() + offset; it != input->end() && isspace(*it);
       ++it) {
    if (*it == '\n')
      line += 1;
    offset += 1;
  }
}

Token Lexer::read_word() const {
  std::stringstream word_stream;
  auto position = get_position(*this);

  for (auto it = input->begin() + offset;
       it != input->end() && (isalnum(*it) || *it == '_'); ++it) {
    word_stream << *it;
  }

  auto word = word_stream.str();
  auto length = static_cast<int>(word.length());

  auto reserved_word = reserved_words.find(word);
  if (reserved_word != reserved_words.end()) {
    return Token{reserved_word->second, extractLexeme(length), position};
  }

  return Token{Token::Kind::IDENTIFIER, extractLexeme(length), position};
}

Token Lexer::read_number() const {
  auto position = get_position(*this);

  auto length = 0;
  for (auto it = input->begin() + offset;
       it != input->end() && (isdigit(*it) || *it == '.'); ++it) {
    length += 1;
  }

  if (input->size() > offset + length + 3) {
    std::string slice(input->data() + offset + length, 3);
    if (slice == "u32" || slice == "u64" || slice == "f32" || slice == "i32") {
      length += 3;
    }
  }

  return Token{Token::Kind::NUMBER, extractLexeme(length), position};
}

Token Lexer::read_string() {
  auto position = get_position(*this);
  auto length = 2; // account for delimiting double quotes
  for (auto it = input->begin() + offset + 1; it != input->end() && *it != '"';
       ++it) {
    length += 1;
  }

  return Token{Token::Kind::STRING, extractLexeme(length), position};
}

bool Lexer::match(char character) const {
  int next = offset + 1;
  return next < input->size() && input->at(next) == character;
}