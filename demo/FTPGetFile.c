/**********************************************************************//**
                                Meteor Firmware Platform
                                  FTP client file get module
*-
@file   FTPGetFile.c
@author Lucas
@date   2020/05/30 09:26:19
@brief  FTP client file get module src
**************************************************************************/
#include "FTPGetFile.h"

#include "lwip/opt.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "./FTPGetFile.h"

#define FTP_GET_FILE_DEBUG_EN 1

#if FTP_GET_FILE_DEBUG_EN
        #define FTP_TRACE(Format,...) 		xil_printf((Format), ##__VA_ARGS__)
        #define FTP_ERR(Format,...) 		xil_printf((Format), ##__VA_ARGS__)
#else
        #define FTP_TRACE(Format,...) 		(void)0
        #define FTP_ERR(Format,...) 		(void)0
#endif

#define RECV_BUF_SIZE (32 * 1024)

struct ftp_file_get_ctrl
{
    char *filename;
    char *server_addr;
    short server_port;
};


static char s_FTPFileGetRecvBuf[RECV_BUF_SIZE];
size_t 		size_sockaddr = sizeof(struct sockaddr);

/**********************************************************************//**
@brief   ftp get file core 

@param   arg [In]  ftp get file control

@return  = 0 : succ
		 < 0 : failed
@author Lucas
@date   2020/05/30 16:19:43
@note
History:
**************************************************************************/
int FTPGetFileCore(void *arg)
{
	if(!arg)
	{
		FTP_ERR("#E,invalid arg in core\n");
		return -1;
	}

	struct ftp_file_get_ctrl *pftp_ctrl = (struct ftp_file_get_ctrl*)arg;

	int socketfd;
	int retval;
	struct timeval timeout = {3000,0};
	struct sockaddr_in sin_server;

	if((socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
			FTP_ERR("#E,socket create failed %d\n",socketfd);
			return -2;
	}
		
	memset((char*) &sin_server, 0, sizeof(struct sockaddr_in));
	sin_server.sin_family = AF_INET;
	sin_server.sin_addr.s_addr = inet_addr(pftp_ctrl->server_addr);
	sin_server.sin_port = htons(pftp_ctrl->server_port);

	retval = connect(socketfd, (struct sockaddr*) &sin_server, size_sockaddr);
	if(retval < 0)
	{
		FTP_ERR("#E,socket connect server failed %d\n",retval);
		return -3;
        }
			
	FTP_TRACE("FTP Client started up. Attempting communication with server @ %s:%d...\n\n", 
                                                pftp_ctrl->server_addr, pftp_ctrl->server_port);
	//END: initialization
	lwip_setsockopt(socketfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
	lwip_setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	int file_size = 0;
	/** get file size */
	if((retval = send(socketfd,"filesize", (strlen("filesize") + 1), 0)) < 0)
	{
			FTP_ERR("send get file size command failed failed %d", retval);
			close(socketfd);
			return -4;
	}
		
	if((retval = recv(socketfd, s_FTPFileGetRecvBuf, RECV_BUF_SIZE, 0)) < 0)
	{
			FTP_ERR("#E,recv filesize failed %d", retval);
			close(socketfd);
			return -5;
	}
	else
	{
			file_size = atoi(s_FTPFileGetRecvBuf);
			FTP_TRACE("file size: %d\n",file_size);
	}
	

	/**  file transfer */
	if((retval = send(socketfd,"start", (strlen("start") + 1), 0)) < 0)
	{
			FTP_ERR("start file transfer failed %d", retval);
			close(socketfd);
			return -5;
	}
	else
	{
			int DataRecvd = 0;
			int TotalSize = 0;
			while(file_size > TotalSize)
			{
				/** start recv file */
				if((DataRecvd = recv(socketfd, s_FTPFileGetRecvBuf, RECV_BUF_SIZE, 0)) < 0)
				{
						FTP_ERR("recv data failed %d\n", DataRecvd);
						close(socketfd);
						break;
				}
				else
				{
						/** write file here */
						/** code */

						TotalSize += DataRecvd;
				}
			}
			
		#if FTP_GET_FILE_DEBUG_EN
			if(file_size == TotalSize)
			{
				FTP_TRACE("file recv suceess!\n");
			}
			else
			{	
				FTP_TRACE("file recv failed!\n");
			}
		#endif
	}

	return 0;
}

/** demo */
struct ftp_file_get_ctrl ftp_ctrl;

int FTPFileGetDemo(void)
{
        /** data structure init */
        ftp_ctrl.filename 	 = "Test.log";
        ftp_ctrl.server_addr = "192.168.1.166";
        ftp_ctrl.server_port = 6000;

        /** excute file get*/
        FTPGetFileCore((void*)(&ftp_ctrl));
}
