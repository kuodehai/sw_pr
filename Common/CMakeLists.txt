project(Common)
add_subdirectory("Compement")


add_library(Common SHARED   
        Compement/src/MemManager/rbtree.c
	Compement/src/MemManager/memory_pool.c
	Compement/src/Thread/thread_attr.cpp
	Compement/src/MQ/msg_def.cpp
	Compement/src/MQ/ipc_mq.cpp
	Compement/src/StateMachine/state_machine.cpp)

target_include_directories(Common PUBLIC
	    "${CMAKE_CURRENT_SOURCE_DIR}/Interfaces/include"
	     "${CMAKE_CURRENT_SOURCE_DIR}/Compement/include")

	      
	     

