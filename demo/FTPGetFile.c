
#include "FTPGetFile.h"

#define FTP_GET_TASK_STACK_SIZE 4096
#define FTP_GET_TASK_PRIOTITY   2

char s_DataSendBuf[1024];
char s_DataRecvBuf[1024];


int FTPGetFileCore(void *arg)
{
        /** msg parse */


        /** try to recv file from remote ftp server */
}



void FTPGetFileTask(void *arg)
{
    while(1)
    {
        /** get msg from queue */
        
        FTPGetFileCore(arg);
    }
}

void FTPGetFileInit(void)
{
    sys_thread_new("FTPGetThread", FTPGetFileTask, NULL, FTP_GET_TASK_STACK_SIZE, FTP_GET_TASK_PRIOTITY);
}