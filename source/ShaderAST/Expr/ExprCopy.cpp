/*
See LICENSE file in root folder
*/
#include "ShaderAST/Expr/ExprCopy.hpp"
#include "ShaderAST/Expr/ExprVisitor.hpp"

namespace ast::expr
{
	Copy::Copy( ExprPtr op )
		: Unary{ std::move( op )
			, Kind::eCopy }
	{
	}

	void Copy::accept( VisitorPtr vis )
	{
		vis->visitCopyExpr( this );
	}
}