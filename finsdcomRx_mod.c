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

#define INBWINDOWSIZE		0x200000
#define MB			262144
enum db{
	DBSYNC=0xE000,
	DBWRITE,
	DBREAD
};

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t dbsync = PTHREAD_COND_INITIALIZER;
static pthread_cond_t dbWrite = PTHREAD_COND_INITIALIZER;
static UINT8 sync=0;
static UINT8 write=0;

void inbDoorbellCb (UINT8 mportId , void *callbackPram, UINT16 doorbell, UINT16 sender){
	printf("Received doorbell %x from device %d\n",doorbell,sender);
	pthread_mutex_lock(&mtx);
    	/*Wake the blocked main thread */
    	if( doorbell == DBSYNC ){
    	sync=1;
    	pthread_cond_broadcast( &dbsync );
	}	
    	else if (doorbell == DBWRITE ){
    	write=1;
    	pthread_cond_broadcast( &dbWrite );
    	}
    	pthread_mutex_unlock(&mtx);
}

int main(int argc, char *argv[]){
	int result;
	int loop1;
	int loop2;
	int loop3;
	int data[INBWINDOWSIZE];
	FILE *fd;
/******************************************************/
        clock_t check;
//        struct timespec start, stop;
//        double ExeTime, ExeTime_nano;
        char buf[100];
/******************************************************/
	UINT8 noOfmport=0;
	UINT16 txId=0xFFFF;
	FINSXRIOMPORTINFO mportInfo;
	FINSXRIOCONFIGOP config;
	FINSXRIOINBOUNDWINDOW inbWindow;
	UINT64 physAddr,rioAddress;
	FINSXRIOMAP	mmap;
	FINSXRIOOUTBDOORBELL outbDb;
	FINSXRIOINBDOORBELL inbDb;
	FINSXRIOSENDDOORBELL dbSend;
        
        printf("\n\n================================================ \n");
        printf("================================================ \n");
	printf("SRIO Bridge Network Test Program - Receiver\n");
        printf("================================================ \n");
        printf("================================================ \n");
        /******************************************************/
        printf("\n");
        check = time(NULL);
        strftime(buf,sizeof(buf), "Program_Started: %a %Y-%m-%d %H:%M:%S location: %Z", localtime(&check));
        printf("%s\n\n", buf);
        /******************************************************/
	if(argc != 4){
		printf("Error: Invalid Arguments..\n");
		printf("Usage: finsdcomRx  <txId> <InbPhyAddress> <rioAddress>\n");
		printf("txId 			- Transmitter Id\n");
		printf("InbPhyAddress		- Inbound memory physical address\n");
		printf("rioAddress		- Inbound Window RIO address\n");
		return -1;
	}

	txId = atoi(argv[1]);
	sscanf(argv[2],"%llx",&physAddr);
	sscanf(argv[3],"%llx",&rioAddress);

	/*Open the access to DCOM driver */
	result=fins_dcomRioOpenDriver();

	if(result){
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
	config.deviceId 	= txId;
	config.configSize	= FINS_RIO_CONFIG_32BIT;
	config.offset 		= RIO_DEV_ID_CAR;

	result = fins_dcomRioReadConfig(&config);

	if(result){
		printf("Error Read configuration data of receiver\n");
		goto closeDrv;
	}

	printf("Trasmitter Board %d Vendor Id= %04X Device Id=%04X\n",txId,(config.data.val32 & 0xFFFF),
						(config.data.val32 >> 16));


	/*Open an inbound window*/
	inbWindow.mportId = 0;
	inbWindow.inbMemPhyAddress = physAddr;
	inbWindow.rioAddress = rioAddress;
	inbWindow.windowSize = INBWINDOWSIZE;

	result = fins_dcomRioMapInboundWindow(&inbWindow);

	if(result){
		printf("Error opening an inbound window\n");
		goto closeDrv;
	}

	/* Map the window to user space */
	mmap.physAddress = inbWindow.inbMemPhyAddress;
	mmap.size		 = INBWINDOWSIZE;
	mmap.offset		 = 0;
	mmap.userAddress = NULL;

	result = fins_dcomRioMmap(&mmap);

	if(result){
		printf("Error in mapping to outbound window to user space\n");
		goto unmapIn;
	}

	printf("Inbound Window 0 of size %x mapped at %p(%llx)to RIO address %llx\n",
			INBWINDOWSIZE,mmap.userAddress,mmap.physAddress,rioAddress);

	/*Register outbound doorbell*/
	outbDb.deviceId = txId;
	outbDb.dbStart  = DBSYNC;
	outbDb.dbEnd	= DBREAD;

	result = fins_dcomRioRegisterOutbDoorbell(&outbDb);

	if(result){
		printf("Error registering outbound doorbell\n");
		goto unmap;
	}

	printf("Successfully registered outbound doorbell for device %d\n",txId);

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
	dbSend.deviceId = txId;
	dbSend.dbInfo = DBSYNC;

	result = fins_dcomRioSendDoorbell(&dbSend);

	if(result){
		printf("Error in sending doorbell %x to device %x\n",
			dbSend.dbInfo,txId);
		goto freeInbDb;
	}

	printf("Send SYNC doorbell to device %d\n",txId);
	printf("Waiting for SYNC doorbell from device %d\n",txId);
	/*Wait for the sync doorbell to be received*/
	pthread_mutex_lock(&mtx);
	if(!sync){
		pthread_cond_wait(&dbsync,&mtx);
	}
	pthread_mutex_unlock(&mtx);

	printf("Obtained SYNC message from transmitter\n");

	/*Re-Send a sync doorbell to receiver */
	dbSend.mportId = 0;
	dbSend.deviceId = txId;
	dbSend.dbInfo = DBSYNC;

	result = fins_dcomRioSendDoorbell(&dbSend);

	if(result){
		printf("Error in sending doorbell %x to device %x\n",
			dbSend.dbInfo,txId);
		goto freeInbDb;
	}


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

	printf("Waiting for WROTE message from transmitter\n");
	pthread_mutex_lock(&mtx);
	if(!write){
		pthread_cond_wait(&dbWrite,&mtx);
	}
	pthread_mutex_unlock (&mtx);

	printf("Received WROTE doorbell from transmitter\n");
	dbSend.mportId = 0;
	dbSend.deviceId = txId;
	dbSend.dbInfo = DBREAD;
        result = fins_dcomRioSendDoorbell(&dbSend);
	loop3 = *(UINT8*)(mmap.userAddress); //getting cycle number
	printf("Dummy file loop size is %d\n\n",loop3);
	pthread_mutex_lock(&mtx);
        if(!write){     
                pthread_cond_wait(&dbWrite,&mtx);
        }
        pthread_mutex_unlock (&mtx);
//// Release 
        printf("================================================ \n\n");
	printf("Receiving dummy data from transmitter cycle=%d\n",loop3);
        dbSend.mportId = 0;
        dbSend.deviceId = txId;
        dbSend.dbInfo = DBREAD;
	result = fins_dcomRioSendDoorbell(&dbSend);
	if(result){
		printf("Error in sending doorbell %x to device %x\n",
			dbSend.dbInfo,txId);
		goto freeInbDb;
	}
        for(loop2 = 0; loop2<loop3; loop2++){
                for(loop1 = 0; loop1<MB; loop1++){
                data[loop1] = *(UINT8*)(mmap.userAddress+loop1);//dummy data copy to buffer
                 }
        }
	/*Clean up*/
freeInbDb:
	fins_dcomRioUnregisterInbDoorbell(&inbDb);
freeOutDb:
	fins_dcomRioUnregisterOutbDoorbell(&outbDb);
unmap:
	fins_dcomRioUnmap( &mmap );
unmapIn:
	fins_dcomRioUnmapInboundWindow( &inbWindow );
closeDrv:
	fins_dcomRioCloseDriver();

        fd = fopen("received.dat","w");
        printf("File discripter openned\n");
        for(loop2 = 0; loop2<loop3; loop2++){
                for(loop1 = 0; loop1<MB; loop1++){
                fprintf(fd, "%d",data[loop1]);
                }
        }
 
        fclose(fd);
        printf("Received dummy data saved as 'received.dat'\n");
        printf("\n");
        check = time(NULL);
        strftime(buf,sizeof(buf), "Program_Ended: %a %Y-%m-%d %H:%M:%S location: %Z", localtime(&check));
        printf("%s\n\n", buf);
	return 0;
}
