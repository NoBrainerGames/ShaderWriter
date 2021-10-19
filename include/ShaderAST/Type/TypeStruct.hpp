/*
See LICENSE file in root folder
*/
#ifndef ___AST_TypeStruct_H___
#define ___AST_TypeStruct_H___
#pragma once

#include "TypeArray.hpp"

#include "ShaderAST/Var/FlagHolder.hpp"

#include <vector>

namespace ast::type
{
	enum class MemoryLayout
	{
		eStd140,
		eStd430,
		eC,
	};

	class Struct
		: public Type
	{
	public:
		static constexpr uint32_t NotFound = ~( 0u );
		static constexpr uint32_t InvalidLocation = ~( 0u );

		struct Member
		{
			Member() = default;
			Member( TypePtr ptype
				, std::string pname
				, uint32_t poffset
				, uint32_t psize
				, uint32_t parrayStride )
				: type{ std::move( ptype ) }
				, name{ std::move( pname ) }
				, offset{ std::move( poffset ) }
				, size{ std::move( psize ) }
				, arrayStride{ std::move( parrayStride ) }
			{
			}

			Member( TypePtr ptype
				, std::string pname
				, uint32_t poffset
				, uint32_t psize
				, uint32_t parrayStride
				, uint32_t plocation )
				: type{ std::move( ptype ) }
				, name{ std::move( pname ) }
				, offset{ std::move( poffset ) }
				, size{ std::move( psize ) }
				, arrayStride{ std::move( parrayStride ) }
				, location{ std::move( plocation ) }
			{
			}

			TypePtr type{};
			std::string name{};
			uint32_t offset{};
			uint32_t size{};
			uint32_t arrayStride{};
			uint32_t location{ InvalidLocation };
		};

	private:
		SDAST_API Struct( TypesCache & cache
			, Struct const & rhs );
		SDAST_API Struct( TypesCache & cache
			, Struct * parent
			, uint32_t index
			, Struct const & copy );
		SDAST_API Struct( TypesCache & cache
			, Struct & parent
			, uint32_t index
			, Struct const & copy );

	protected:
		SDAST_API Struct( TypesCache & cache
			, MemoryLayout layout
			, std::string name
			, var::Flag flag );

	public:
		SDAST_API Member getMember( uint32_t index );
		SDAST_API Member getMember( std::string const & name );
		SDAST_API uint32_t findMember( std::string const & name );
		SDAST_API TypePtr getMemberType( Struct & parent, uint32_t index )const override;

		bool hasMember( std::string const & name )
		{
			return findMember( name ) != NotFound;
		}

		std::string const & getName()const
		{
			return m_name;
		}

		size_t size()const
		{
			return m_members.size();
		}

		bool empty()const
		{
			return m_members.empty();
		}

		auto begin()const
		{
			return m_members.begin();
		}

		auto end()const
		{
			return m_members.end();
		}

		auto front()const
		{
			return m_members.front();
		}

		auto back()const
		{
			return m_members.back();
		}

		MemoryLayout getMemoryLayout()const
		{
			return m_layout;
		}

		uint32_t getFlag()const
		{
			return uint32_t( m_flag );
		}

		bool isShaderInput()const
		{
			return m_flag == var::Flag::eShaderInput;
		}

		bool isShaderOutput()const
		{
			return m_flag == var::Flag::eShaderOutput;
		}

	protected:
		std::pair< uint32_t, uint32_t > doLookupMember( std::string const & name
			, TypePtr type );
		void doAddMember( Member const & member );

	private:
		void doCopyMembers( Struct const & rhs );
		void doUpdateOffsets();

	private:
		std::string m_name;
		std::vector< Member > m_members;
		MemoryLayout m_layout;
		var::Flag m_flag{};
	};

	class BaseStruct
		: public Struct
	{
	public:
		SDAST_API BaseStruct( TypesCache & cache
			, MemoryLayout layout
			, std::string name );

		SDAST_API Member declMember( std::string name
			, Kind kind
			, uint32_t arraySize = NotArray );
		SDAST_API Member declMember( std::string name
			, TypePtr type );
		SDAST_API Member declMember( std::string name
			, ArrayPtr type
			, uint32_t arraySize );
		SDAST_API Member declMember( std::string name
			, ArrayPtr type );
		SDAST_API Member declMember( std::string name
			, BaseStructPtr type
			, uint32_t arraySize );
		SDAST_API Member declMember( std::string name
			, BaseStructPtr type );

	private:
		Member doCreateMember( TypePtr type
			, std::string const & name );
	};

	class IOStruct
		: public Struct
	{
	public:
		SDAST_API IOStruct( TypesCache & cache
			, MemoryLayout layout
			, std::string name
			, var::Flag flag );

		SDAST_API Member declMember( std::string name
			, Kind kind
			, uint32_t arraySize
			, uint32_t location );
		SDAST_API Member declMember( std::string name
			, TypePtr type
			, uint32_t location );
		SDAST_API Member declMember( std::string name
			, ArrayPtr type
			, uint32_t arraySize
			, uint32_t location );
		SDAST_API Member declMember( std::string name
			, ArrayPtr type
			, uint32_t location );

	private:
		Struct::Member doCreateMember( TypePtr type
			, std::string const & name
			, uint32_t location );
	};

	SDAST_API bool isStructType( type::TypePtr type );
	SDAST_API type::StructPtr getStructType( type::TypePtr type );

	SDAST_API size_t getHash( MemoryLayout layout
		, std::string const & name );
	SDAST_API size_t getHash( MemoryLayout layout
		, std::string const & name
		, var::Flag flag );

	SDAST_API bool operator==( Struct const & lhs, Struct const & rhs );

	SDAST_API uint32_t getSize( Type const & type
		, MemoryLayout layout );
	SDAST_API uint32_t getSize( TypePtr type
		, MemoryLayout layout );
	SDAST_API uint32_t getAlignment( Type const & type
		, MemoryLayout layout );
	SDAST_API uint32_t getAlignment( TypePtr type
		, MemoryLayout layout );
	SDAST_API uint32_t getArrayStride( Array const & type
		, MemoryLayout layout );
	SDAST_API uint32_t getArrayStride( ArrayPtr type
		, MemoryLayout layout );
	SDAST_API uint32_t getArrayStride( Type const & type
		, MemoryLayout layout );
	SDAST_API uint32_t getArrayStride( TypePtr type
		, MemoryLayout layout );
}

#endif
