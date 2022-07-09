#include "ast.hpp"

Token ast::Type::Primitive::BOOL{Token::Kind::IDENTIFIER, "bool"};
Token ast::Type::Primitive::INT32{Token::Kind::IDENTIFIER, "i32"};
Token ast::Type::Primitive::INT64{Token::Kind::IDENTIFIER, "i64"};
Token ast::Type::Primitive::UINT32{Token::Kind::IDENTIFIER, "u32"};
Token ast::Type::Primitive::UINT64{Token::Kind::IDENTIFIER, "u64"};
Token ast::Type::Primitive::FLOAT32{Token::Kind::IDENTIFIER, "f32"};
Token ast::Type::Primitive::FLOAT64{Token::Kind::IDENTIFIER, "f64"};
