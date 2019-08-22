
/**
 * @author wangdaopo
 * @email 3168270295@qq.com
 */

#ifndef PRINTRUNLOG_H
#define PRINTRUNLOG_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>




#if 1

#if 0

// LOG4CPLUS  //通过配置文件配置

#include "log4cplus/logger.h"
#include "log4cplus/configurator.h"
#include "log4cplus/helpers/loglog.h"
#include "log4cplus/helpers/stringhelper.h"
//#include "log4cplus/helpers/sleep.h"
#include "log4cplus/loggingmacros.h"
using namespace std;
using namespace log4cplus;
using namespace log4cplus::helpers;

void  InitLogger(const char* configfile);

//输出运行错误错误宏
#define LOG_RUN_ERROR(modelname,format,...)  do{\
        LOG4CPLUS_ERROR(modelname, " " format,##__VA_ARGS__);\
    }while(0)


//输出警告信息宏
#define LOG_RUN_WARN(modelname,format,...)  do{\
        LOG4CPLUS_WARN(modelname, " " format,##__VA_ARGS__);\
    }while(0)


//输出关键信息宏
#define LOG_RUN_INFO(modelname,format,...)  do{\
        LOG4CPLUS_INFO(modelname, " " format,##__VA_ARGS__);\
    }while(0)

//输出调试信息宏
#define LOG_RUN_DEBUG(modelname,format,...)  do{\
        LOG4CPLUS_DEBUG(modelname, " " format,##__VA_ARGS__);\
    }while(0)

//输出全面信息宏
#define LOG_RUN_VERBOSE(modelname,format,...)  do{\
        LOG4CPLUS_TRACE_METHOD(modelname, " " format,##__VA_ARGS__);\
    }while(0)


#else

enum ELogLevel
{
    LOGDPW_LEVEL_NULL=0,//不输出任何提示
    LOGDPW_LEVEL_ERROR=0x01,//只输出系统错误
    LOGDPW_LEVEL_WARNING=0x02,//输出关键信息及警告
    LOGDPW_LEVEL_INFO=0x04,//信息
    LOGDPW_LEVEL_DEBUG=0x08,//调试
    LOGDPW_LEVEL_VERBOSE=0x10,//详细
};
//重新设置日志级别
void SetLogRunLevel(int level);

//得到当前输出等级
char GetLogRunLevel( void );

#ifdef ANDROIDLOGCAT

#include <android/log.h>
//输出运行错误错误宏
#define LOG_RUN_ERROR(modelname,format,...)  do{\
    if(GetLogRunLevel() & LOGDPW_LEVEL_ERROR){\
        __android_log_print(ANDROID_LOG_ERROR,modelname,"[%s:%d]" format, __FILE__,__LINE__ ,##__VA_ARGS__);\
        }\
    }while(0)

//输出警告信息宏
#define LOG_RUN_WARN(modelname,format,...)  do{\
    if(GetLogRunLevel() & LOGDPW_LEVEL_WARNING){\
        __android_log_print(ANDROID_LOG_WARN,modelname,"[%s:%d]" format, __FILE__,__LINE__ ,##__VA_ARGS__);\
        }\
    }while(0)

//输出关键信息宏
#define LOG_RUN_INFO(modelname,format,...)  do{\
    if(GetLogRunLevel() & LOGDPW_LEVEL_INFO){\
        __android_log_print(ANDROID_LOG_INFO,modelname,"[%s:%d]" format, __FILE__,__LINE__ ,##__VA_ARGS__);\
        }\
    }while(0)
//输出调试信息宏
#define LOG_RUN_DEBUG(modelname,format,...)  do{\
    if(GetLogRunLevel() & LOGDPW_LEVEL_DEBUG){\
        __android_log_print(ANDROID_LOG_DEBUG,modelname,"[%s:%d]" format, __FILE__,__LINE__ ,##__VA_ARGS__);\
        }\
    }while(0)
//输出全面信息宏
#define LOG_RUN_VERBOSE(modelname,format,...)  do{\
    if(GetLogRunLevel() & LOGDPW_LEVEL_VERBOSE){\
        __android_log_print(ANDROID_LOG_VERBOSE,modelname,"[%s:%d]" format, __FILE__,__LINE__ ,##__VA_ARGS__);\
        }\
    }while(0)

#else

//输出运行错误错误宏
#define LOG_RUN_ERROR(modelname,format,...)  do{\
    if(GetLogRunLevel() & LOGDPW_LEVEL_ERROR){\
        printf("ERROR: %s [%s:%d][%s]" format " \n\r",  modelname, __FILE__,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
        }\
    }while(0)

//输出警告信息宏
#define LOG_RUN_WARN(modelname,format,...)  do{\
    if(GetLogRunLevel() & LOGDPW_LEVEL_WARNING){\
        printf("WARN: %s [%s:%d][%s]" format "\n\r",  modelname, __FILE__,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
        }\
    }while(0)

//输出关键信息宏
#define LOG_RUN_INFO(modelname,format,...)  do{\
    if(GetLogRunLevel() & LOGDPW_LEVEL_INFO){\
        printf("INFO: %s[%s:%d][%s]" format "\n\r",  modelname, __FILE__,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
        }\
    }while(0)
//输出调试信息宏
#define LOG_RUN_DEBUG(modelname,format,...)  do{\
    if(GetLogRunLevel() & LOGDPW_LEVEL_DEBUG){\
        printf("DEBUG: %s[%s:%d][%s]" format "\n\r",  modelname, __FILE__,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
        }\
    }while(0)
//输出全面信息宏
#define LOG_RUN_VERBOSE(modelname,format,...)  do{\
    if(GetLogRunLevel() & LOGDPW_LEVEL_VERBOSE){\
        printf("VERBOSE: %s[%s:%d][%s]" format "\n\r",  modelname, __FILE__,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
        }\
    }while(0)

#endif


#endif

#endif


#endif // PRINTRUNLOG_H
