/*
See LICENSE file in root folder
*/
#include "CompilerSpirV/SpirvModuleTypes.hpp"

#include "CompilerSpirV/SpirvHelpers.hpp"
#include "CompilerSpirV/SpirvModule.hpp"

#include <ShaderAST/Type/TypeCache.hpp>

namespace spirv
{
	//*************************************************************************

	namespace modtyp
	{
		static ast::type::TypePtr getUnqualifiedType( ast::type::TypesCache & typesCache
			, ast::type::TypePtr qualified );

		static ast::type::StructPtr getUnqualifiedType( ast::type::TypesCache & typesCache
			, ast::type::Struct const & qualified )
		{
			auto result = typesCache.getStruct( qualified.getMemoryLayout(), qualified.getName() );
			assert( result->empty() || ( result->size() == qualified.size() ) );

			if ( result->empty() && !qualified.empty() )
			{
				for ( auto & member : qualified )
				{
					auto type = getUnqualifiedType( typesCache, member.type );

					if ( type->getKind() == ast::type::Kind::eArray )
					{
						result->declMember( member.name
							, std::static_pointer_cast< ast::type::Array >( type ) );
					}
					else if ( type->getKind() == ast::type::Kind::eStruct
						|| type->getKind() == ast::type::Kind::eRayDesc )
					{
						result->declMember( member.name
							, std::static_pointer_cast< ast::type::Struct >( type ) );
					}
					else
					{
						result->declMember( member.name
							, type );
					}
				}
			}

			return result;
		}

		static ast::type::ArrayPtr getUnqualifiedType( ast::type::TypesCache & typesCache
			, ast::type::Array const & qualified )
		{
			return typesCache.getArray( getUnqualifiedType( typesCache, qualified.getType() ), qualified.getArraySize() );
		}

		static ast::type::SamplerPtr getUnqualifiedType( ast::type::TypesCache & typesCache
			, ast::type::Sampler const & qualified )
		{
			return typesCache.getSampler( qualified.isComparison() );
		}

		static ast::type::CombinedImagePtr getUnqualifiedType( ast::type::TypesCache & typesCache
			, ast::type::CombinedImage const & qualified )
		{
			auto config = qualified.getConfig();
			// Ignore access kind, since it's not handled in non Kernel programs.
			// Prevents generating duplicate types in SPIRV programs.
			config.accessKind = ast::type::AccessKind::eReadWrite;
			return typesCache.getCombinedImage( config, qualified.isComparison() );
		}

		static ast::type::SampledImagePtr getUnqualifiedType( ast::type::TypesCache & typesCache
			, ast::type::SampledImage const & qualified )
		{
			auto config = qualified.getConfig();
			// Ignore access kind, since it's not handled in non Kernel programs.
			// Prevents generating duplicate types in SPIRV programs.
			config.accessKind = ast::type::AccessKind::eReadWrite;
			return typesCache.getSampledImage( config, qualified.getDepth() );
		}

		static ast::type::ImagePtr getUnqualifiedType( ast::type::TypesCache & typesCache
			, ast::type::Image const & qualified )
		{
			auto config = qualified.getConfig();
			// Ignore access kind, since it's not handled in non Kernel programs.
			// Prevents generating duplicate types in SPIRV programs.
			config.accessKind = ast::type::AccessKind::eReadWrite;
			return typesCache.getImage( config );
		}

		static ast::type::TypePtr getUnqualifiedType( ast::type::TypesCache & typesCache
			, ast::type::Type const & qualified )
		{
			ast::type::TypePtr result;

			if ( qualified.getRawKind() == ast::type::Kind::eArray )
			{
				result = getUnqualifiedType( typesCache, static_cast< ast::type::Array const & >( qualified ) );
			}
			else if ( qualified.getRawKind() == ast::type::Kind::eStruct
				|| qualified.getRawKind() == ast::type::Kind::eRayDesc )
			{
				result = getUnqualifiedType( typesCache, static_cast< ast::type::Struct const & >( qualified ) );
			}
			else if ( qualified.getRawKind() == ast::type::Kind::eImage )
			{
				result = getUnqualifiedType( typesCache, static_cast< ast::type::Image const & >( qualified ) );
			}
			else if ( qualified.getRawKind() == ast::type::Kind::eCombinedImage )
			{
				result = getUnqualifiedType( typesCache, static_cast< ast::type::CombinedImage const & >( qualified ) );
			}
			else if ( qualified.getRawKind() == ast::type::Kind::eSampledImage )
			{
				result = getUnqualifiedType( typesCache, static_cast< ast::type::SampledImage const & >( qualified ) );
			}
			else if ( qualified.getRawKind() == ast::type::Kind::eSampler )
			{
				result = getUnqualifiedType( typesCache, static_cast< ast::type::Sampler const & >( qualified ) );
			}
			else if ( qualified.isMember() )
			{
				result = typesCache.getBasicType( qualified.getKind() );
			}

			return result;
		}

		static ast::type::TypePtr getUnqualifiedType( ast::type::TypesCache & typesCache
			, ast::type::TypePtr qualified )
		{
			ast::type::TypePtr result = getUnqualifiedType( typesCache, *qualified );
			return result
				? result
				: qualified;
		}

		static void writeArrayStride( Module & module
			, ast::type::TypePtr elementType
			, TypeId typeId
			, uint32_t arrayStride )
		{
			auto kind = getNonArrayKind( elementType );

			if ( kind != ast::type::Kind::eImage
				&& kind != ast::type::Kind::eCombinedImage
				&& kind != ast::type::Kind::eSampler
				&& kind != ast::type::Kind::eSampledImage )
			{
				if ( !arrayStride )
				{
					arrayStride = ast::type::getSize( elementType, ast::type::MemoryLayout::eStd140 );
				}

				module.decorate( typeId
					, makeIdList( module.allocator
						, uint32_t( spv::DecorationArrayStride )
						, arrayStride ) );
			}
		}

		static size_t myHash( ast::type::ImageConfiguration const & config
			, ast::type::Trinary isComparison )noexcept
		{
			size_t result = std::hash< ast::type::ImageDim >{}( config.dimension );
			result = ast::type::hashCombine( result, config.format );
			result = ast::type::hashCombine( result, config.isSampled );
			result = ast::type::hashCombine( result, config.isArrayed );
			result = ast::type::hashCombine( result, config.isMS );
			result = ast::type::hashCombine( result, isComparison );
			return result;
		}
	}

	//*************************************************************************

	ModuleTypes::ModuleTypes( ast::ShaderAllocatorBlock * allocator
		, Module & module
		, ast::type::TypesCache * typesCache
		, InstructionList & constantsTypes )
		: m_allocator{ allocator }
		, m_module{ module }
		, m_debugNames{ m_module.getDebugNames() }
		, m_nonSemanticDebug{ m_module.getNonSemanticDebug() }
		, m_declarations{ constantsTypes }
		, m_typesCache{ typesCache }
		, m_registeredTypes{ allocator }
		, m_registeredMemberTypes{ allocator }
		, m_registeredSamplerImages{ allocator }
		, m_registeredImageTypes{ allocator }
		, m_registeredPointerTypes{ allocator }
		, m_registeredForwardPointerTypes{ allocator }
		, m_registeredFunctionTypes{ allocator }
	{
	}

	TypeId ModuleTypes::registerType( ast::type::TypePtr type
		, glsl::Statement const * debugStatement )
	{
		return doRegisterTypeRec( type
			, ast::type::NotMember
			, TypeId{}
			, 0u
			, debugStatement );
	}

	TypeId ModuleTypes::registerType( ast::type::TypePtr type
		, uint32_t mbrIndex
		, TypeId parentId
		, glsl::Statement const * debugStatement )
	{
		return doRegisterTypeRec( type
			, mbrIndex
			, parentId
			, 0u
			, debugStatement );
	}

	TypeId ModuleTypes::registerImageType( ast::type::ImagePtr image
		, bool isComparison )
	{
		return doRegisterBaseType( image
			, isComparison ? ast::type::Trinary::eTrue : ast::type::Trinary::eFalse );
	}

	TypeId ModuleTypes::registerFunctionType( TypeIdList const & funcTypes )
	{
		auto it = m_registeredFunctionTypes.find( funcTypes );
		TypeId resultId;

		if ( it == m_registeredFunctionTypes.end() )
		{
			resultId.id.id = m_module.getNextId();

			m_declarations.push_back( makeInstruction< FunctionTypeInstruction >( m_module.getNameCache()
				, resultId.id
				, convert( funcTypes ) ) );
			m_nonSemanticDebug.registerFunctionType( funcTypes, resultId );

			it = m_registeredFunctionTypes.emplace( funcTypes, resultId ).first;
		}
		else
		{
			resultId = it->second;
		}

		return resultId;
	}

	TypeId ModuleTypes::registerPointerType( TypeId type
		, spv::StorageClass storage
		, bool isForward )
	{
		uint64_t key = ( uint64_t( type.id.id ) << 33 )
			| ( ( uint64_t( type.isPointer() ) << 32 ) & 0x01 )
			| ( uint64_t( storage ) << 1 )
			| ( isForward & 0x01 );
		auto it = m_registeredPointerTypes.find( key );

		if ( it == m_registeredPointerTypes.end() )
		{
			TypeId id{ m_module.getNextId()
				, getTypesCache().getPointerType( type->type, convert( storage ) ) };
			id.debug = type.debug;
			it = m_registeredPointerTypes.emplace( key, id ).first;

			if ( isForward )
			{
				m_declarations.push_back( makeInstruction< ForwardPointerTypeInstruction >( m_module.getNameCache()
					, id.id
					, ValueId{ spv::Id( storage ) } ) );
				m_declarations.push_back( makeInstruction< PointerTypeInstruction >( m_module.getNameCache()
					, id.id
					, ValueId{ spv::Id( storage ) }
					, type.id ) );
				key = ( uint64_t( type.id.id ) << 33 )
					| ( ( uint64_t( type.isPointer() ) << 32 ) & 0x01 )
					| ( uint64_t( storage ) << 1 );
				m_registeredPointerTypes.emplace( key, id );
			}
			else
			{
				m_declarations.push_back( makeInstruction< PointerTypeInstruction >( m_module.getNameCache()
					, id.id
					, ValueId{ spv::Id( storage ) }
					, type.id ) );
			}
		}

		return it->second;
	}

	DebugId ModuleTypes::mergeSamplerImage( DebugId const & image
		, DebugId const & sampler
		, Block & currentBlock )
	{
		auto & imgType = static_cast< ast::type::Image const & >( *getNonArrayType( image->type ) );
		auto & splType = static_cast< ast::type::Sampler const & >( *getNonArrayType( sampler->type ) );
		auto lhsIt = m_registeredSamplerImages.emplace( image
			, UnorderedMap< DebugId, DebugId, DebugIdHasher >{ m_allocator } ).first;
		auto ires = lhsIt->second.emplace( sampler, DebugId{} );
		auto it = ires.first;

		if ( ires.second )
		{
			auto typeId = registerType( getTypesCache().getCombinedImage( imgType.getConfig()
				, splType.isComparison() )
				, nullptr );
			it->second = DebugId{ m_module.getNextId(), typeId->type };
			currentBlock.instructions.push_back( makeInstruction< SampledImageInstruction >( m_module.getNameCache()
				, typeId.id
				, it->second.id
				, image.id
				, sampler.id ) );
		}

		return it->second;
	}

	TypeId ModuleTypes::doRegisterNonArrayType( ast::type::TypePtr type
		, uint32_t mbrIndex
		, TypeId parentId
		, glsl::Statement const * debugStatement )
	{
		TypeId result;
		auto unqualifiedType = modtyp::getUnqualifiedType( *m_typesCache, type );
		auto it = m_registeredTypes.find( unqualifiedType );

		if ( it == m_registeredTypes.end() )
		{
			result = doRegisterBaseType( unqualifiedType
				, mbrIndex
				, parentId
				, 0u
				, debugStatement );
		}
		else
		{
			result = it->second;
		}

		return result;
	}

	TypeId ModuleTypes::doRegisterTypeRec( ast::type::TypePtr type
		, uint32_t mbrIndex
		, TypeId parentId
		, uint32_t arrayStride
		, glsl::Statement const * debugStatement )
	{
		TypeId result{ 0u, type };

		if ( type->getRawKind() == ast::type::Kind::eArray )
		{
			auto arrayedType = static_cast< ast::type::Array const & >( *type ).getType();
			auto elementTypeId = doRegisterTypeRec( arrayedType
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
			auto unqualifiedType = modtyp::getUnqualifiedType( *m_typesCache, type );
			auto it = m_registeredTypes.find( unqualifiedType );

			if ( it == m_registeredTypes.end() )
			{
				result.id.id = m_module.getNextId();
				auto & resultId = m_registeredTypes.emplace( unqualifiedType, result ).first->second;
				auto arraySize = getArraySize( type );

				if ( arraySize != ast::type::UnknownArraySize )
				{
					auto lengthId = m_module.registerLiteral( arraySize );
					m_declarations.push_back( makeInstruction< ArrayTypeInstruction >( m_module.getNameCache()
						, resultId.id
						, elementTypeId.id
						, lengthId.id ) );
					m_nonSemanticDebug.registerArrayType( elementTypeId, arraySize, resultId );
				}
				else
				{
					m_declarations.push_back( makeInstruction< RuntimeArrayTypeInstruction >( m_module.getNameCache()
						, resultId.id
						, elementTypeId.id ) );
					m_nonSemanticDebug.registerRuntimeArrayType( elementTypeId, resultId );
				}

				modtyp::writeArrayStride( m_module
					, arrayedType
					, resultId
					, arrayStride );
				result = resultId;
			}
			else
			{
				result = it->second;
			}
		}
		else if ( type->getRawKind() == ast::type::Kind::ePointer )
		{
			auto & pointerType = static_cast< ast::type::Pointer const & >( *type );
			auto rawTypeId = doRegisterTypeRec( pointerType.getPointerType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
			auto storageClass = convert( pointerType.getStorage() );
			result = registerPointerType( rawTypeId
				, storageClass
				, pointerType.isForward() );
		}
		else if ( type->getRawKind() == ast::type::Kind::eRayPayload )
		{
			auto & payloadType = static_cast< ast::type::RayPayload const & >( *type );
			result = doRegisterTypeRec( payloadType.getDataType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}
		else if ( type->getRawKind() == ast::type::Kind::eCallableData )
		{
			auto & callableType = static_cast< ast::type::CallableData const & >( *type );
			result = doRegisterTypeRec( callableType.getDataType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}
		else if ( type->getRawKind() == ast::type::Kind::eHitAttribute )
		{
			auto & callableType = static_cast< ast::type::HitAttribute const & >( *type );
			result = doRegisterTypeRec( callableType.getDataType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}
		else if ( type->getRawKind() == ast::type::Kind::eGeometryOutput )
		{
			auto & outputType = static_cast< ast::type::GeometryOutput const & >( *type );
			result = doRegisterTypeRec( outputType.getType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
			m_module.registerExecutionMode( outputType.getLayout(), outputType.getCount() );
		}
		else if ( type->getRawKind() == ast::type::Kind::eGeometryInput )
		{
			auto & inputType = static_cast< ast::type::GeometryInput const & >( *type );
			result = doRegisterTypeRec( inputType.getType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
			m_module.registerExecutionMode( inputType.getLayout() );
		}
		else if ( type->getRawKind() == ast::type::Kind::eTessellationInputPatch )
		{
			auto & outputType = static_cast< ast::type::TessellationInputPatch const & >( *type );
			result = doRegisterTypeRec( outputType.getType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}
		else if ( type->getRawKind() == ast::type::Kind::eTessellationOutputPatch )
		{
			auto & outputType = static_cast< ast::type::TessellationOutputPatch const & >( *type );
			result = doRegisterTypeRec( outputType.getType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}
		else if ( type->getRawKind() == ast::type::Kind::eTessellationControlOutput )
		{
			auto & outputType = static_cast< ast::type::TessellationControlOutput const & >( *type );
			result = doRegisterTypeRec( outputType.getType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
			m_module.registerExecutionMode( outputType.getDomain()
				, outputType.getPartitioning()
				, outputType.getTopology()
				, outputType.getOrder()
				, outputType.getOutputVertices() );
		}
		else if ( type->getRawKind() == ast::type::Kind::eTessellationControlInput )
		{
			auto & inputType = static_cast< ast::type::TessellationControlInput const & >( *type );
			result = doRegisterTypeRec( inputType.getType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}
		else if ( type->getRawKind() == ast::type::Kind::eTessellationEvaluationInput )
		{
			auto & inputType = static_cast< ast::type::TessellationEvaluationInput const & >( *type );
			result = doRegisterTypeRec( inputType.getType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}
		else if ( type->getRawKind() == ast::type::Kind::eMeshVertexOutput )
		{
			auto & outputType = static_cast< ast::type::MeshVertexOutput const & >( *type );
			result = doRegisterTypeRec( outputType.getType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}
		else if ( type->getRawKind() == ast::type::Kind::eMeshPrimitiveOutput )
		{
			auto & outputType = static_cast< ast::type::MeshPrimitiveOutput const & >( *type );
			result = doRegisterTypeRec( outputType.getType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}
		else if ( type->getRawKind() == ast::type::Kind::eTaskPayloadNV )
		{
			auto & outputType = static_cast< ast::type::TaskPayloadNV const & >( *type );
			result = doRegisterTypeRec( outputType.getType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}
		else if ( type->getRawKind() == ast::type::Kind::eTaskPayload )
		{
			auto & outputType = static_cast< ast::type::TaskPayload const & >( *type );
			result = doRegisterTypeRec( outputType.getType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}
		else if ( type->getRawKind() == ast::type::Kind::eTaskPayloadInNV )
		{
			auto & inputType = static_cast< ast::type::TaskPayloadInNV const & >( *type );
			result = doRegisterTypeRec( inputType.getType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}
		else if ( type->getRawKind() == ast::type::Kind::eTaskPayloadIn )
		{
			auto & inputType = static_cast< ast::type::TaskPayloadIn const & >( *type );
			result = doRegisterTypeRec( inputType.getType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}
		else if ( type->getRawKind() == ast::type::Kind::eComputeInput )
		{
			auto & inputType = static_cast< ast::type::ComputeInput const & >( *type );
			result = doRegisterTypeRec( inputType.getType()
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}
		else
		{
			result = doRegisterNonArrayType( type
				, mbrIndex
				, parentId
				, debugStatement );
		}

		return result;
	}

	TypeId ModuleTypes::doRegisterBaseType( ast::type::Kind kind
		, uint32_t mbrIndex
		, TypeId parentId
		, uint32_t arrayStride
		, glsl::Statement const * debugStatement )
	{
		assert( kind != ast::type::Kind::eStruct );
		assert( kind != ast::type::Kind::eRayDesc );
		assert( kind != ast::type::Kind::eSampler );
		assert( kind != ast::type::Kind::eImage );
		assert( kind != ast::type::Kind::eSampledImage );
		assert( kind != ast::type::Kind::eCombinedImage );

		auto type = m_typesCache->getBasicType( kind );
		TypeId result{ 0u, type };
		result.id.id = m_module.getNextId();
		auto & resultId = m_registeredTypes.emplace( type, result ).first->second;

		if ( isVectorType( kind )
			|| isMatrixType( kind ) )
		{
			auto componentType = registerType( m_typesCache->getBasicType( getComponentType( kind ) ), debugStatement );
			auto componentCount = getComponentCount( kind );

			if ( isMatrixType( kind ) )
			{
				m_declarations.push_back( makeInstruction< MatrixTypeInstruction >( m_module.getNameCache()
					, resultId.id
					, componentType.id
					, ValueId{ componentCount } ) );
				m_nonSemanticDebug.registerMatrixType( componentType, componentCount, resultId );
			}
			else
			{
				m_declarations.push_back( makeInstruction< VectorTypeInstruction >( m_module.getNameCache()
					, resultId.id
					, componentType.id
					, ValueId{ componentCount } ) );
				m_nonSemanticDebug.registerVectorType( componentType, componentCount, resultId );
			}
		}
		else
		{
			m_declarations.push_back( makeBaseTypeInstruction( m_module.getNameCache()
				, kind
				, resultId.id ) );
			m_nonSemanticDebug.registerBaseType( kind, resultId );
		}

		return resultId;
	}

	TypeId ModuleTypes::doRegisterBaseType( ast::type::SamplerPtr type
		, uint32_t mbrIndex
		, TypeId parentId )
	{
		TypeId result{ 0u, type };
		result.id.id = m_module.getNextId();
		m_declarations.push_back( makeInstruction< SamplerTypeInstruction >( m_module.getNameCache()
			, result.id ) );
		auto & resultId = m_registeredTypes.emplace( type, result ).first->second;
		m_nonSemanticDebug.registerSamplerType( std::move( type ), resultId );
		return resultId;
	}

	TypeId ModuleTypes::doRegisterBaseType( ast::type::CombinedImagePtr type
		, uint32_t mbrIndex
		, TypeId parentId )
	{
		auto imgTypeId = doRegisterBaseType( type->getImageType()
			, type->isComparison() ? ast::type::Trinary::eTrue : ast::type::Trinary::eFalse );
		TypeId result{ 0u, type };
		result.id.id = m_module.getNextId();
		m_declarations.push_back( makeInstruction< TextureTypeInstruction >( m_module.getNameCache()
			, result.id
			, imgTypeId.id ) );
		auto & resultId = m_registeredTypes.emplace( type, result ).first->second;
		m_nonSemanticDebug.registerCombinedImageType( std::move( type ), resultId );
		return resultId;
	}

	TypeId ModuleTypes::doRegisterBaseType( ast::type::ImagePtr type
		, ast::type::Trinary isComparison )
	{
		auto ires = m_registeredImageTypes.emplace( modtyp::myHash( type->getConfig(), isComparison ), TypeId{} );
		auto it = ires.first;

		if ( ires.second )
		{
			// The Sampled Type.
			auto sampledTypeId = registerType( m_typesCache->getBasicType( type->getConfig().sampledType ), nullptr );
			// The Image Type.
			it->second = TypeId{ m_module.getNextId(), type };
			m_declarations.push_back( makeImageTypeInstruction( m_module.getNameCache()
				, type->getConfig()
				, isComparison
				, it->second.id
				, sampledTypeId.id ) );
			auto & resultId = m_registeredTypes.emplace( type, it->second ).first->second;
			m_nonSemanticDebug.registerImageType( std::move( type ), resultId );
			it->second = resultId;
		}

		return it->second;
	}

	TypeId ModuleTypes::doRegisterBaseType( ast::type::ImagePtr type
		, uint32_t mbrIndex
		, TypeId parent )
	{
		return doRegisterBaseType( type, ast::type::Trinary::eFalse );
	}

	TypeId ModuleTypes::doRegisterBaseType( ast::type::SampledImagePtr type
		, uint32_t mbrIndex
		, TypeId parent )
	{
		return doRegisterBaseType( type->getImageType(), type->getDepth() );
	}

	TypeId ModuleTypes::doRegisterBaseType( ast::type::AccelerationStructurePtr type
		, uint32_t mbrIndex
		, TypeId parentId
		, glsl::Statement const * debugStatement )
	{
		TypeId result{ 0u, type };
		result.id.id = m_module.getNextId();
		m_declarations.push_back( makeAccelerationStructureTypeInstruction( m_module.getNameCache()
			, result.id ) );
		auto & resultId = m_registeredTypes.emplace( type, result ).first->second;
		m_nonSemanticDebug.registerAccelerationStructureType( std::move( type ), resultId );
		return resultId;
	}

	TypeId ModuleTypes::doRegisterBaseType( ast::type::StructPtr type
		, uint32_t
		, TypeId
		, glsl::Statement const * debugStatement )
	{
		TypeId result{ 0u, type };
		result.id.id = m_module.getNextId();
		TypeIdList subTypes{ m_allocator };
		ValueIdList debugSubTypes{ m_allocator };

		for ( auto & member : *type )
		{
			auto subTypeId = doRegisterTypeRec( member.type
				, member.type->getIndex()
				, result
				, member.arrayStride
				, debugStatement );
			subTypes.push_back( subTypeId );
			m_nonSemanticDebug.registerMemberType( member
				, subTypeId
				, debugStatement
				, debugSubTypes );
		}

		m_declarations.push_back( makeInstruction< StructTypeInstruction >( m_module.getNameCache()
			, result.id
			, convert( subTypes ) ) );
		m_debugNames.registerName( result, type->getName() );
		bool hasBuiltin = false;
		bool hasDynarray = false;

		for ( auto & member : *type )
		{
			auto index = member.type->getIndex();
			m_debugNames.registerMemberName( result, index, member.name );

			if ( member.builtin == ast::Builtin::eNone )
			{
				m_module.decorateMember( result
					, index
					, makeIdList( m_allocator, uint32_t( spv::DecorationOffset ), member.offset ) );
			}
			else
			{
				doAddMbrBuiltin( member.builtin, result, index );
				hasBuiltin = true;
			}

			auto kind = getNonArrayKind( member.type );
			auto arraySize = getArraySize( member.type );

			if ( arraySize == ast::type::UnknownArraySize )
			{
				hasDynarray = true;
			}

			if ( isMatrixType( kind ) )
			{
				auto colKind = getComponentType( kind );
				auto colType = m_typesCache->getBasicType( colKind );
				auto rowCount = getComponentCount( colType );

				if ( rowCount == 3
					|| ( rowCount == 2 && type->getMemoryLayout() == ast::type::MemoryLayout::eStd140 ) )
				{
					colType = m_typesCache->getVector( getComponentType( colKind ), 4u );
				}

				auto size = getSize( *colType
					, type->getMemoryLayout() );
				m_module.decorateMember( result
					, index
					, spv::DecorationColMajor );
				m_module.decorateMember( result
					, index
					, makeIdList( m_allocator, uint32_t( spv::DecorationMatrixStride ), size ) );
			}
		}

		if ( hasBuiltin || hasDynarray )
		{
			m_module.decorate( result, spv::DecorationBlock );
		}

		auto & resultId = m_registeredTypes.emplace( type, result ).first->second;
		m_nonSemanticDebug.registerStructType( std::move( type )
			, debugSubTypes
			, debugStatement
			, resultId );
		return resultId;
	}

	TypeId ModuleTypes::doRegisterBaseType( ast::type::TypePtr type
		, uint32_t mbrIndex
		, TypeId parentId
		, uint32_t arrayStride
		, glsl::Statement const * debugStatement )
	{
		TypeId result{ 0u, type };

		if ( type->getRawKind() == ast::type::Kind::eArray )
		{
			type = std::static_pointer_cast< ast::type::Array >( type )->getType();
		}

		auto kind = type->getRawKind();

		if ( kind == ast::type::Kind::eSampler )
		{
			result = doRegisterBaseType( std::static_pointer_cast< ast::type::Sampler >( type )
				, mbrIndex
				, parentId );
		}
		else if ( kind == ast::type::Kind::eCombinedImage )
		{
			result = doRegisterBaseType( std::static_pointer_cast< ast::type::CombinedImage >( type )
				, mbrIndex
				, parentId );
		}
		else if ( kind == ast::type::Kind::eImage )
		{
			result = doRegisterBaseType( std::static_pointer_cast< ast::type::Image >( type )
				, mbrIndex
				, parentId );
		}
		else if ( kind == ast::type::Kind::eSampledImage )
		{
			result = doRegisterBaseType( std::static_pointer_cast< ast::type::SampledImage >( type )
				, mbrIndex
				, parentId );
		}
		else if ( kind == ast::type::Kind::eAccelerationStructure )
		{
			result = doRegisterBaseType( std::static_pointer_cast< ast::type::AccelerationStructure >( type )
				, mbrIndex
				, parentId
				, debugStatement );
		}
		else if ( kind == ast::type::Kind::eStruct
			|| kind == ast::type::Kind::eRayDesc )
		{
			result = doRegisterBaseType( std::static_pointer_cast< ast::type::Struct >( type )
				, mbrIndex
				, parentId
				, debugStatement );
		}
		else
		{
			result = doRegisterBaseType( kind
				, mbrIndex
				, parentId
				, arrayStride
				, debugStatement );
		}

		return result;
	}

	bool ModuleTypes::doAddMbrBuiltin( ast::Builtin pbuiltin
		, DebugId outer
		, uint32_t mbrIndex )
	{
		bool result = false;
		Vector< spv::Decoration > additionalDecorations{ m_allocator };
		auto builtin = getBuiltin( pbuiltin, m_module.getExecutionModel(), additionalDecorations );

		if ( builtin != spv::BuiltInMax )
		{
			m_module.decorateMember( outer, mbrIndex, makeIdList( m_allocator, spv::Id( spv::DecorationBuiltIn ), spv::Id( builtin ) ) );

			for ( auto & decoration : additionalDecorations )
			{
				m_module.decorateMember( outer, mbrIndex, decoration );
			}

			result = true;
		}

		return result;
	}

	//*************************************************************************
}