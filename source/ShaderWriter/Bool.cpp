#include "ShaderWriter/Bool.hpp"

#include <ASTGenerator/Expr/ExprEqual.hpp>
#include <ASTGenerator/Expr/ExprIdentifier.hpp>
#include <ASTGenerator/Expr/ExprLiteral.hpp>
#include <ASTGenerator/Expr/ExprLogAnd.hpp>
#include <ASTGenerator/Expr/ExprLogOr.hpp>
#include <ASTGenerator/Expr/ExprNotEqual.hpp>

namespace sdw
{
	Boolean::Boolean( stmt::Container * container
		, expr::ExprPtr expr )
		: Value{ container, std::move( expr ) }
	{
	}

	Boolean::Boolean( Boolean && rhs )
		: Value{ std::move( rhs ) }
	{
	}

	Boolean::Boolean( Boolean const & rhs )
		: Value{ rhs }
	{
	}

	Boolean::Boolean( Value const & rhs )
		: Value{ rhs }
	{
	}

	Boolean & Boolean::operator=( bool rhs )
	{
		addStmt( *m_container
			, stmt::makeSimple( expr::makeAssign( type::getBool()
				, make( m_expr )
				, make( rhs ) ) ) );
		return *this;
	}

	Boolean::operator bool()
	{
		return false;
	}

	Boolean operator==( Value const & lhs, Value const & rhs )
	{
		return Boolean{ lhs.m_container
			, expr::makeEqual( make( lhs )
				, make( rhs ) ) };
	}

	Boolean operator!=( Value const & lhs, Value const & rhs )
	{
		return Boolean{ lhs.m_container
			, expr::makeNotEqual( make( lhs )
				, make( rhs ) ) };
	}

	Boolean operator||( Boolean const & lhs, Boolean const & rhs )
	{
		return Boolean{ lhs.m_container
			, expr::makeLogOr( make( lhs )
				, make( rhs ) ) };
	}

	Boolean operator&&( Boolean const & lhs, Boolean const & rhs )
	{
		return Boolean{ lhs.m_container
			, expr::makeLogAnd( make( lhs )
				, make( rhs ) ) };
	}
}
