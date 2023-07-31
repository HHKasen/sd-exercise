/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * @file    user_diskio.c
  * @brief   This file includes a diskio driver skeleton to be completed by the user.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
 /* USER CODE END Header */

#ifdef USE_OBSOLETE_USER_CODE_SECTION_0
/*
 * Warning: the user section 0 is no more in use (starting from CubeMx version 4.16.0)
 * To be suppressed in the future.
 * Kept to ensure backward compatibility with previous CubeMx versions when
 * migrating projects.
 * User code previously added there should be copied in the new user sections before
 * the section contents can be deleted.
 */
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
#endif

/* USER CODE BEGIN DECL */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "ff_gen_drv.h"

/* Private typedef -----------------------------------------------------------*/
typedef enum {
	CMD0 = 0, //GO_IDLE_STATE
	CMD1 = 1,//SEND_OP_CMD
	ACMD41 = 41, //APP_SEND_OP_CODE - SDC only - app cmd
	CMD8 = 8, //SEND_IF_COND
	CMD9 = 9, //SEND_CSD
	CMD10= 10, //SEND_CID
	CMD12 = 12, //STOP_TRANSMISSION
	CMD16 = 16, //SET_BLOCKLEN
	CMD17 = 17, //READ_SINGLE_BLOCK
	CMD18 = 18, //READ_MULTIPLE_BLOCKS
	CMD23 = 23, // SET_BLOCK_COUNT if appcmd first, becomes SET_WE_BLOCK_ERASE_CNT
	ACMD23 = 23|0x40, //SET_WE_BLOCK_ERASE_CNT - really 23, names like this to prevent conflict
	CMD24 = 24, //WRITE_BLOCK
	CMD25 = 25, //WRITE_MULTIPLE_BLOCK
	CMD55 = 55, //APP_CMD must precede any app cmd
	CMD58 = 58 //READ_OCR
}SD_cmd_t;
/* Private define ------------------------------------------------------------*/
#define SD_CS_PIN  GPIO_PIN_1
#define SD_CS_BANK GPIOB
#define SD_CS_LOW()   HAL_GPIO_WritePin	(SD_CS_BANK,SD_CS_PIN,GPIO_PIN_RESET)
#define SD_CS_HIGH()  HAL_GPIO_WritePin	(SD_CS_BANK,SD_CS_PIN,GPIO_PIN_SET)
#define N_CS 8
/* Private variables ---------------------------------------------------------*/
extern Disk_drvTypeDef  disk;
uint8_t CRCTable[256];

/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

//user fn
void GenerateCRCTable();
uint8_t CRCAdd(uint8_t CRC_val , uint8_t message_byte);
uint8_t getCRC(uint8_t message[], int length);
void get_trail(uint8_t* ocr);
uint8_t send_SD_cmd(SD_cmd_t cmd, uint32_t cmd_arg);


void GenerateCRCTable()
{
  int i, j;
  uint8_t CRCPoly = 0x89;  // the value of our CRC-7 polynomial

  // generate a table value for all 256 possible byte values
  for (i = 0; i < 256; ++i) {
    CRCTable[i] = (i & 0x80) ? i ^ CRCPoly : i;
    for (j = 1; j < 8; ++j) {
        CRCTable[i] <<= 1;
        if (CRCTable[i] & 0x80){
            CRCTable[i] ^= CRCPoly;
        }
    }
  }
}

uint8_t CRCAdd(uint8_t CRC_val , uint8_t message_byte){
	 return CRCTable[(CRC_val << 1) ^ message_byte];
}


// returns the CRC-7 for a message of "length" bytes
uint8_t getCRC(uint8_t message[], int length)
{

  int i;
  uint8_t CRC_val = 0;

  for (i = 0; i < length; ++i){
	  CRC_val = CRCAdd(CRC_val, message[i]);
  }

  return CRC_val;

}

uint8_t send_SD_cmd(SD_cmd_t cmd, uint32_t cmd_arg){

	//should assert that resp!=NULL
	if( (cmd==ACMD41)||(cmd==ACMD23)){
		uint8_t acmd_resp = send_SD_cmd(CMD55,0);
		if(acmd_resp == 0xFF){
			return 0xFF;
		}
	}


	uint8_t tx_high = 0xFF;

	uint8_t MSG[35] = {'\0'};
	uint8_t spi_rx = 0xFF;
	uint8_t rec_res = 0;
    HAL_SPI_Transmit(&HSPI, &tx_high , 1, 50);
    HAL_SPI_Transmit(&HSPI, &tx_high , 1, 50);


	HAL_StatusTypeDef status;
	uint8_t spi_tx_bf[6] = {0};

	spi_tx_bf[0] = (0x40)| cmd;

	spi_tx_bf[4] =  cmd_arg & 0x000000FF;
	spi_tx_bf[3] = (cmd_arg & 0x0000FF00) >> 8;
	spi_tx_bf[2] = (cmd_arg & 0x00FF0000) >> 16;
	spi_tx_bf[1] = (cmd_arg & 0xFF000000) >> 24;

	spi_tx_bf[5] = (getCRC(spi_tx_bf,5)<<1)  + 1;

	//printf(MSG, "cmd:%u %u %u %u %u %u\r\n",spi_tx_bf[0],spi_tx_bf[1],spi_tx_bf[2],spi_tx_bf[3],spi_tx_bf[4],spi_tx_bf[5]);

    HAL_SPI_Transmit(&HSPI, spi_tx_bf , 6, 50);

	int count = 0;


	while( count<N_CS && !rec_res  ){
		HAL_SPI_TransmitReceive(&HSPI, &tx_high, &spi_rx , 1, 50);
		if( (spi_rx&0x80) == 0){
			rec_res = 1;
		}
		//printf("resp:%u\r\n",spi_rx);

		count++;
	}


	if(cmd!=CMD58 && cmd!=CMD8 && cmd!=CMD9 && spi_rx!=0xFF){
		uint8_t spi_rx_clear = 0x0;

		for(int ii=0; ii<8; ii++){
			HAL_SPI_TransmitReceive(&HSPI, &tx_high, &spi_rx_clear , 1, 50);
			if(spi_rx_clear==0xFF){
				break;
			}
		}
	}

	return spi_rx;
}

void get_trail(uint8_t* ocr){
	    uint8_t tx_high = 0xFF;
		uint8_t rx_buff[4] = {0};
		HAL_SPI_TransmitReceive(&HSPI, &tx_high, ocr, 1, 50);
		HAL_SPI_TransmitReceive(&HSPI, &tx_high, ocr+1, 1, 50);
		HAL_SPI_TransmitReceive(&HSPI, &tx_high, ocr+2, 1, 50);
		HAL_SPI_TransmitReceive(&HSPI, &tx_high, ocr+3, 1, 50);


		//sprintf(MSG, "trail:%u %u %u %u\r\n",ocr[0],ocr[1],ocr[2],ocr[3]);
		//HAL_UART_Transmit(&huart2, MSG, sizeof(MSG), 100);
		//printf("trail:%u %u %u %u\r\n",ocr[0],ocr[1],ocr[2],ocr[3]);
		uint8_t spi_rx_clear = 0x0;
		while(spi_rx_clear!=0xFF){
			HAL_SPI_TransmitReceive(&HSPI, &tx_high, &spi_rx_clear , 1, 50);
		}
}




/* USER CODE END DECL */

/* Private function prototypes -----------------------------------------------*/


DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status (BYTE pdrv);
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
  DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
  DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif /* _USE_IOCTL == 1 */

Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read,
#if  _USE_WRITE
  USER_write,
#endif  /* _USE_WRITE == 1 */
#if  _USE_IOCTL == 1
  USER_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_initialize (
	BYTE pdrv           /* Physical drive nmuber to identify the drive */
)
{
  /* USER CODE BEGIN INIT */

    Stat = STA_NOINIT;
	if(pdrv!=0){
	  return Stat;
	}

    GenerateCRCTable();


    uint8_t spi_tx = 0xFF;


    SD_CS_HIGH();

    HAL_Delay(50); //delay at least 1 ms

    //only need 72, do a bunch more
    for(int ii = 0; ii<10; ii++){
  	  HAL_SPI_Transmit(&HSPI, &spi_tx , 1, 0);
    }

    uint8_t R1_resp = 0;
    uint8_t ocr[4] = {0};


    SD_CS_LOW();

    //online comments indicate this makes things more consistent
    for(int ii = 0; ii<2; ii++){
  	  HAL_SPI_Transmit(&HSPI, &spi_tx , 1, 0);
    }

    R1_resp = send_SD_cmd(CMD0,0);


    if(R1_resp == 0x01){


  	  R1_resp = send_SD_cmd(CMD8,0x01AA);
  	  get_trail(ocr);


  	  if( (R1_resp!=0x01)){ //if error or no response, SD1 or MMC
  		  //Won't support this

  	  }
  	  else if( (ocr[2] == 0x01)&&(ocr[3] == 0xAA)  ){ //SD v2
  		  //printf("SD 2.0+\r\n");



  		  uint32_t t_init = HAL_GetTick();
  		  while( (HAL_GetTick()-t_init) < 2000){ //really should be 1000ms, being extra safe


  			  R1_resp = send_SD_cmd(ACMD41,0x40000000);

  			  if(R1_resp==0x00){
  				  break;

  			  }
  		  }




  		  R1_resp = send_SD_cmd(CMD58,0x00000000);
  		  get_trail(ocr);

  		  //printf("OCR status:\r\n");
  		  //printf("\tCCS(0=bytes,1=blocks):%u\r\n", (ocr[0]>>6)&0x01 );

  		  //force 512 byte blocks
  		  R1_resp = send_SD_cmd(CMD16,0x00000200);


  		  //I don't, but could check voltage here
  		  Stat=0;
  		  //after init, can speed up spi
  		  HSPI.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;

  	  }
  	  else{
  		  //printf("error, CMD8 response:%u\r\n",R1_resp);

  	  }
  	  //else,  simply error
    }
    else{
  	  //printf("error, CMD0 response:%u\r\n",R1_resp);
    }


    return Stat;
  /* USER CODE END INIT */
}

/**
  * @brief  Gets Disk Status
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_status (
	BYTE pdrv       /* Physical drive number to identify the drive */
)
{
  /* USER CODE BEGIN STATUS */

    //Stat = STA_NOINIT;
    return Stat;
  /* USER CODE END STATUS */
}

/**
  * @brief  Reads Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT USER_read (
	BYTE pdrv,      /* Physical drive nmuber to identify the drive */
	BYTE *buff,     /* Data buffer to store read data */
	DWORD sector,   /* Sector address in LBA */
	UINT count      /* Number of sectors to read */
)
{

  /* USER CODE BEGIN READ */

	if(pdrv!=0){
		return RES_ERROR;
	}
	if(USER_status(pdrv)!=0){
		return RES_ERROR;
	}


	if(count==0){
	    return RES_OK;
	}
	else if(count==1){
	  uint8_t R1_resp = send_SD_cmd(CMD17,sector);

	  uint8_t spi_tx = 0xFF;
	  uint8_t spi_rx = 0xFF;

	  int ii = 0;
	  while( (spi_rx == 0xFF)&&(ii<100) ){
		ii++;
		HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx , 1, 50);
	  }

	  //capture data
	  for(ii=0;ii<512;ii++){
	 	HAL_SPI_TransmitReceive(&HSPI, &spi_tx, buff+ii , 1, 50);
	  }

		//read CRC - don't use for now
	  for(ii=0;ii<2;ii++){
		HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx  , 1, 50);
	  }

	  return RES_OK;

	}
	else{
	  uint8_t R1_resp = send_SD_cmd(CMD18,sector);

	  uint8_t spi_tx = 0xFF;
	  uint8_t spi_rx = 0xFF;


	  for(int jj=0; jj<count; jj++){

		int ii = 0;
		spi_rx = 0xFF;
		while( (spi_rx == 0xFF)&&(ii<100) ){
		  ii++;
		  HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx , 1, 50);
		}

			//capture data
		for(ii=0;ii<512;ii++){
		  HAL_SPI_TransmitReceive(&HSPI, &spi_tx, buff+ii+(jj*512) , 1, 50);
		}

			//read CRC - don't use for now
		for(ii=0;ii<2;ii++){
		  HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx, 1, 50);
		}
	  }


	  R1_resp = send_SD_cmd(CMD12,0x0);

	  int busy = 1;
	  int counter = 0;
	  while(busy&&(counter<1000) ){
		HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx, 1, 50);
		if(spi_rx!=0){
		  busy = 0;
		}
		counter++;
	  }

	  return RES_OK;

	}

	//shouldn't get here
    return RES_OK;
  /* USER CODE END READ */
}

/**
  * @brief  Writes Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT USER_write (
	BYTE pdrv,          /* Physical drive nmuber to identify the drive */
	const BYTE *buff,   /* Data to be written */
	DWORD sector,       /* Sector address in LBA */
	UINT count          /* Number of sectors to write */
)
{
  /* USER CODE BEGIN WRITE */
  /* USER CODE HERE */

	if(pdrv!=0){
		return RES_ERROR;
	}
	if(USER_status(pdrv)!=0){
		return RES_ERROR;
	}

	if(count==0){
	  return RES_OK;
	}
	else if(count==1){
	  uint8_t R1_resp = send_SD_cmd(CMD24,sector);

	  uint8_t spi_tx = 0xFF;
	  uint8_t spi_rx = 0xFF;

	  //1 byte of delat


	  //send data packet
	  spi_tx = 0xFE;
	  HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx , 1, 50);

	  //send data
	  for(int ii=0;ii<512;ii++){
		HAL_SPI_TransmitReceive(&HSPI, buff+ii, &spi_rx, 1, 50);
	  }

	  //crc, dummy packets
	  for(int ii=0;ii<2;ii++){
		HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx, 1, 50);
	  }

	    //get data response, fail if not data accepted
	  spi_tx = 0xFF;
	  HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx, 1, 50);

	  int busy = 1;
	  int counter = 0;
	  while(busy&&(counter<1000) ){
		HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx, 1, 50);
		if(spi_rx!=0){
	  	  busy = 0;
		}
		counter++;
	  }

	  return RES_OK;
	}
	else{
		uint8_t R1_resp = send_SD_cmd(CMD25,sector);

		uint8_t spi_tx = 0xFF;
		uint8_t spi_rx = 0xFF;

		//one buffer of nothing
		HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx, 1, 50);


		for(int jj=0; jj<count; jj++){


			//send data packet
			spi_tx = 0xFC;
			HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx , 1, 50);

			//send data
			//spi_tx = 0xAC;
			for(int ii=0;ii<512;ii++){
	//			HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx, 1, 50);
				HAL_SPI_TransmitReceive(&HSPI, buff+ii+(512*jj), &spi_rx, 1, 50);
			}

			spi_tx = 0x00;
			//crc, dummy packets
			for(int ii=0;ii<2;ii++){
				HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx, 1, 50);
			}

			//get data response, fail if not data accepted
			spi_tx = 0xFF;
			HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx, 1, 50);

			int busy = 1;
			int counter = 0;
			spi_tx = 0xFF;

			while(busy&&(counter<1000) ){
				HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx, 1, 50);
				if(spi_rx!=0){
					busy = 0;
				}
				counter++;
			}

		}

		//send stop token
		spi_tx = 0xFD;
		HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx , 1, 50);

		//
		spi_tx = 0xFF;
		HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx , 1, 50);


		int busy = 1;
		int counter = 0;
		while(busy&&(counter<1000) ){
			HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx, 1, 50);
			if(spi_rx!=0){
				busy = 0;
			}
			else{
			}
			counter++;
		}
	}

	//shouldn't get here
    return RES_OK;
  /* USER CODE END WRITE */
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  pdrv: Physical drive number (0..)
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT USER_ioctl (
	BYTE pdrv,      /* Physical drive nmuber (0..) */
	BYTE cmd,       /* Control code */
	void *buff      /* Buffer to send/receive control data */
)
{
  /* USER CODE BEGIN IOCTL */
    DRESULT res = RES_ERROR;
	switch(cmd){

	case CTRL_SYNC:
		// nothing to do here
		res = RES_OK;
		break;
	case GET_BLOCK_SIZE:
		*(int*)buff = 512; //buffer size is always 512
		res = RES_OK;
		break;
	case GET_SECTOR_SIZE:
		*(int*)buff = 512; //buffer size is always 512
		res = RES_OK;
		break;

	case GET_SECTOR_COUNT:
		if(USER_status(pdrv)!=0){
			return RES_ERROR;
		}
		else{
			BYTE buffer[16] = {0};
			//printf("GET_BLOCK_SIZE\r\n");
			uint8_t R1_resp = send_SD_cmd(CMD9,0x0);
			//printf("cmd resp:%u\r\n",R1_resp);
			uint8_t spi_tx = 0xFF;
			uint8_t spi_rx = 0xFF;

			int ii = 0;
			while( (spi_rx == 0xFF)&&(ii<10000) ){
				ii++;
				HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx , 1, 50);
			}

			//printf("ii:%u\r\n",ii);
			//printf("data token:%u\r\n",spi_rx);

			//capture data
			for(ii=0;ii<16;ii++){
				HAL_SPI_TransmitReceive(&HSPI, &spi_tx, buffer+15-ii , 1, 50);
				//printf("(%u,%u)\r\n",8*(15-ii),buffer[ii]);
				printf("(%u,%u)\r\n",8*(15-ii),buffer[15-ii]);

			}


			for(ii=0;ii<2;ii++){
				HAL_SPI_TransmitReceive(&HSPI, &spi_tx, &spi_rx  , 1, 50);
				printf("rx crc:%u\r\n",spi_rx);
			}

			//printf( "sector size: %u\r\n", ((buffer[5]&0x3F)<<1)|(buffer[4]>>7) );//  | buffer[4]>>7) );
			uint32_t mem_size = 0;
			mem_size = (uint32_t)buffer[6] | ( ((uint32_t) buffer[7])<<8) | (((uint32_t)(buffer[8]&0x3F))<<16) ;
//			printf("48:%u\r\n",buffer[6]);
//			printf("56:%u\r\n",buffer[7]);
//			printf("64:%u\r\n",buffer[8]);


//			printf("mem size(kB):%u\r\n",(mem_size+1)*512);
//			printf("mem size(sectors):%u\r\n",(mem_size+1)*1000);
		  *(int*)buff = ((mem_size+1)*1000);
	      res =  RES_OK;

		}

		break;
	case CTRL_TRIM:
		printf("CTRL_TRIM\r\n");
		break;
	default:
		res = RES_PARERR;
	}
	return res;


  /* USER CODE END IOCTL */
}
#endif /* _USE_IOCTL == 1 */

