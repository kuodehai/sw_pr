#输出CMake过程详细信息,不是必须的
SET(CMAKE_VERBOSE_MAKEFILE on)

#搜索PkgConfig库,搜索到才能使用pkg_check_modules命令
FIND_PACKAGE(PkgConfig)
#PKG_CONFIG_FOUND 为true说明找到该模块,可以使用pkg_check_modules命令
IF(PKG_CONFIG_FOUND)
	message("use pkg_check_modules")
ELSE()
	#打印错误并终止cmake
	message(FATAL_ERROR "no pkg_check_modules")

ENDIF(PKG_CONFIG_FOUND)


add_executable(UnitTestShareMem_MMap_Read  local_ipc_data_share_memory.c UnitTestShareMem_MMap_Read.c)

add_executable(UnitTestShareMem_MMap_Write local_ipc_data_share_memory.c  UnitTestShareMem_MMap_Write.c)

