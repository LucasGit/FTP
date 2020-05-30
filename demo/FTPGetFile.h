/**********************************************************************//**
                Meteor Firmware Platform
                    FTP Get file module
*-
@file   FTPGetFile.h
@author Lucas
@date   2020/05/30 09:23:43
@brief  module for ftp client get file
**************************************************************************/

#ifndef _FTP_GET_FILE_
#define _FTP_GET_FILE_



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


#define	PORTSERVER	8487
#define CONTROLPORT	PORTSERVER
#define DATAPORT	(PORTSERVER + 1)

enum TYPE
	{
		REQU,
		DONE,
		INFO,
		TERM,
		DATA,
		EOT
	};


struct packet* ntohp(struct packet*);
struct packet* htonp(struct packet*);

#endif