/*
See LICENSE file in root folder
*/
#include "ShaderWriter/BaseTypes/AccelerationStructure.hpp"

#include <ShaderAST/Shader.hpp>
#include <ShaderAST/Stmt/StmtSimple.hpp>

namespace sdw
{
	AccelerationStructure::AccelerationStructure( ShaderWriter & writer
		, expr::ExprPtr expr
		, bool writeStmt )
		: Value{ writer, std::move( expr ), true }
	{
		if ( getType()
			&& getType()->getKind() != type::Kind::eAccelerationStructure )
		{
			throw std::runtime_error{ "Wrong type for acceleration structure." };
		}

		if ( writeStmt )
		{
			getShader()->addStmt( sdw::makeSimple( makeExpr( *this ) ) );
		}
	}

	AccelerationStructure::AccelerationStructure( Value rhs )
		: Value{ std::move( rhs ) }
	{
	}

	ast::type::TypePtr AccelerationStructure::makeType( ast::type::TypesCache & cache )
	{
		return cache.getAccelerationStructure();
	}
}