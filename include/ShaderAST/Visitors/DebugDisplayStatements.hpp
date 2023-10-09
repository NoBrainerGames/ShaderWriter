/*
See LICENSE file in root folder
*/
#ifndef ___AST_DebugStmtVisitor_H___
#define ___AST_DebugStmtVisitor_H___
#pragma once

#include "ShaderAST/Expr/Expr.hpp"
#include "ShaderAST/Stmt/Stmt.hpp"
#include "ShaderAST/Type/Type.hpp"

namespace ast::debug
{
	SDAST_API std::string displayStatements( stmt::Stmt * stmt );
	SDAST_API std::string displayExpression( expr::Expr * expr );
	SDAST_API std::string getTypeName( type::TypePtr type );
	SDAST_API std::string getTypeName( type::Kind kind );
	SDAST_API std::string getImageFormatName( type::ImageFormat value );
	SDAST_API std::string getImageTypeName( ast::type::ImageFormat format
		, ast::type::ImageDim dim
		, bool arrayed
		, bool ms
		, bool depth );
	SDAST_API std::string getImageTypeName( ast::type::ImageFormat format
		, ast::type::ImageDim dim
		, bool arrayed
		, bool ms );
	SDAST_API std::string getImageTypeName( ast::type::ImageFormat format
		, ast::type::AccessKind access
		, ast::type::ImageDim dim
		, bool arrayed
		, bool ms );

	inline std::string displayExpression( expr::ExprPtr const & expr )
	{
		return displayExpression( expr.get() );
	}

	template< typename ExprT >
	inline std::string displayExpression( expr::ExprPtrT< ExprT > const & expr )
	{
		return displayExpression( expr.get() );
	}
}

#endif
