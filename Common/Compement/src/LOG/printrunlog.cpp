#include "Common/Interfaces/printrunlog.h"

//#ifdef LOG4CPLUS   //通过配置文件配置
void InitLogger(char* configfile)
{
    tcout << LOG4CPLUS_TEXT("Entering main()...") << endl;
    log4cplus::initialize ();
    LogLog::getLogLog()->setInternalDebugging(true);
    Logger root = Logger::getRoot();
    ConfigureAndWatchThread configureThread(configfile, 5 * 1000);
    LOG4CPLUS_INFO( root, " ====================日志记录启动 ====================" );
}

#if 0
#else

//简单日志级别标志，正常运行期间一般设置成空
static volatile char log_level= LOGDPW_LEVEL_ERROR | LOGDPW_LEVEL_WARNING | LOGDPW_LEVEL_INFO | LOGDPW_LEVEL_DEBUG  | LOGDPW_LEVEL_VERBOSE;
//得到当前日志级别
char GetLogRunLevel(void)
{
    return log_level;
}
//重新设置日志级别
void SetLogRunLevel(int level)
{
    log_level=level;
}

#endif
#endif
