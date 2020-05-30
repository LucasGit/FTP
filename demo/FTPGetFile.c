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
#include "../Ntrip/BSDSocketIF.h"

#include "./FTPGetFile.h"

#define FTP_CMD_GET_ID 0
#define FTP_GET_FILE_DEBUG_EN 1

#if FTP_GET_FILE_DEBUG_EN
        #define FTP_TRACE(Format,...) 		xil_printf((Format), ##__VA_ARGS__)
        #define FTP_ERR(Format,...) 		xil_printf((Format), ##__VA_ARGS__)
#else
        #define FTP_TRACE(Format,...) 		(void)0
        #define FTP_ERR(Format,...) 		(void)0
#endif



#define	LENBUFFER	504		// so as to make the whole packet well-rounded ( = 512 bytes)
struct packet
{
	short int conid;
	short int type;
	short int comid;
	short int datalen;
	char buffer[LENBUFFER];
};

struct ftp_file_get_ctrl
{
    char *filename;
    char *server_addr;
    short server_port;
    struct packet packet;
};

#define RECV_BUF_SIZE (32 * 1024)


#define	PORTSERVER	8487
#define CONTROLPORT	21// PORTSERVER
#define DATAPORT	1024 // (PORTSERVER + 1)

enum TYPE
	{
		REQU,
		DONE,
		INFO,
		TERM,
		DATA,
		EOT
	};

static char s_FTPFileGetSendBuf[RECV_BUF_SIZE];
static char s_FTPFileGetRecvBuf[RECV_BUF_SIZE];

size_t size_sockaddr = sizeof(struct sockaddr);
static size_t size_packet = sizeof(struct packet);
struct packet data_packet;


void packet_clear(struct packet* p)
{
	memset(p, 0, sizeof(struct packet));
}

int ntohp(struct packet* np,struct packet* hp)
{

	hp->conid = ntohs(np->conid);
	hp->type = ntohs(np->type);
	hp->comid = ntohs(np->comid);
	hp->datalen = ntohs(np->datalen);
	memcpy(hp->buffer, np->buffer, LENBUFFER);

	return 0;	
}

int htonp(struct packet* hp,struct packet* np)
{
	
	np->conid = ntohs(hp->conid);
	np->type  = ntohs(hp->type);
	np->comid = ntohs(hp->comid);
	np->datalen = ntohs(hp->datalen);
	memcpy(np->buffer, hp->buffer, LENBUFFER);
	
	return 0;
}


void receive_file(struct packet* hp, struct packet* data, int sfd, FILE* f)
{
	int x;
	int i = 0, j = 0;
	if((x = recv(sfd, data, size_packet, 0)) <= 0)
	{
		FTP_ERR("#E,recv file\n", x);
	}
		
	j++;
	ntohp(data,hp);
	//printpacket(hp, HP);

	while(hp->type == DATA)
	{
		if((x = recv(sfd, data, size_packet, 0)) <= 0)
		{
			FTP_ERR("#E,recv file data %d", x);
		}
		else
		{
			FTP_TRACE("recv file data %d", x);
		}
                //i += fwrite(hp->buffer, 1, hp->datalen, f);
			
		j++;

		ntohp(data,hp);
		//printpacket(hp, HP);
	}
	//fprintf(stderr, "\t%d data packet(s) received.\n", --j);	// j decremented because the last packet is EOT.
	//fprintf(stderr, "\t%d byte(s) written.\n", i);
	
        if(hp->type == EOT)
        {
                FTP_TRACE("end of file\n");
                return;
        }
	else
	{
		FTP_ERR("#E,occured while downloading remote file.\n");
	}

	//fflush(stderr);
}

void command_get(struct packet* chp, struct packet* data, int sfd_client, char* filename)
{
	int retval;
	packet_clear(chp);
        
	chp->type  = REQU;
	chp->conid = -1;
	chp->comid = FTP_CMD_GET_ID;
	strcpy(chp->buffer, filename);
	
        htonp(chp,data);

	if((retval = send(sfd_client, data, size_packet, 0)) != size_packet)
        {
                FTP_ERR("send data failed %d", retval);
                return;
        }
		
	if((retval = recv(sfd_client, data, size_packet, 0)) <= 0)
        {
                FTP_ERR("recv data failed %d", retval);

        }
        
	ntohp(data,chp);
	//printpacket(chp, HP);

	if(chp->type == INFO && chp->comid == FTP_CMD_GET_ID && strlen(chp->buffer))
	{
		FTP_TRACE("\t%s\n", chp->buffer);

                
#if 0


                FILE* f = fopen(filename, "wb");
                if(!f)
                {
                        FTP_ERR("#E,File could not be opened for writing. Aborting...\n");
                        return;
                }
#else

                FILE* f = NULL;
                #endif

		receive_file(chp, data, sfd_client, f);
		
	}
	else
        {
                FTP_ERR("#E, getting remote file : <%s>\n", filename);
        }
		
        //fclose(f);
}

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
                return;
        }
		
	if((retval = recv(socketfd, s_FTPFileGetRecvBuf, size_packet, 0)) < 0)
        {
                FTP_ERR("#E,recv filesize failed %d", retval);
                close(socketfd);
                return;
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
                return;
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
                                TotalSize += DataRecvd;
                        }
                        
                }
                
                FTP_TRACE("file recv suceess!\n");
        }
        

        

        /** try to recv file from remote ftp server */
        /** (struct packet* chp, struct packet* data, int sfd_client, char* filename) */
        // command_get(&pftp_ctrl->packet,&data_packet,socketfd,pftp_ctrl->filename);
}

/** demo */
struct ftp_file_get_ctrl ftp_ctrl;

int FTPFileGetDemo(void)
{
        /** data structure init */
        ftp_ctrl.filename = "Test.log";
        ftp_ctrl.server_addr = "192.168.1.166";
        ftp_ctrl.server_port = 6000;

        packet_clear(&ftp_ctrl.packet);

        /** excute file get*/
        FTPGetFileCore((void*)(&ftp_ctrl));
}
