#include "ast.hpp"

Token ast::Type::Primitive::BOOL{ Token::Kind::IDENTIFIER, "bool" };
Token ast::Type::Primitive::INT32{ Token::Kind::IDENTIFIER, "int32" };
Token ast::Type::Primitive::INT64{ Token::Kind::IDENTIFIER, "int64" };
Token ast::Type::Primitive::UINT32{ Token::Kind::IDENTIFIER, "uint32" };
Token ast::Type::Primitive::UINT64{ Token::Kind::IDENTIFIER, "uint64" };
Token ast::Type::Primitive::FLOAT32{ Token::Kind::IDENTIFIER, "float32" };
Token ast::Type::Primitive::FLOAT64{ Token::Kind::IDENTIFIER, "float64" };
