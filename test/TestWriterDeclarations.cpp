#include "Common.hpp"
#include "TestWriterCommon.hpp"

#include <ASTGenerator/Debug/DebugCommon.hpp>
#include <ASTGenerator/Debug/DebugStmtVisitor.hpp>
#include <ASTGenerator/Var/Variable.hpp>
#include <ShaderWriter/Intrinsics.hpp>
#include <ShaderWriter/Sampler.hpp>
#include <ShaderWriter/Shader.hpp>

namespace
{
	template< typename T >
	void testConstant( sdw::Shader & shader )
	{
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "ConstantValue";
			auto value = shader.declConstant< T >( name, test::getDefault< T >( shader ) );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::ePreprocDefine );
		}
		{
			sdw::Shader shader;
			auto value = shader.declConstant< T >( "value", test::getDefault< T >( shader ), false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "ConstantValue_opt";
			auto value = shader.declConstant< T >( name, test::getDefault< T >( shader ), true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::ePreprocDefine );
		}
	}

	template< typename T >
	void testSpecConstant( sdw::Shader & shader )
	{
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "SpecConstantValue_0";
			auto value = shader.declSpecConstant< T >( name, 0u, test::getDefault< T >( shader ) );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderConstant() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eInOutVariableDecl );
			check( static_cast< sdw::stmt::InOutVariableDecl const & >( stmt ).getLocation() == 0u );
		}
		{
			sdw::Shader shader;
			auto value = shader.declSpecConstant< T >( "value", 1u, test::getDefault< T >( shader ), false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderConstant() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "SpecConstantValue_2_opt";
			auto value = shader.declSpecConstant< T >( name, 2u, test::getDefault< T >( shader ), true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderConstant() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eInOutVariableDecl );
			check( static_cast< sdw::stmt::InOutVariableDecl const & >( stmt ).getLocation() == 2u );
		}
	}

	template< typename T >
	void testShaderInput( sdw::Shader & shader )
	{
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "InputValue_0";
			auto value = shader.declInput< T >( name, 0u );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderInput() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eInOutVariableDecl );
			check( static_cast< sdw::stmt::InOutVariableDecl const & >( stmt ).getLocation() == 0u );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "InputValue_1";
			auto value = shader.declInputArray< T >( name, 1u, 12u );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderInput() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eInOutVariableDecl );
			check( static_cast< sdw::stmt::InOutVariableDecl const & >( stmt ).getLocation() == 1u );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "InputValue_2";
			auto value = shader.declInputArray< T >( name, 2u );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::UnknownArraySize );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderInput() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eInOutVariableDecl );
			check( static_cast< sdw::stmt::InOutVariableDecl const & >( stmt ).getLocation() == 2u );
		}
		{
			sdw::Shader shader;
			auto value = shader.declInput< T >( "value", 0u, false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderInput() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			sdw::Shader shader;
			auto value = shader.declInputArray< T >( "value", 1u, 12u, false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderInput() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			sdw::Shader shader;
			auto value = shader.declInputArray< T >( "value", 2u, false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::UnknownArraySize );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderInput() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "InputValue_0_opt";
			auto value = shader.declInput< T >( name, 0u, true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderInput() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eInOutVariableDecl );
			check( static_cast< sdw::stmt::InOutVariableDecl const & >( stmt ).getLocation() == 0u );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "InputValue_1_opt";
			auto value = shader.declInputArray< T >( name, 1u, 12u, true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderInput() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eInOutVariableDecl );
			check( static_cast< sdw::stmt::InOutVariableDecl const & >( stmt ).getLocation() == 1u );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "InputValue_2_opt";
			auto value = shader.declInputArray< T >( name, 2u, true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::UnknownArraySize );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderInput() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eInOutVariableDecl );
			check( static_cast< sdw::stmt::InOutVariableDecl const & >( stmt ).getLocation() == 2u );
		}
	}

	template< typename T >
	void testShaderOutput( sdw::Shader & shader )
	{
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "OutputValue_0";
			auto value = shader.declOutput< T >( name, 0u );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderOutput() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eInOutVariableDecl );
			check( static_cast< sdw::stmt::InOutVariableDecl const & >( stmt ).getLocation() == 0u );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "OutputValue_1";
			auto value = shader.declOutputArray< T >( name, 1u, 12u );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderOutput() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eInOutVariableDecl );
			check( static_cast< sdw::stmt::InOutVariableDecl const & >( stmt ).getLocation() == 1u );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "OutputValue_2";
			auto value = shader.declOutputArray< T >( name, 2u );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::UnknownArraySize );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderOutput() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eInOutVariableDecl );
			check( static_cast< sdw::stmt::InOutVariableDecl const & >( stmt ).getLocation() == 2u );
		}
		{
			sdw::Shader shader;
			auto value = shader.declOutput< T >( "value", 0u, false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderOutput() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			sdw::Shader shader;
			auto value = shader.declOutputArray< T >( "value", 1u, 12u, false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderOutput() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			sdw::Shader shader;
			auto value = shader.declOutputArray< T >( "value", 2u, false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::UnknownArraySize );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderOutput() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "OutputValue_0_opt";
			auto value = shader.declOutput< T >( name, 0u, true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderOutput() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eInOutVariableDecl );
			check( static_cast< sdw::stmt::InOutVariableDecl const & >( stmt ).getLocation() == 0u );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "OutputValue_1_opt";
			auto value = shader.declOutputArray< T >( name, 1u, 12u, true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderOutput() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eInOutVariableDecl );
			check( static_cast< sdw::stmt::InOutVariableDecl const & >( stmt ).getLocation() == 1u );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "OutputValue_2_opt";
			auto value = shader.declOutputArray< T >( name, 2u, true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::UnknownArraySize );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isShaderOutput() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eInOutVariableDecl );
			check( static_cast< sdw::stmt::InOutVariableDecl const & >( stmt ).getLocation() == 2u );
		}
	}

	template< typename T >
	void testLocale( sdw::Shader & shader )
	{
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "LocaleValue";
			auto value = shader.declLocale< T >( name );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isLocale() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eVariableDecl );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "LocaleValueAssigned";
			auto value = shader.declLocale< T >( name, test::getDefault< T >( shader ) );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eInit );
			check( static_cast< sdw::expr::Init const & >( *value.getExpr() ).getIdentifier()->getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Init const & >( *value.getExpr() ).getIdentifier()->getVariable()->isLocale() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eSimple );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "LocaleValue";
			auto value = shader.declLocaleArray< T >( name, 12u );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isLocale() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eVariableDecl );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "LocaleValue";
			auto value = shader.declLocaleArray< T >( name, 3u, test::getDefaultArray< T >( shader, 3u ) );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 3u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eAggrInit );
			check( static_cast< sdw::expr::AggrInit const & >( *value.getExpr() ).getIdentifier()->getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::AggrInit const & >( *value.getExpr() ).getIdentifier()->getVariable()->isLocale() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eSimple );
		}
		{
			sdw::Shader shader;
			auto value = shader.declLocale< T >( "value", false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isLocale() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			sdw::Shader shader;
			auto value = shader.declLocale< T >( "value", sdw::Optional< T >{ test::getDefault< T >( shader ), false } );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eInit );
			check( static_cast< sdw::expr::Init const & >( *value.getExpr() ).getIdentifier()->getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Init const & >( *value.getExpr() ).getIdentifier()->getVariable()->isLocale() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			sdw::Shader shader;
			auto value = shader.declLocale< T >( "value", test::getDefault< T >( shader ), false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eInit );
			check( static_cast< sdw::expr::Init const & >( *value.getExpr() ).getIdentifier()->getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Init const & >( *value.getExpr() ).getIdentifier()->getVariable()->isLocale() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			sdw::Shader shader;
			auto value = shader.declLocaleArray< T >( "value", 12u, false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isLocale() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			sdw::Shader shader;
			auto value = shader.declLocaleArray< T >( "value", 3u, test::getDefaultArray< T >( shader, 3u ), false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 3u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eAggrInit );
			check( static_cast< sdw::expr::AggrInit const & >( *value.getExpr() ).getIdentifier()->getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::AggrInit const & >( *value.getExpr() ).getIdentifier()->getVariable()->isLocale() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "LocaleValue_opt";
			auto value = shader.declLocale< T >( name, true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isLocale() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eVariableDecl );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "LocaleValueAssigned_opt";
			auto value = shader.declLocale< T >( name, sdw::Optional< T >{ test::getDefault< T >( shader ), true } );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eInit );
			check( static_cast< sdw::expr::Init const & >( *value.getExpr() ).getIdentifier()->getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Init const & >( *value.getExpr() ).getIdentifier()->getVariable()->isLocale() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eSimple );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "LocaleValueAssigned_opt";
			auto value = shader.declLocale< T >( name, test::getDefault< T >( shader ), true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eInit );
			check( static_cast< sdw::expr::Init const & >( *value.getExpr() ).getIdentifier()->getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Init const & >( *value.getExpr() ).getIdentifier()->getVariable()->isLocale() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eSimple );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "LocaleValue_opt";
			auto value = shader.declLocaleArray< T >( name, 12u, true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isLocale() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eVariableDecl );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "LocaleValue_opt";
			auto value = shader.declLocaleArray< T >( name, 3u, test::getDefaultArray< T >( shader, 3u ), true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 3u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eAggrInit );
			check( static_cast< sdw::expr::AggrInit const & >( *value.getExpr() ).getIdentifier()->getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::AggrInit const & >( *value.getExpr() ).getIdentifier()->getVariable()->isLocale() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eSimple );
		}
	}

	template< typename T >
	void testBuiltin( sdw::Shader & shader )
	{
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "BuiltinValue";
			auto value = shader.declBuiltin< T >( name );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isBuiltin() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "BuiltinValue";
			auto value = shader.declBuiltinArray< T >( name, 12u );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isBuiltin() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "BuiltinValue";
			auto value = shader.declBuiltinArray< T >( name );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::UnknownArraySize );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isBuiltin() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			auto value = shader.declBuiltin< T >( "value", false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isBuiltin() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			auto value = shader.declBuiltinArray< T >( "value", 12u, false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isBuiltin() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			auto value = shader.declBuiltinArray< T >( "value", false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::UnknownArraySize );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isBuiltin() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "BuiltinValue_opt";
			auto value = shader.declBuiltin< T >( name, true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isBuiltin() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "BuiltinValue_opt";
			auto value = shader.declBuiltinArray< T >( name, 12u, true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isBuiltin() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::TypeTraits< T >::TypeEnum ) + "BuiltinValue_opt";
			auto value = shader.declBuiltinArray< T >( name, true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::TypeTraits< T >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::UnknownArraySize );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isBuiltin() );
			check( shader.getStatements()->getStatements().empty() );
		}
	}

	template< sdw::SamplerType ST >
	void testSampler( sdw::Shader & shader )
	{
		{
			auto name = sdw::debug::getTypeName( sdw::SamplerTypeTraits< ST >::TypeEnum ) + "SamplerValue_1_1";
			auto value = shader.declSampler< ST >( name, 1u, 1u );
			check( value.getType()->getKind() == sdw::SamplerTypeTraits< ST >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isSampler() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eSamplerDecl );
			check( static_cast< sdw::stmt::SamplerDecl const & >( stmt ).getBindingPoint() == 1u );
			check( static_cast< sdw::stmt::SamplerDecl const & >( stmt ).getBindingSet() == 1u );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::SamplerTypeTraits< ST >::TypeEnum ) + "SamplerValue_2_2";
			auto value = shader.declSamplerArray< ST >( name, 2u, 2u, 12u );
			check( value.getType()->getKind() == sdw::SamplerTypeTraits< ST >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isSampler() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eSamplerDecl );
			check( static_cast< sdw::stmt::SamplerDecl const & >( stmt ).getBindingPoint() == 2u );
			check( static_cast< sdw::stmt::SamplerDecl const & >( stmt ).getBindingSet() == 2u );
		}
		{
			sdw::Shader shader;
			auto value = shader.declSampler< ST >( "value", 1u, 1u, false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::SamplerTypeTraits< ST >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isSampler() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			sdw::Shader shader;
			auto value = shader.declSamplerArray< ST >( "value", 1u, 1u, 12u, false );
			check( !value.isEnabled() );
			check( value.getType()->getKind() == sdw::SamplerTypeTraits< ST >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == "value" );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isSampler() );
			check( shader.getStatements()->getStatements().empty() );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::SamplerTypeTraits< ST >::TypeEnum ) + "SamplerValue_1_1_opt";
			auto value = shader.declSampler< ST >( name, 1u, 1u, true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::SamplerTypeTraits< ST >::TypeEnum );
			check( value.getType()->getArraySize() == sdw::type::NotArray );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isSampler() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eSamplerDecl );
			check( static_cast< sdw::stmt::SamplerDecl const & >( stmt ).getBindingPoint() == 1u );
			check( static_cast< sdw::stmt::SamplerDecl const & >( stmt ).getBindingSet() == 1u );
		}
		{
			auto name = sdw::debug::getTypeName( sdw::SamplerTypeTraits< ST >::TypeEnum ) + "SamplerValue_2_2_opt";
			auto value = shader.declSamplerArray< ST >( name, 2u, 2u, 12u, true );
			check( value.isEnabled() );
			check( value.getType()->getKind() == sdw::SamplerTypeTraits< ST >::TypeEnum );
			check( value.getType()->getArraySize() == 12u );
			check( value.getExpr()->getKind() == sdw::expr::Kind::eIdentifier );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->getTypeName() == name );
			check( static_cast< sdw::expr::Identifier const & >( *value.getExpr() ).getVariable()->isSampler() );
			auto & stmt = *shader.getStatements()->getStatements().back();
			check( stmt.getKind() == sdw::stmt::Kind::eSamplerDecl );
			check( static_cast< sdw::stmt::SamplerDecl const & >( stmt ).getBindingPoint() == 2u );
			check( static_cast< sdw::stmt::SamplerDecl const & >( stmt ).getBindingSet() == 2u );
		}
	}

	void testConstants()
	{
		testBegin( "testConstants" );
		sdw::Shader shader;
		testConstant< sdw::Boolean >( shader );
		testConstant< sdw::Int >( shader );
		testConstant< sdw::UInt >( shader );
		testConstant< sdw::Float >( shader );
		testConstant< sdw::Vec2 >( shader );
		testConstant< sdw::Vec3 >( shader );
		testConstant< sdw::Vec4 >( shader );
		testConstant< sdw::BVec2 >( shader );
		testConstant< sdw::BVec3 >( shader );
		testConstant< sdw::BVec4 >( shader );
		testConstant< sdw::IVec2 >( shader );
		testConstant< sdw::IVec3 >( shader );
		testConstant< sdw::IVec4 >( shader );
		testConstant< sdw::UVec2 >( shader );
		testConstant< sdw::UVec3 >( shader );
		testConstant< sdw::UVec4 >( shader );
		testConstant< sdw::Mat2 >( shader );
		testConstant< sdw::Mat3 >( shader );
		testConstant< sdw::Mat4 >( shader );
		testConstant< sdw::BMat2 >( shader );
		testConstant< sdw::BMat3 >( shader );
		testConstant< sdw::BMat4 >( shader );
		testConstant< sdw::IMat2 >( shader );
		testConstant< sdw::IMat3 >( shader );
		testConstant< sdw::IMat4 >( shader );
		testConstant< sdw::UMat2 >( shader );
		testConstant< sdw::UMat3 >( shader );
		testConstant< sdw::UMat4 >( shader );
		std::cout << sdw::debug::StmtVisitor::submit( shader.getStatements() ) << std::endl;
		testEnd();
	}

	void testSpecConstants()
	{
		testBegin( "testSpecConstants" );
		sdw::Shader shader;
		testSpecConstant< sdw::Boolean >( shader );
		testSpecConstant< sdw::Int >( shader );
		testSpecConstant< sdw::UInt >( shader );
		testSpecConstant< sdw::Float >( shader );
		testSpecConstant< sdw::Vec2 >( shader );
		testSpecConstant< sdw::Vec3 >( shader );
		testSpecConstant< sdw::Vec4 >( shader );
		testSpecConstant< sdw::BVec2 >( shader );
		testSpecConstant< sdw::BVec3 >( shader );
		testSpecConstant< sdw::BVec4 >( shader );
		testSpecConstant< sdw::IVec2 >( shader );
		testSpecConstant< sdw::IVec3 >( shader );
		testSpecConstant< sdw::IVec4 >( shader );
		testSpecConstant< sdw::UVec2 >( shader );
		testSpecConstant< sdw::UVec3 >( shader );
		testSpecConstant< sdw::UVec4 >( shader );
		testSpecConstant< sdw::Mat2 >( shader );
		testSpecConstant< sdw::Mat3 >( shader );
		testSpecConstant< sdw::Mat4 >( shader );
		testSpecConstant< sdw::BMat2 >( shader );
		testSpecConstant< sdw::BMat3 >( shader );
		testSpecConstant< sdw::BMat4 >( shader );
		testSpecConstant< sdw::IMat2 >( shader );
		testSpecConstant< sdw::IMat3 >( shader );
		testSpecConstant< sdw::IMat4 >( shader );
		testSpecConstant< sdw::UMat2 >( shader );
		testSpecConstant< sdw::UMat3 >( shader );
		testSpecConstant< sdw::UMat4 >( shader );
		std::cout << sdw::debug::StmtVisitor::submit( shader.getStatements() ) << std::endl;
		testEnd();
	}

	void testShaderInputs()
	{
		testBegin( "testShaderInputs" );
		sdw::Shader shader;
		testShaderInput< sdw::Boolean >( shader );
		testShaderInput< sdw::Int >( shader );
		testShaderInput< sdw::UInt >( shader );
		testShaderInput< sdw::Float >( shader );
		testShaderInput< sdw::Vec2 >( shader );
		testShaderInput< sdw::Vec3 >( shader );
		testShaderInput< sdw::Vec4 >( shader );
		testShaderInput< sdw::BVec2 >( shader );
		testShaderInput< sdw::BVec3 >( shader );
		testShaderInput< sdw::BVec4 >( shader );
		testShaderInput< sdw::IVec2 >( shader );
		testShaderInput< sdw::IVec3 >( shader );
		testShaderInput< sdw::IVec4 >( shader );
		testShaderInput< sdw::UVec2 >( shader );
		testShaderInput< sdw::UVec3 >( shader );
		testShaderInput< sdw::UVec4 >( shader );
		testShaderInput< sdw::Mat2 >( shader );
		testShaderInput< sdw::Mat3 >( shader );
		testShaderInput< sdw::Mat4 >( shader );
		testShaderInput< sdw::BMat2 >( shader );
		testShaderInput< sdw::BMat3 >( shader );
		testShaderInput< sdw::BMat4 >( shader );
		testShaderInput< sdw::IMat2 >( shader );
		testShaderInput< sdw::IMat3 >( shader );
		testShaderInput< sdw::IMat4 >( shader );
		testShaderInput< sdw::UMat2 >( shader );
		testShaderInput< sdw::UMat3 >( shader );
		testShaderInput< sdw::UMat4 >( shader );
		std::cout << sdw::debug::StmtVisitor::submit( shader.getStatements() ) << std::endl;
		testEnd();
	}

	void testShaderOutputs()
	{
		testBegin( "testShaderOutputs" );
		sdw::Shader shader;
		testShaderOutput< sdw::Boolean >( shader );
		testShaderOutput< sdw::Int >( shader );
		testShaderOutput< sdw::UInt >( shader );
		testShaderOutput< sdw::Float >( shader );
		testShaderOutput< sdw::Vec2 >( shader );
		testShaderOutput< sdw::Vec3 >( shader );
		testShaderOutput< sdw::Vec4 >( shader );
		testShaderOutput< sdw::BVec2 >( shader );
		testShaderOutput< sdw::BVec3 >( shader );
		testShaderOutput< sdw::BVec4 >( shader );
		testShaderOutput< sdw::IVec2 >( shader );
		testShaderOutput< sdw::IVec3 >( shader );
		testShaderOutput< sdw::IVec4 >( shader );
		testShaderOutput< sdw::UVec2 >( shader );
		testShaderOutput< sdw::UVec3 >( shader );
		testShaderOutput< sdw::UVec4 >( shader );
		testShaderOutput< sdw::Mat2 >( shader );
		testShaderOutput< sdw::Mat3 >( shader );
		testShaderOutput< sdw::Mat4 >( shader );
		testShaderOutput< sdw::BMat2 >( shader );
		testShaderOutput< sdw::BMat3 >( shader );
		testShaderOutput< sdw::BMat4 >( shader );
		testShaderOutput< sdw::IMat2 >( shader );
		testShaderOutput< sdw::IMat3 >( shader );
		testShaderOutput< sdw::IMat4 >( shader );
		testShaderOutput< sdw::UMat2 >( shader );
		testShaderOutput< sdw::UMat3 >( shader );
		testShaderOutput< sdw::UMat4 >( shader );
		std::cout << sdw::debug::StmtVisitor::submit( shader.getStatements() ) << std::endl;
		testEnd();
	}

	void testLocales()
	{
		testBegin( "testLocales" );
		sdw::Shader shader;
		testLocale< sdw::Boolean >( shader );
		testLocale< sdw::Int >( shader );
		testLocale< sdw::UInt >( shader );
		testLocale< sdw::Float >( shader );
		testLocale< sdw::Vec2 >( shader );
		testLocale< sdw::Vec3 >( shader );
		testLocale< sdw::Vec4 >( shader );
		testLocale< sdw::BVec2 >( shader );
		testLocale< sdw::BVec3 >( shader );
		testLocale< sdw::BVec4 >( shader );
		testLocale< sdw::IVec2 >( shader );
		testLocale< sdw::IVec3 >( shader );
		testLocale< sdw::IVec4 >( shader );
		testLocale< sdw::UVec2 >( shader );
		testLocale< sdw::UVec3 >( shader );
		testLocale< sdw::UVec4 >( shader );
		testLocale< sdw::Mat2 >( shader );
		testLocale< sdw::Mat3 >( shader );
		testLocale< sdw::Mat4 >( shader );
		testLocale< sdw::BMat2 >( shader );
		testLocale< sdw::BMat3 >( shader );
		testLocale< sdw::BMat4 >( shader );
		testLocale< sdw::IMat2 >( shader );
		testLocale< sdw::IMat3 >( shader );
		testLocale< sdw::IMat4 >( shader );
		testLocale< sdw::UMat2 >( shader );
		testLocale< sdw::UMat3 >( shader );
		testLocale< sdw::UMat4 >( shader );
		std::cout << sdw::debug::StmtVisitor::submit( shader.getStatements() ) << std::endl;
		testEnd();
	}

	void testBuiltins()
	{
		testBegin( "testBuiltins" );
		sdw::Shader shader;
		testBuiltin< sdw::Boolean >( shader );
		testBuiltin< sdw::Int >( shader );
		testBuiltin< sdw::UInt >( shader );
		testBuiltin< sdw::Float >( shader );
		testBuiltin< sdw::Vec2 >( shader );
		testBuiltin< sdw::Vec3 >( shader );
		testBuiltin< sdw::Vec4 >( shader );
		testBuiltin< sdw::BVec2 >( shader );
		testBuiltin< sdw::BVec3 >( shader );
		testBuiltin< sdw::BVec4 >( shader );
		testBuiltin< sdw::IVec2 >( shader );
		testBuiltin< sdw::IVec3 >( shader );
		testBuiltin< sdw::IVec4 >( shader );
		testBuiltin< sdw::UVec2 >( shader );
		testBuiltin< sdw::UVec3 >( shader );
		testBuiltin< sdw::UVec4 >( shader );
		testBuiltin< sdw::Mat2 >( shader );
		testBuiltin< sdw::Mat3 >( shader );
		testBuiltin< sdw::Mat4 >( shader );
		testBuiltin< sdw::BMat2 >( shader );
		testBuiltin< sdw::BMat3 >( shader );
		testBuiltin< sdw::BMat4 >( shader );
		testBuiltin< sdw::IMat2 >( shader );
		testBuiltin< sdw::IMat3 >( shader );
		testBuiltin< sdw::IMat4 >( shader );
		testBuiltin< sdw::UMat2 >( shader );
		testBuiltin< sdw::UMat3 >( shader );
		testBuiltin< sdw::UMat4 >( shader );
		std::cout << sdw::debug::StmtVisitor::submit( shader.getStatements() ) << std::endl;
		testEnd();
	}

	void testSamplers()
	{
		testBegin( "testSamplers" );
		sdw::Shader shader;
		testSampler< sdw::SamplerType::eBuffer >( shader );
		testSampler< sdw::SamplerType::e1D >( shader );
		testSampler< sdw::SamplerType::e2D >( shader );
		testSampler< sdw::SamplerType::e3D >( shader );
		testSampler< sdw::SamplerType::eCube >( shader );
		testSampler< sdw::SamplerType::e2DRect >( shader );
		testSampler< sdw::SamplerType::e1DArray >( shader );
		testSampler< sdw::SamplerType::e2DArray >( shader );
		testSampler< sdw::SamplerType::eCubeArray >( shader );
		testSampler< sdw::SamplerType::e1DShadow >( shader );
		testSampler< sdw::SamplerType::e2DShadow >( shader );
		testSampler< sdw::SamplerType::eCubeShadow >( shader );
		testSampler< sdw::SamplerType::e2DRectShadow >( shader );
		testSampler< sdw::SamplerType::e1DArrayShadow >( shader );
		testSampler< sdw::SamplerType::e2DArrayShadow >( shader );
		testSampler< sdw::SamplerType::eCubeArrayShadow >( shader );
		std::cout << sdw::debug::StmtVisitor::submit( shader.getStatements() ) << std::endl;
		testEnd();
	}

	void testDeclarationStatements()
	{
		testBegin( "testDeclarations" );
		sdw::Shader shader;
		auto InVec2_0 = shader.declInput< sdw::Vec2 >( "InVec2_0", 0u );
		auto InVec3_1 = shader.declInput< sdw::Vec3 >( "InVec3_1", 1u );
		auto InVec4_2 = shader.declInput< sdw::Vec4 >( "InVec3_2", 2u );
		auto OutVec2_0 = shader.declOutput< sdw::Vec2 >( "OutVec2_0", 0u );
		auto OutVec2_1 = shader.declOutput< sdw::Vec2 >( "OutVec2_1", 1u );
		auto BuiltinVec4 = shader.declBuiltin< sdw::Vec4 >( "BuiltinVec4" );
		auto Sampler_0_0 = shader.declSampler< sdw::SamplerType::e1D >( "Sampler_0_0", 0u, 0u );
		auto Sampler_0_1 = shader.declSampler< sdw::SamplerType::e2D >( "Sampler_0_1", 0u, 1u );
		auto Sampler_1_0 = shader.declSampler< sdw::SamplerType::eCubeArray >( "Sampler_1_0", 1u, 0u );
		InVec2_0.x();
		InVec3_1.y();
		InVec4_2.z();
		InVec2_0[0];
		InVec3_1[0];
		InVec4_2[0];
		InVec2_0 = InVec4_2.xy();
		InVec3_1 = InVec4_2.xyz();
		InVec4_2 = BuiltinVec4;
		InVec2_0 * InVec2_0;
		InVec3_1 * InVec3_1;
		InVec4_2 * InVec4_2;
		std::cout << sdw::debug::StmtVisitor::submit( shader.getStatements() ) << std::endl;
		testEnd();
	}
}

int main( int argc, char ** argv )
{
	testSuiteBegin( "TestWriterDeclarations" );
	testConstants();
	testSpecConstants();
	testShaderInputs();
	testShaderOutputs();
	testLocales();
	testBuiltins();
	testSamplers();
	testSuiteEnd();
}
