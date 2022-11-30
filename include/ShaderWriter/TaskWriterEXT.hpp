/*
See LICENSE file in root folder
*/
#ifndef ___SDW_TaskWriterEXT_H___
#define ___SDW_TaskWriterEXT_H___

#include "ShaderWriter/Writer.hpp"
#include "ShaderWriter/BaseTypes/TaskPayloadEXT.hpp"
#include "ShaderWriter/BaseTypes/UInt.hpp"
#include "ShaderWriter/CompositeTypes/PerPrimitive.hpp"
#include "ShaderWriter/CompositeTypes/PerVertex.hpp"
#include "ShaderWriter/CompositeTypes/StructInstance.hpp"
#include "ShaderWriter/VecTypes/Vec3.hpp"

namespace sdw
{
	/**
	*name
	*	Task.
	*/
	/**@{*/
	/**
	*	Holds input data for a task shader.
	*/
	struct TaskIn
		: public StructInstance
	{
		SDW_API TaskIn( ShaderWriter & writer
			, uint32_t localSizeX
			, uint32_t localSizeY
			, uint32_t localSizeZ );
		SDW_API TaskIn( ShaderWriter & writer
			, ast::expr::ExprPtr expr
			, bool enabled = true );

		SDW_API static ast::type::StructPtr makeType( ast::type::TypesCache & cache );

		//in uint  gl_DrawID;
		Int32 const drawID;
		//const uvec3 gl_WorkGroupSize;
		UInt32 const workGroupSize;
		//in uvec3 gl_WorkGroupID;
		UInt32 const workGroupID;
		//in uvec3 gl_LocalInvocationID;
		UInt32 const localInvocationID;
		//in uvec3 gl_GlobalInvocationID;
		UInt32 const globalInvocationID;
		//in uint  gl_LocalInvocationIndex;
		UInt32 const localInvocationIndex;
	};
	/**
	*	Holds input data for a task subgroup shader.
	*/
	struct TaskSubgroupIn
		: private TaskIn
	{
		SDW_API TaskSubgroupIn( ShaderWriter & writer
			, uint32_t localSizeX
			, uint32_t localSizeY
			, uint32_t localSizeZ );
		SDW_API TaskSubgroupIn( ShaderWriter & writer
			, ast::expr::ExprPtr expr
			, bool enabled = true );

		SDW_API static ast::type::StructPtr makeType( ast::type::TypesCache & cache );

		using TaskIn::updateContainer;
		using TaskIn::getContainer;
		using TaskIn::updateExpr;
		using TaskIn::getType;
		using TaskIn::getExpr;
		using TaskIn::getWriter;
		using TaskIn::getShader;
		using TaskIn::isEnabled;

		//in uint  gl_DrawID;
		using TaskIn::drawID;
		//in uvec3 gl_WorkGroupSize;
		using TaskIn::workGroupSize;
		//in uvec3 gl_WorkGroupID;
		using TaskIn::workGroupID;
		//in uvec3 gl_LocalInvocationID;
		using TaskIn::localInvocationID;
		//in uvec3 gl_GlobalInvocationID;
		using TaskIn::globalInvocationID;
		//in uint  gl_LocalInvocationIndex;
		using TaskIn::localInvocationIndex;

		//in uint gl_NumSubgroups;
		UInt32 const numSubgroups;
		//in uint gl_SubgroupID;
		UInt32 const subgroupID;
		//in uint gl_SubgroupSize;
		UInt32 const subgroupSize;
		//in uint gl_SubgroupInvocationID;
		UInt32 const subgroupInvocationID;
		//in uvec4 gl_SubgroupEqMask;
		U32Vec4 const subgroupEqMask;
		//const uvec4 gl_SubgroupGeMask;
		U32Vec4 const subgroupGeMask;
		//const uvec4 gl_SubgroupGtMask;
		U32Vec4 const subgroupGtMask;
		//const uvec4 gl_SubgroupLeMask;
		U32Vec4 const subgroupLeMask;
		//const uvec4 gl_SubgroupLtMask;
		U32Vec4 const subgroupLtMask;
	};
	/**
	*	Entry point type
	*/
	/**@{*/
	template< template< ast::var::Flag FlagT > typename PayloadT >
	using TaskEXTMainFuncT = std::function< void( TaskIn
		, TaskPayloadOutEXTT< PayloadT > ) >;
	template< template< ast::var::Flag FlagT > typename PayloadT >
	using TaskEXTSubgroupMainFuncT = std::function< void( TaskSubgroupIn
		, TaskPayloadOutEXTT< PayloadT > ) >;
	using TaskEXTMainFunc = TaskEXTMainFuncT< VoidT >;
	using TaskEXTSubgroupMainFunc = TaskEXTSubgroupMainFuncT< VoidT >;
	/**@}*/

	class TaskWriterEXT
		: public ShaderWriter
	{
	public:
		SDW_API TaskWriterEXT();

		SDW_API void dispatchMesh( UInt numGroupsX
			, UInt numGroupsY
			, UInt numGroupsZ );
		template< template< ast::var::Flag FlagT > typename PayloadT >
		void dispatchMesh( UInt numGroupsX
			, UInt numGroupsY
			, UInt numGroupsZ
			, TaskPayloadOutEXTT< PayloadT > const & payload );

		SDW_API void implementMain( uint32_t localSizeX
			, uint32_t localSizeY
			, uint32_t localSizeZ
			, TaskEXTMainFunc const & function );
		SDW_API void implementMain( uint32_t localSizeX
			, uint32_t localSizeY
			, uint32_t localSizeZ
			, TaskEXTSubgroupMainFunc const & function );

		template< template< ast::var::Flag FlagT > typename PayloadT >
		void implementMainT( uint32_t localSizeX
			, uint32_t localSizeY
			, uint32_t localSizeZ
			, TaskPayloadOutEXTT< PayloadT > payload
			, TaskEXTMainFuncT< PayloadT > const & function );
		template< template< ast::var::Flag FlagT > typename PayloadT >
		void implementMainT( uint32_t localSizeX
			, uint32_t localSizeY
			, uint32_t localSizeZ
			, TaskPayloadOutEXTT< PayloadT > payload
			, TaskEXTSubgroupMainFuncT< PayloadT > const & function );
	};
	/**@}*/
}

#include "TaskWriterEXT.inl"

#endif