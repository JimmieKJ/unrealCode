#
# Build {{TARGETNAME_CC}}
#

SET(GW_DEPS_ROOT $ENV{GW_DEPS_ROOT})
FIND_PACKAGE(PxShared REQUIRED)

SET(APEX_MODULE_DIR ${PROJECT_SOURCE_DIR}/../../../module)

SET(AM_SOURCE_DIR ${APEX_MODULE_DIR}/{{TARGET_MODULE_DIR}})

FIND_PACKAGE(nvToolsExt REQUIRED)

SET({{TARGETNAME_UC}}_PLATFORM_INCLUDES
	${NVTOOLSEXT_INCLUDE_DIRS}
)

SET({{TARGETNAME_UC}}_COMPILE_DEFS
	# Common to all configurations
	${APEX_WINDOWS_COMPILE_DEFS};_WINSOCK_DEPRECATED_NO_WARNINGS;PX_PHYSX_STATIC_LIB

	$<$<CONFIG:debug>:${APEX_WINDOWS_DEBUG_COMPILE_DEFS};PX_PHYSX_DLL_NAME_POSTFIX=DEBUG;>
	$<$<CONFIG:checked>:${APEX_WINDOWS_CHECKED_COMPILE_DEFS};PX_PHYSX_DLL_NAME_POSTFIX=CHECKED;>
	$<$<CONFIG:profile>:${APEX_WINDOWS_PROFILE_COMPILE_DEFS};PX_PHYSX_DLL_NAME_POSTFIX=PROFILE;>
	$<$<CONFIG:release>:${APEX_WINDOWS_RELEASE_COMPILE_DEFS};>
)

# include common ApexTemplate.cmake
INCLUDE(../common/ApexTemplate.cmake)


