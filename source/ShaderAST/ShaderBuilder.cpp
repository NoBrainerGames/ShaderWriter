/*
See LICENSE file in root folder
*/
#include "ShaderAST/ShaderBuilder.hpp"

#include "ShaderAST/Stmt/StmtIf.hpp"
#include "ShaderAST/Stmt/StmtSimple.hpp"
#include "ShaderAST/Stmt/StmtSwitch.hpp"
#include "ShaderAST/Visitors/CloneExpr.hpp"

#include <algorithm>
#include <string_view>

namespace ast
{
	namespace builder
	{
		static auto findVariable( std::set< var::VariablePtr > const & vars
			, std::string_view name )
		{
			return std::find_if( vars.begin()
				, vars.end()
				, [name]( var::VariablePtr const & var )
				{
					return ( var->getFullName() == name
						|| var->getName() == name );
				} );
		}

		static auto findMbrVariable( std::set< var::VariablePtr > const & vars
			, var::VariablePtr outer
			, std::string_view name )
		{
			return std::find_if( vars.begin()
				, vars.end()
				, [name, &outer]( var::VariablePtr const & var )
				{
					return var->isMember()
						&& var->getOuter() == outer
						&& ( var->getFullName() == name
							|| var->getName() == name );
				} );
		}
	}

	ShaderBuilder::ShaderBuilder( Shader & shader )
		: m_shader{ shader }
	{
		push( shader.getStatements(), var::VariableList{} );
	}

	void ShaderBuilder::push( stmt::Container * container
		, var::VariableList vars )
	{
		m_blocks.push_back( { {}, container } );

		if ( m_blocks.size() > 1u )
		{
			auto it = m_blocks.begin() + ptrdiff_t( m_blocks.size() - 2 );
			// move variables contained in the given list to the new scope.
			for ( auto & var : vars )
			{
				auto itVar = it->registered.find( var );

				if ( itVar != it->registered.end() )
				{
					it->registered.erase( itVar );
				}

				registerVariable( var );
			}

			// Add variables from previous scopes to current scope.
			it = m_blocks.begin();
			auto end = m_blocks.begin() + ptrdiff_t( m_blocks.size() - 1 );
			auto & block = m_blocks.back();

			while ( it != end )
			{
				for ( auto var : it->registered )
				{
					block.registered.insert( var );
				}

				++it;
			}
		}
	}

	void ShaderBuilder::pop()
	{
		m_blocks.erase( m_blocks.begin() + ptrdiff_t( m_blocks.size() - 1 ) );
	}

	void ShaderBuilder::saveNextExpr()
	{
		assert( m_savedStmt == nullptr );
		m_ignore = true;
	}

	expr::ExprPtr ShaderBuilder::loadExpr( expr::ExprPtr expr )
	{
		if ( m_savedStmt != nullptr )
		{
			auto result = ExprCloner::submit( expr->getExprCache()
				, static_cast< stmt::Simple const & >( *m_savedStmt ).getExpr() );
			m_savedStmt = nullptr;
			return result;
		}

		m_ignore = false;
		return expr;
	}

	void ShaderBuilder::beginIf( expr::ExprPtr condition )
	{
		auto stmt = getStmtCache().makeIf( std::move( condition ) );
		m_ifStmt.push_back( stmt.get() );
		pushScope( std::move( stmt ) );
	}

	void ShaderBuilder::beginElseIf( expr::ExprPtr condition )
	{
		push( m_ifStmt.back()->createElseIf( std::move( condition ) )
			, ast::var::VariableList{} );
	}

	void ShaderBuilder::beginElse()
	{
		push( m_ifStmt.back()->createElse()
			, ast::var::VariableList{} );
	}

	void ShaderBuilder::endIf()
	{
		m_ifStmt.pop_back();
	}

	void ShaderBuilder::beginSwitch( expr::ExprPtr value )
	{
		auto stmt = getStmtCache().makeSwitch( getExprCache().makeSwitchTest( std::move( value ) ) );
		m_switchStmt.push_back( stmt.get() );
		pushScope( std::move( stmt ) );
	}

	void ShaderBuilder::beginCase( expr::LiteralPtr literal )
	{
		push( m_switchStmt.back()->createCase( getExprCache().makeSwitchCase( std::move( literal ) ) )
			, ast::var::VariableList{} );
	}

	void ShaderBuilder::beginDefault()
	{
		push( m_switchStmt.back()->createDefault()
			, ast::var::VariableList{} );
	}

	void ShaderBuilder::endSwitch()
	{
		m_switchStmt.pop_back();
	}

	void ShaderBuilder::pushScope( ast::stmt::ContainerPtr container )
	{
		m_currentStmts.emplace_back( std::move( container ) );
		push( m_currentStmts.back().get()
			, ast::var::VariableList{} );
	}

	void ShaderBuilder::popScope()
	{
		pop();
		addStmt( std::move( m_currentStmts.back() ) );
		m_currentStmts.erase( m_currentStmts.begin() + ptrdiff_t( m_currentStmts.size() - 1u ) );
	}

	bool ShaderBuilder::hasFunction( std::string_view name )const
	{
		auto it = builder::findVariable( m_blocks.front().registered, name );
		return m_blocks.front().registered.end() != it;
	}

	var::VariablePtr ShaderBuilder::getFunction( std::string name )
	{
		auto it = builder::findVariable( m_blocks.front().registered, name );

		if ( it == m_blocks.front().registered.end() )
		{
			std::string text;
			text += "No registered function with the name [" + std::string( name ) + "].";
			throw std::runtime_error{ text };
		}

		return *it;
	}

	var::VariablePtr ShaderBuilder::registerFunction( std::string name
		, type::FunctionPtr type )
	{
		auto it = builder::findVariable( m_blocks.front().registered, name );

		if ( it != m_blocks.front().registered.end()
			&& type != it->get()->getType() )
		{
			std::string text;
			text += "A function with the name [" + std::string( name ) + "] is already registered, with a different type.";
			throw std::runtime_error{ text };
		}

		auto result = var::makeFunction( ++getData().nextVarId
			, std::move( type )
			, std::move( name ) );
		m_shader.registerVar( 1u, result );
		m_blocks.front().registered.emplace( result );
		return result;
	}

	uint32_t ShaderBuilder::getNextVarId()
	{
		return ++getData().nextVarId;
	}

	bool ShaderBuilder::hasVariable( std::string_view name )const
	{
		auto & block = m_blocks.back();
		auto it = builder::findVariable( block.registered, name );
		return it != block.registered.end();
	}

	void ShaderBuilder::registerVariable( var::VariablePtr var )
	{
		auto & block = m_blocks.back();
#if !defined( NDEBUG )
		auto ires = block.registered.emplace( var );
		assert( ires.second );
#else
		block.registered.emplace( var );
#endif
		m_shader.registerVar( m_blocks.size(), var );

		if ( var->getType()->getRawKind() == type::Kind::eTessellationControlInput )
		{
			getData().tessellationControlPoints = static_cast< type::TessellationControlInput const & >( *var->getType() ).getInputVertices();
		}
	}

	var::VariablePtr ShaderBuilder::registerName( std::string name
		, type::TypePtr type
		, uint64_t flags )
	{
		auto var = var::makeVariable( ++getData().nextVarId
			, type
			, std::move( name )
			, flags );
		registerVariable( var );
		return var;
	}

	var::VariablePtr ShaderBuilder::registerName( std::string name
		, type::TypePtr type
		, var::Flag flag )
	{
		return registerName( std::move( name )
			, type
			, uint64_t( flag ) );
	}

	var::VariablePtr ShaderBuilder::registerName( std::string name
		, type::TypePtr type )
	{
		return registerName( std::move( name )
			, type
			, 0u );
	}

	var::VariablePtr ShaderBuilder::registerMember( var::VariablePtr outer
		, std::string name
		, type::TypePtr type
		, uint64_t flags )
	{
		flags |= uint64_t( var::Flag::eMember );
		auto result = var::makeVariable( ++getData().nextVarId
			, outer
			, type
			, std::move( name )
			, flags );
		registerVariable( result );
		return result;
	}

	var::VariablePtr ShaderBuilder::registerMember( var::VariablePtr outer
		, std::string name
		, type::TypePtr type
		, var::Flag flag )
	{
		return registerMember( std::move( outer )
			, std::move( name )
			, type
			, uint64_t( flag ) );
	}

	var::VariablePtr ShaderBuilder::registerMember( var::VariablePtr outer
		, std::string name
		, type::TypePtr type )
	{
		auto flags = outer->isUniform()
			? var::Flag::eUniform
			: var::Flag( 0u );
		return registerMember( std::move( outer )
			, std::move( name )
			, type
			, flags );
	}

	var::VariablePtr ShaderBuilder::registerStaticConstant( std::string name
		, type::TypePtr type )
	{
		auto it = builder::findVariable( m_blocks.front().registered, name );

		if ( it != m_blocks.front().registered.end()
			&& type != it->get()->getType() )
		{
			std::string text;
			text += "A static constant with the name [" + std::string( name ) + "] is already registered, with a different type.";
			throw std::runtime_error{ text };
		}

		auto result = m_blocks.front().registered.emplace( var::makeVariable( ++getData().nextVarId
			, std::move( type )
			, std::move( name )
			, var::Flag::eStatic | var::Flag::eConstant ) ).first;
		m_shader.registerVar( 1u, *result );
		getData().constants.emplace( std::move( name ), type );
		return *result;
	}

	var::VariablePtr ShaderBuilder::registerSpecConstant( std::string name
		, uint32_t location
		, type::TypePtr type )
	{
		auto result = registerName( name
			, type
			, var::Flag::eSpecialisationConstant );
		getData().specConstants.emplace( std::move( name ), SpecConstantInfo{ { type, location } } );
		return result;
	}

	var::VariablePtr ShaderBuilder::registerAccelerationStructure( std::string name
		, type::TypePtr type
		, uint32_t binding
		, uint32_t set
		, bool enabled )
	{
		auto result = registerName( std::move( name )
			, type
			, var::Flag::eUniform | var::Flag::eConstant );

		if ( enabled )
		{
			auto accType = getNonArrayType( type );
			assert( accType->getKind() == type::Kind::eAccelerationStructure );
			getData().accelerationStruct = AccStructInfo{ std::static_pointer_cast< type::AccelerationStructure >( accType )
				, binding
				, set };
		}

		return result;
	}

	var::VariablePtr ShaderBuilder::registerSampler( std::string name
		, type::TypePtr type
		, uint32_t binding
		, uint32_t set
		, bool enabled )
	{
		var::VariablePtr result;

		if ( enabled )
		{
			result = registerName( name
				, type
				, var::Flag::eUniform | var::Flag::eConstant );

			auto splType = getNonArrayType( type );
			assert( splType->getKind() == type::Kind::eSampler );
			getData().samplers.emplace( std::move( name ), SamplerInfo{ { type, { binding, set } } } );
		}
		else
		{
			result = registerName( std::move( name )
				, type
				, var::Flag::eUniform | var::Flag::eConstant );
		}

		return result;
	}

	var::VariablePtr ShaderBuilder::registerSampledImage( std::string name
		, type::TypePtr type
		, uint32_t binding
		, uint32_t set
		, bool enabled )
	{
		var::VariablePtr result;

		if ( enabled )
		{
			result = registerName( name
				, type
				, var::Flag::eUniform | var::Flag::eConstant );

			auto splType = getNonArrayType( type );
			assert( splType->getKind() == type::Kind::eSampledImage );

			if ( static_cast< type::SampledImage const & >( *splType ).getConfig().dimension == type::ImageDim::eBuffer )
			{
				getData().uniformTexels.emplace( std::move( name ), TextureInfo{ { type, { binding, set } } } );
			}
			else
			{
				getData().sampled.emplace( std::move( name ), TextureInfo{ { type, { binding, set } } } );
			}
		}
		else
		{
			result = registerName( std::move( name )
				, type
				, var::Flag::eUniform | var::Flag::eConstant );
		}

		return result;
	}

	var::VariablePtr ShaderBuilder::registerTexture( std::string name
		, type::TypePtr type
		, uint32_t binding
		, uint32_t set
		, bool enabled )
	{
		var::VariablePtr result;

		if ( enabled )
		{
			result = registerName( name
				, type
				, var::Flag::eUniform | var::Flag::eConstant );

			auto imgType = getNonArrayType( type );
			assert( imgType->getKind() == type::Kind::eCombinedImage );

			if ( static_cast< type::CombinedImage const & >( *imgType ).getConfig().dimension == type::ImageDim::eBuffer )
			{
				getData().uniformTexels.emplace( std::move( name ), TextureInfo{ { type, { binding, set } } } );
			}
			else
			{
				getData().textures.emplace( std::move( name ), TextureInfo{ { type, { binding, set } } } );
			}
		}
		else
		{
			result = registerName( std::move( name )
				, type
				, var::Flag::eUniform | var::Flag::eConstant );
		}

		return result;
	}

	var::VariablePtr ShaderBuilder::registerImage( std::string name
		, type::TypePtr type
		, uint32_t binding
		, uint32_t set
		, bool enabled )
	{
		var::VariablePtr result;

		if ( enabled )
		{
			result = registerName( name
				, type
				, var::Flag::eUniform | var::Flag::eConstant );

			auto imgType = getNonArrayType( type );
			assert( imgType->getKind() == type::Kind::eImage );

			if ( static_cast< type::Image const & >( *imgType ).getConfig().dimension == type::ImageDim::eBuffer )
			{
				getData().storageTexels.emplace( std::move( name ), ImageInfo{ { type, { binding, set } } } );
			}
			else
			{
				getData().images.emplace( std::move( name ), ImageInfo{ { type, { binding, set } } } );
			}
		}
		else
		{
			result = registerName( std::move( name )
				, type
				, var::Flag::eUniform | var::Flag::eConstant );
		}

		return result;
	}

	var::VariablePtr ShaderBuilder::registerInput( EntryPoint entryPoint
		, std::string name
		, uint32_t location
		, uint64_t attributes
		, type::TypePtr type )
	{
		auto & inputs = getData().inputs.emplace( entryPoint, ShaderData::InputMap{} ).first->second;
		auto it = std::find_if( inputs.begin()
			, inputs.end()
			, [&location]( std::map< std::string, InputInfo >::value_type const & lookup )
			{
				return lookup.second.location == location;
			} );

		if ( inputs.end() == it )
		{
			inputs.emplace( name, InputInfo{ { type, location } } );
		}

		if ( hasVar( name ) )
		{
			return getVar( name );
		}

		auto kind = getNonArrayType( type )->getKind();
		auto flags = attributes;

		if ( ( getType() != ShaderStage::eVertex && getType() != ShaderStage::eCompute )
			&& !isMeshStage( getType() )
			&& !isRayTraceStage( getType() )
			&& ( isSignedIntType( kind ) || isUnsignedIntType( kind ) ) )
		{
			flags = flags | var::Flag::eFlat;
		}

		auto result = registerName( std::move( name )
			, type
			, flags | var::Flag::eShaderInput );
		return result;
	}

	var::VariablePtr ShaderBuilder::registerOutput( EntryPoint entryPoint
		, std::string name
		, uint32_t location
		, uint64_t attributes
		, type::TypePtr type )
	{
		auto & outputs = getData().outputs.emplace( entryPoint, ShaderData::OutputMap{} ).first->second;
		auto it = std::find_if( outputs.begin()
			, outputs.end()
			, [&location]( std::map< std::string, OutputInfo >::value_type const & lookup )
			{
				return lookup.second.location == location;
			} );

		if ( outputs.end() == it )
		{
			outputs.emplace( name, OutputInfo{ { type, location } } );
		}

		if ( hasVar( name ) )
		{
			return getVar( name );
		}

		auto kind = getNonArrayType( type )->getKind();
		auto flags = attributes;

		if ( ( getType() != ShaderStage::eFragment && getType() != ShaderStage::eCompute )
			&& !isMeshStage( getType() )
			&& !isRayTraceStage( getType() )
			&& ( isSignedIntType( kind ) || isUnsignedIntType( kind ) ) )
		{
			flags = flags | var::Flag::eFlat;
		}

		auto result = registerName( std::move( name )
			, type
			, flags | var::Flag::eShaderOutput );
		return result;
	}

	var::VariablePtr ShaderBuilder::registerInOut( std::string name
		, uint64_t attributes
		, type::TypePtr type )
	{
		if ( getData().inOuts.empty() )
		{
			getData().inOuts.emplace( name, InOutInfo{ { type } } );
		}

		if ( hasVar( name ) )
		{
			return getVar( name );
		}

		auto result = registerName( std::move( name )
			, type
			, attributes | var::Flag::eShaderOutput | var::Flag::eShaderInput );
		return result;
	}

	var::VariablePtr ShaderBuilder::registerBuiltin( Builtin builtin
		, type::TypePtr type
		, var::Flag flag )
	{
		auto result = var::makeBuiltin( ++getData().nextVarId
			, builtin
			, type
			, flag );
		registerVariable( result );
		return result;
	}

	var::VariablePtr ShaderBuilder::registerBlockVariable( std::string name
		, type::TypePtr type )
	{
		return registerLocale( std::move( name ), type );
	}

	var::VariablePtr ShaderBuilder::registerLocale( std::string name
		, type::TypePtr type )
	{
		return registerName( std::move( name )
			, type
			, var::Flag::eLocale );
	}

	var::VariablePtr ShaderBuilder::registerLoopVar( std::string name
		, type::TypePtr type )
	{
		return registerName( std::move( name )
			, type
			, var::Flag::eLocale | var::Flag::eLoopVar );
	}

	var::VariablePtr ShaderBuilder::registerParam( std::string name
		, type::TypePtr type )
	{
		return registerName( std::move( name )
			, type
			, var::Flag::eParam );
	}

	var::VariablePtr ShaderBuilder::registerInParam( std::string name
		, type::TypePtr type )
	{
		return registerName( std::move( name )
			, type
			, var::Flag::eInputParam );
	}

	var::VariablePtr ShaderBuilder::registerOutParam( std::string name
		, type::TypePtr type )
	{
		return registerName( std::move( name )
			, type
			, var::Flag::eOutputParam );
	}

	var::VariablePtr ShaderBuilder::registerInOutParam( std::string name
		, type::TypePtr type )
	{
		return registerName( std::move( name )
			, type
			, uint64_t( var::Flag::eInputParam ) | uint64_t( var::Flag::eOutputParam ) );
	}

	bool ShaderBuilder::hasVar( std::string_view name )const
	{
		auto curBlockIt = std::find_if( m_blocks.crbegin()
			, m_blocks.crend()
			, [&name]( Block const & lookup )
			{
				return builder::findVariable( lookup.registered, name ) != lookup.registered.end();
			} );
		return curBlockIt != m_blocks.rend();
	}

	var::VariablePtr ShaderBuilder::getVar( std::string_view name )const
	{
		std::set< var::VariablePtr >::const_iterator it;
		auto curBlockIt = std::find_if( m_blocks.crbegin()
			, m_blocks.crend()
			, [&name, &it]( Block const & lookup )
			{
				it = builder::findVariable( lookup.registered, name );
				return it != lookup.registered.end();
			} );

		if ( curBlockIt == m_blocks.rend() )
		{
			std::string text;
			text += "No registered variable with the name [" + std::string( name ) + "].";
			throw std::runtime_error{ text };
		}

		return *it;
	}

	var::VariablePtr ShaderBuilder::getMemberVar( var::VariablePtr outer
		, std::string_view name )const
	{
		std::set< var::VariablePtr >::const_iterator it;
		auto curBlockIt = std::find_if( m_blocks.crbegin()
			, m_blocks.crend()
			, [&outer, &name, &it]( Block const & lookup )
			{
				it = builder::findMbrVariable( lookup.registered, outer, name );
				return it != lookup.registered.end();
			} );

		if ( curBlockIt == m_blocks.rend() )
		{
			std::string text;
			text += "No registered member variable with the name [" + std::string( name ) + "].";
			throw std::runtime_error{ text };
		}

		return *it;
	}

	void ShaderBuilder::addStmt( stmt::StmtPtr stmt )
	{
		if ( m_ignore )
		{
			if ( stmt->getKind() == stmt::Kind::eSimple )
			{
				m_savedStmt = std::move( stmt );
			}

			m_ignore = false;
		}
		else
		{
			getContainer()->addStmt( std::move( stmt ) );
		}
	}

	void ShaderBuilder::addGlobalStmt( stmt::StmtPtr stmt )
	{
		if ( m_ignore )
		{
			if ( stmt->getKind() == stmt::Kind::eSimple )
			{
				m_savedStmt = std::move( stmt );
			}

			m_ignore = false;
		}
		else
		{
			getGlobalContainer()->addStmt( std::move( stmt ) );
		}
	}

	void ShaderBuilder::registerSsbo( std::string name
		, SsboInfo const & info )
	{
		getData().ssbos.emplace( std::move( name ), info );
	}

	void ShaderBuilder::registerUbo( std::string name
		, UboInfo const & info )
	{
		getData().ubos.emplace( std::move( name ), info );
	}

	void ShaderBuilder::registerPcb( std::string name
		, InterfaceBlock const & info )
	{
		getData().pcbs.emplace( std::move( name ), info );
	}

	void ShaderBuilder::registerShaderRecord( std::string name
		, ShaderRecordInfo const & info )
	{
		getData().shaderRecords.emplace( std::move( name ), info );
	}

	expr::ExprPtr ShaderBuilder::getDummyExpr( type::TypePtr type )const
	{
		return getExprCache().makeDummyExpr( type );
	}

	void ShaderBuilder::doPushScope( ast::stmt::ContainerPtr container
		, ast::var::VariableList vars )
	{
		m_currentStmts.emplace_back( std::move( container ) );
		push( m_currentStmts.back().get(), std::move( vars ) );
	}
}