/******************************************************/
/********** TEST CODE Modified by Ziho Shin ***********/
/******************************************************/
/******************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include "finsXdcom.h"

#define OUTBWINDOW		       2
#define OUTBWINDOWSIZE		0x200000
#define MB			262144

enum db{
	DBSYNC=0xE000,
	DBWRITE,
	DBREAD
};

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t dbsync = PTHREAD_COND_INITIALIZER;
static pthread_cond_t dbRead = PTHREAD_COND_INITIALIZER;
static UINT8 sync=0;
static UINT8 read=0;

void inbDoorbellCb (UINT8 mportId , void *callbackPram, UINT16 doorbell, UINT16 sender){
	printf("Received doorbell %x from device %d\n",doorbell,sender);
	pthread_mutex_lock(&mtx);
	/*Wake the blocked main thread */
	if(doorbell == DBSYNC){
    	sync=1;
    	pthread_cond_broadcast(&dbsync);
	}	
    	else if (doorbell == DBREAD){
    	read=1;
    	pthread_cond_broadcast(&dbRead);
    	}
    	pthread_mutex_unlock(&mtx);
}
int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1){
    long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
    result->tv_sec = diff / 1000000;
    result->tv_usec = diff % 1000000;

    return (diff<0);
}

int main(int argc, char *argv[]){
	int result;
	int loop1;
	int loop2;
	int loop3;
	FILE *fd;
/******************************************************/
        clock_t check;
        //struct timespec start, stop;
        struct timeval tvBegin, tvEnd, tvDiff;
        //double ExeTime, ExeTime_nano;
        char buf[100];
/******************************************************/
	UINT8 noOfmport=0;
	UINT16 rxId=0xFFFF;
	UINT64 rioAddress;
	FINSXRIOMPORTINFO mportInfo;
	FINSXRIOCONFIGOP config;
	FINSXRIOOUTBOUNDWINDOW outbWindow;
	FINSXRIOOUTBOUNDZONE outbZone;
	FINSXRIOMAP	mmap;
	FINSXRIOOUTBDOORBELL outbDb;
	FINSXRIOINBDOORBELL inbDb;
	FINSXRIOSENDDOORBELL dbSend;

        printf("\n\n================================================ \n");
        printf("================================================ \n");
	printf("SRIO Bridge Network Test Program - Transmitter\n");
        printf("================================================ \n");
        printf("================================================ \n");
        /******************************************************/
        printf("\n");
        check = time(NULL);
        strftime(buf,sizeof(buf), "Program_Started: %a %Y-%m-%d %H:%M:%S location: %Z", localtime(&check));
        printf("%s\n\n", buf);
        /******************************************************/
	if(argc != 3){
		printf("Error: Invalid Arguments..\n");
		printf("Usage: finsdcomTx  <rcvId> <rioAddr>\n");
		printf("rcvId 	- Receiver board id\n");
		printf("rioAddr	- outbound Window RIO Address\n");
		return -1;
	}

	rxId = atoi(argv[1]);
	sscanf(argv[2],"%llx",&rioAddress);

	/*Open the access to DCOM driver */
	result=fins_dcomRioOpenDriver();

	if(result < 0){
		printf("Error in opening FIN-S DCOM driver\n");
		return result;
	}

	/*Get the number of master SRIO ports*/
	result = fins_dcomRioGetNumOfMPort(&noOfmport);

	if(result){
		printf("Error in getting the number of master ports\n");
		goto closeDrv;
	}

	printf("Number of SRIO master ports detected are %d\n",noOfmport);

	/*Get the information of master port*/
	mportInfo.mportId = 0;

	result = fins_dcomRioGetMPortInfo(&mportInfo);

	if(result){
		printf("Error in getting master port 0 information\n");
		goto closeDrv;
	}
        printf("================================================ \n");
	printf("Master Port 0 Information:\n\n");
	printf("Master Port Name			: %s\n",mportInfo.name);
	printf("Master Port Vendor Id			: %04X\n",mportInfo.vendorId);
	printf("Master Port Device Id			: %04X\n",mportInfo.deviceId);
	printf("Master Port System Size			: %s\n",mportInfo.maxSysSize?"65536":"256");
	printf("Master Port BAR 2 Address   		: %08llX\n",mportInfo.outboundMemRegion1);
	printf("Master Port BAR 2 Size			: %08llX\n",mportInfo.outboundMemRegion1Size);
	printf("Master Port BAR 4 Address		: %08llX\n",mportInfo.outboundMemRegion2);
	printf("Master Port BAR 4 Size			: %08llX\n",mportInfo.outboundMemRegion2Size);
        printf("================================================ \n\n");
	config.mportId		= 0;
	config.deviceId 	= rxId;
	config.configSize	= FINS_RIO_CONFIG_32BIT;
	config.offset 		= RIO_DEV_ID_CAR;

	result = fins_dcomRioReadConfig(&config);

	if(result){
		printf("Error Read configuration data of receiver\n");
		goto closeDrv;
	}

	printf("Receiver Board %d Vendor Id= %04X Device Id=%04X\n",
		rxId,(config.data.val32 & 0xFFFF),(config.data.val32 >> 16));

	/*Open an outbound window*/
	outbWindow.mportId = 0;
	outbWindow.window  = OUTBWINDOW;
	outbWindow.outbPhysAddress = mportInfo.outboundMemRegion2;
	outbWindow.windowSize	= OUTBWINDOWSIZE;

	result = fins_dcomRioMapOutboundWindow(&outbWindow);

	if(result){
		printf("Error opening outbound window\n");
		goto closeDrv;
	}

	/*Configure the outbound zone*/
	outbZone.deviceId 	= rxId;
	outbZone.mportId	= 0;
	outbZone.rioAddress	= rioAddress;
	outbZone.window		= OUTBWINDOW;
	outbZone.zone		= 0;
	outbZone.flags		= (FINS_RIO_NREAD|FINS_RIO_NWRITE_SWRITE);

	result = fins_dcomRioConfigOutboundZone(&outbZone);

	if(result){
		printf("Error in configuring outbound zone\n");
		goto unmapOut;
	}

	/* Map the window to user space */
	mmap.physAddress = mportInfo.outboundMemRegion2;
	mmap.size		 = OUTBWINDOWSIZE;
	mmap.offset		 = 0;
	mmap.userAddress = NULL;

	result = fins_dcomRioMmap(&mmap);

	if(result){
		printf("Error in mapping to outbound window to user space\n");
		goto unmapOut;
	}

	printf("Outbound Window 0 of size %x mapped at %p(%llx) to RIO address %llx\n",
			OUTBWINDOWSIZE,mmap.userAddress,mmap.physAddress,rioAddress);


	/*Register outbound doorbell*/
	outbDb.deviceId = rxId;
	outbDb.dbStart  = DBSYNC;
	outbDb.dbEnd	= DBREAD;

	result = fins_dcomRioRegisterOutbDoorbell(&outbDb);

	if(result){
		printf("Error registering outbound doorbell\n");
		goto unmap;
	}

	printf("Successfully registered outbound doorbell for device %d\n",rxId);

	/*Register inbound doorbell*/
	inbDb.mportId = 0;
	inbDb.dbStart = DBSYNC;
	inbDb.dbEnd	  = DBREAD;
	inbDb.callback = inbDoorbellCb;
	inbDb.callbackPram = NULL;

	result = fins_dcomRioRegisterInbDoorbell(&inbDb);

	if(result){
		printf("Error registering inbound doorbell\n");
		goto freeOutDb;
	}

	printf("Sucessfully registered inbound doorbells range %x-%x\n",
		inbDb.dbStart,inbDb.dbEnd);

	/*Send a sync doorbell to receiver*/
	dbSend.mportId = 0;
	dbSend.deviceId = rxId;
	dbSend.dbInfo = DBSYNC;

	result = fins_dcomRioSendDoorbell(&dbSend);

	if(result){
		printf("Error in sending doorbell %x to device %x\n",
												dbSend.dbInfo,rxId);
		goto freeInbDb;
	}

	printf("Send SYNC doorbell to device %d\n",rxId);
	printf("Waiting for SYNC doorbell from device %d\n",rxId);
	/*Wait for the sync doorbell to be received*/
	pthread_mutex_lock(&mtx);
	if (!sync){
		pthread_cond_wait(&dbsync,&mtx);
	}
	pthread_mutex_unlock(&mtx);

	printf("Obtained SYNC message from receiver\n");

	/*Re-Send a sync doorbell to receiver */
	dbSend.mportId = 0;
	dbSend.deviceId = rxId;
	dbSend.dbInfo = DBSYNC;

	result = fins_dcomRioSendDoorbell(&dbSend);

	if(result){
		printf("Error in sending doorbell %x to device %x\n",
			dbSend.dbInfo,rxId);
		goto freeInbDb;
	}


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

	printf("Sending dummy data cycle into outbound window\n");
	loop2=200;
//	loop2=10;
	memset(mmap.userAddress,loop2,1);
        dbSend.mportId = 0;
        dbSend.deviceId = rxId;
        dbSend.dbInfo = DBWRITE;
        result = fins_dcomRioSendDoorbell(&dbSend);
	if(result){
                printf("Error in sending doorbell %x to device %x\n",
			dbSend.dbInfo,rxId);
                goto freeInbDb;
        }
        printf("Waiting for READ doorbell from receiver\n");
        /*Wait for the sync doorbell to be received*/
        pthread_mutex_lock(&mtx);
        if(!read){
                pthread_cond_wait(&dbRead,&mtx);
        }
        pthread_mutex_unlock(&mtx);
        printf("Obtained READ doorbell from receiver\n");
	printf("Dummy file loop size is %d\n\n",loop2);
//// Release
	printf("================================================ \n\n");
	printf("Sending dummy data to receiver cycle=%d\n",loop2);

//////////////////////////////////////////////////////////////////////////////////////////
	//clock_gettime(CLOCK_MONOTONIC, &start);
	 gettimeofday(&tvBegin, NULL);
//////////////////////////////////////////////////////////////////////////////////////////

	for(loop1 = 0; loop1 < loop2; loop1++){
	memset(mmap.userAddress,20,(OUTBWINDOWSIZE/8));
	}
	dbSend.mportId = 0;
	dbSend.deviceId = rxId;
	dbSend.dbInfo = DBWRITE;
	result = fins_dcomRioSendDoorbell(&dbSend);
	if(result){
		printf("Error in sending doorbell %x to device %x\n",
			dbSend.dbInfo,rxId);
		goto freeInbDb;
	}
	pthread_mutex_lock(&mtx);
	if(!read){
		pthread_cond_wait(&dbRead,&mtx);
	}
	pthread_mutex_unlock(&mtx);
//////////////////////////////////////////////////////////////////////////////////////////
	//clock_gettime(CLOCK_MONOTONIC, &stop);
	gettimeofday(&tvEnd, NULL);
	timeval_subtract(&tvDiff, &tvEnd, &tvBegin);
	printf("ExeTime : %ld.%06ld s\n", tvDiff.tv_sec, tvDiff.tv_usec);
	printf("ExeTime : %06ld us\n",tvDiff.tv_usec);
        //ExeTime_nano = (((long)(stop.tv_usec))-((long)(start.tv_usec)));
        //ExeTime_u = (stop.tv_usec)-(start.tv_usec);
	//ExeTime = ExeTime_u * (0.001);
        //printf("\nEXE Time_n: %f usec\n", ExeTime_u);
	//printf("EXE Time_m: %f msec\n\n", ExeTime);
//////////////////////////////////////////////////////////////////////////////////////////
	/*Clean up*/
freeInbDb:
	fins_dcomRioUnregisterInbDoorbell(&inbDb);
freeOutDb:
	fins_dcomRioUnregisterOutbDoorbell(&outbDb);
unmap:
	fins_dcomRioUnmap(&mmap);
unmapOut:
	fins_dcomRioUnmapOutboundWindow(&outbWindow);
closeDrv:
	fins_dcomRioCloseDriver();

        fd = fopen("dummy.db","w");
        printf("File discripter openned\n");
        for(loop1 = 0; loop1 < loop2; loop1++){
                for(loop3 = 0; loop3<MB; loop3++){
                fprintf(fd,"20");
                }
        }
        fclose(fd);
        printf("Sended dummy data saved as 'dummy.db'\n");
        printf("\n");
        check = time(NULL);
        strftime(buf,sizeof(buf), "Program_Ended: %a %Y-%m-%d %H:%M:%S location: %Z", localtime(&check));
        printf("%s\n\n", buf);
	return result;
}
