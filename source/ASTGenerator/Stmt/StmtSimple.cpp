/*
See LICENSE file in root folder
*/
#include "ASTGenerator/Stmt/StmtSimple.hpp"

#include "ASTGenerator/Stmt/StmtVisitor.hpp"

namespace ast
{
	StmtSimple::StmtSimple( ExprPtr expr )
		: Stmt{ Kind::eSimple }
		, m_expr{ std::move( expr ) }
	{
	}

	void StmtSimple::accept( StmtVisitorPtr vis )
	{
		vis->visitSimpleStmt( this );
	}
}
