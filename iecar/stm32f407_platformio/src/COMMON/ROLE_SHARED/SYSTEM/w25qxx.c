#include "w25qxx.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//ֻѧϰʹãδɣκ;
//ALIENTEK STM32F407
//W25QXX 	   
//ԭ@ALIENTEK
//̳:www.openedv.com
//:2014/5/6
//汾V1.0
//ȨУؾ
//Copyright(C) ӿƼ޹˾ 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	
 
u16 W25QXX_TYPE=W25Q128;	//ĬW25Q128

//4KbytesΪһSector
//16Ϊ1Block
//W25Q128
//Ϊ16Mֽ,128Block,4096Sector 
													 
//ʼSPI FLASHIO
void W25QXX_Init(void)
{ 
  GPIO_InitTypeDef  GPIO_InitStructure;
 
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//ʹGPIOBʱ
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);//ʹGPIOGʱ

	  //GPIOB14
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;//PB14
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//
  GPIO_Init(GPIOB, &GPIO_InitStructure);//ʼ

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;//PG7
  GPIO_Init(GPIOG, &GPIO_InitStructure);//ʼ
 
	GPIO_SetBits(GPIOG,GPIO_Pin_7);//PG71,ֹNRFSPI FLASHͨ 
	W25QXX_CS=1;			//SPI FLASHѡ
	SPI1_Init();		   			//ʼSPI
	SPI1_SetSpeed(SPI_BaudRatePrescaler_4);		//Ϊ21Mʱ,ģʽ 
	W25QXX_TYPE=W25QXX_ReadID();	//ȡFLASH ID.
}  

//ȡW25QXX״̬Ĵ
//BIT7  6   5   4   3   2   1   0
//SPR   RV  TB BP2 BP1 BP0 WEL BUSY
//SPR:Ĭ0,״̬Ĵλ,WPʹ
//TB,BP2,BP1,BP0:FLASHд
//WEL:дʹ
//BUSY:æλ(1,æ;0,)
//Ĭ:0x00
u8 W25QXX_ReadSR(void)   
{  
	u8 byte=0;   
	W25QXX_CS=0;                            //ʹ   
	SPI1_ReadWriteByte(W25X_ReadStatusReg);    //Ͷȡ״̬Ĵ    
	byte=SPI1_ReadWriteByte(0Xff);             //ȡһֽ  
	W25QXX_CS=1;                            //ȡƬѡ     
	return byte;   
} 
//дW25QXX״̬Ĵ
//ֻSPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)д!!!
void W25QXX_Write_SR(u8 sr)   
{
	W25QXX_CS=0;                            //ʹ   
	SPI1_ReadWriteByte(W25X_WriteStatusReg);   //дȡ״̬Ĵ    
	SPI1_ReadWriteByte(sr);               //дһֽ  
	W25QXX_CS=1;                            //ȡƬѡ     	      
}   
//W25QXXдʹ	
//WELλ   
void W25QXX_Write_Enable(void)   
{
	W25QXX_CS=0;                            //ʹ   
    SPI1_ReadWriteByte(W25X_WriteEnable);      //дʹ  
	W25QXX_CS=1;                            //ȡƬѡ     	      
} 
//W25QXXдֹ	
//WEL  
void W25QXX_Write_Disable(void)   
{  
	W25QXX_CS=0;                            //ʹ   
    SPI1_ReadWriteByte(W25X_WriteDisable);     //дָֹ    
	W25QXX_CS=1;                            //ȡƬѡ     	      
} 		
//ȡоƬID
//ֵ:				   
//0XEF13,ʾоƬͺΪW25Q80  
//0XEF14,ʾоƬͺΪW25Q16    
//0XEF15,ʾоƬͺΪW25Q32  
//0XEF16,ʾоƬͺΪW25Q64 
//0XEF17,ʾоƬͺΪW25Q128 	  
u16 W25QXX_ReadID(void)
{
	u16 Temp = 0;	  
	W25QXX_CS=0;				    
	SPI1_ReadWriteByte(0x90);//ͶȡID	    
	SPI1_ReadWriteByte(0x00); 	    
	SPI1_ReadWriteByte(0x00); 	    
	SPI1_ReadWriteByte(0x00); 	 			   
	Temp|=SPI1_ReadWriteByte(0xFF)<<8;  
	Temp|=SPI1_ReadWriteByte(0xFF);	 
	W25QXX_CS=1;				    
	return Temp;
}   		    
//ȡSPI FLASH  
//ַָʼȡָȵ
//pBuffer:ݴ洢
//ReadAddr:ʼȡĵַ(24bit)
//NumByteToRead:Ҫȡֽ(65535)
void W25QXX_Read(u8* pBuffer,u32 ReadAddr,u16 NumByteToRead)   
{ 
 	u16 i;   										    
	W25QXX_CS=0;                            //ʹ   
    SPI1_ReadWriteByte(W25X_ReadData);         //Ͷȡ   
    SPI1_ReadWriteByte((u8)((ReadAddr)>>16));  //24bitַ    
    SPI1_ReadWriteByte((u8)((ReadAddr)>>8));   
    SPI1_ReadWriteByte((u8)ReadAddr);   
    for(i=0;i<NumByteToRead;i++)
	{ 
        pBuffer[i]=SPI1_ReadWriteByte(0XFF);   //ѭ  
    }
	W25QXX_CS=1;  				    	      
}  
//SPIһҳ(0~65535)д256ֽڵ
//ַָʼд256ֽڵ
//pBuffer:ݴ洢
//WriteAddr:ʼдĵַ(24bit)
//NumByteToWrite:Ҫдֽ(256),Ӧóҳʣֽ!!!	 
void W25QXX_Write_Page(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite)
{
 	u16 i;  
    W25QXX_Write_Enable();                  //SET WEL 
	W25QXX_CS=0;                            //ʹ   
    SPI1_ReadWriteByte(W25X_PageProgram);      //дҳ   
    SPI1_ReadWriteByte((u8)((WriteAddr)>>16)); //24bitַ    
    SPI1_ReadWriteByte((u8)((WriteAddr)>>8));   
    SPI1_ReadWriteByte((u8)WriteAddr);   
    for(i=0;i<NumByteToWrite;i++)SPI1_ReadWriteByte(pBuffer[i]);//ѭд  
	W25QXX_CS=1;                            //ȡƬѡ 
	W25QXX_Wait_Busy();					   //ȴд
} 
//޼дSPI FLASH 
//ȷдĵַΧڵȫΪ0XFF,ڷ0XFFдݽʧ!
//Զҳ 
//ַָʼдָȵ,ҪȷַԽ!
//pBuffer:ݴ洢
//WriteAddr:ʼдĵַ(24bit)
//NumByteToWrite:Ҫдֽ(65535)
//CHECK OK
void W25QXX_Write_NoCheck(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite)   
{ 			 		 
	u16 pageremain;	   
	pageremain=256-WriteAddr%256; //ҳʣֽ		 	    
	if(NumByteToWrite<=pageremain)pageremain=NumByteToWrite;//256ֽ
	while(1)
	{	   
		W25QXX_Write_Page(pBuffer,WriteAddr,pageremain);
		if(NumByteToWrite==pageremain)break;//д
	 	else //NumByteToWrite>pageremain
		{
			pBuffer+=pageremain;
			WriteAddr+=pageremain;	

			NumByteToWrite-=pageremain;			  //ȥѾд˵ֽ
			if(NumByteToWrite>256)pageremain=256; //һοд256ֽ
			else pageremain=NumByteToWrite; 	  //256ֽ
		}
	};	    
} 
//дSPI FLASH  
//ַָʼдָȵ
//ú!
//pBuffer:ݴ洢
//WriteAddr:ʼдĵַ(24bit)						
//NumByteToWrite:Ҫдֽ(65535)   
u8 W25QXX_BUFFER[4096];		 
void W25QXX_Write(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite)   
{ 
	u32 secpos;
	u16 secoff;
	u16 secremain;	   
 	u16 i;    
	u8 * W25QXX_BUF;	  
   	W25QXX_BUF=W25QXX_BUFFER;	     
 	secpos=WriteAddr/4096;//ַ  
	secoff=WriteAddr%4096;//ڵƫ
	secremain=4096-secoff;//ʣռС   
 	//printf("ad:%X,nb:%X\r\n",WriteAddr,NumByteToWrite);//
 	if(NumByteToWrite<=secremain)secremain=NumByteToWrite;//4096ֽ
	while(1) 
	{	
		W25QXX_Read(W25QXX_BUF,secpos*4096,4096);//
		for(i=0;i<secremain;i++)//У
		{
			if(W25QXX_BUF[secoff+i]!=0XFF)break;//Ҫ  	  
		}
		if(i<secremain)//Ҫ
		{
			W25QXX_Erase_Sector(secpos);//
			for(i=0;i<secremain;i++)	   //
			{
				W25QXX_BUF[i+secoff]=pBuffer[i];	  
			}
			W25QXX_Write_NoCheck(W25QXX_BUF,secpos*4096,4096);//д  

		}else W25QXX_Write_NoCheck(pBuffer,WriteAddr,secremain);//дѾ˵,ֱдʣ. 				   
		if(NumByteToWrite==secremain)break;//д
		else//дδ
		{
			secpos++;//ַ1
			secoff=0;//ƫλΪ0 	 

		   	pBuffer+=secremain;  //ָƫ
			WriteAddr+=secremain;//дַƫ	   
		   	NumByteToWrite-=secremain;				//ֽݼ
			if(NumByteToWrite>4096)secremain=4096;	//һд
			else secremain=NumByteToWrite;			//һд
		}	 
	};	 
}
//оƬ		  
//ȴʱ䳬...
void W25QXX_Erase_Chip(void)   
{                                   
    W25QXX_Write_Enable();                  //SET WEL 
    W25QXX_Wait_Busy();   
  	W25QXX_CS=0;                            //ʹ   
    SPI1_ReadWriteByte(W25X_ChipErase);        //Ƭ  
	W25QXX_CS=1;                            //ȡƬѡ     	      
	W25QXX_Wait_Busy();   				   //ȴоƬ
}   
//һ
//Dst_Addr:ַ ʵ
//һɽʱ:150ms
void W25QXX_Erase_Sector(u32 Dst_Addr)   
{  
	//falsh,   
 	printf("fe:%lx\r\n",(unsigned long)Dst_Addr);	  
 	Dst_Addr*=4096;
    W25QXX_Write_Enable();                  //SET WEL 	 
    W25QXX_Wait_Busy();   
  	W25QXX_CS=0;                            //ʹ   
    SPI1_ReadWriteByte(W25X_SectorErase);      //ָ 
    SPI1_ReadWriteByte((u8)((Dst_Addr)>>16));  //24bitַ    
    SPI1_ReadWriteByte((u8)((Dst_Addr)>>8));   
    SPI1_ReadWriteByte((u8)Dst_Addr);  
	W25QXX_CS=1;                            //ȡƬѡ     	      
    W25QXX_Wait_Busy();   				   //ȴ
}  
//ȴ
void W25QXX_Wait_Busy(void)   
{   
	while((W25QXX_ReadSR()&0x01)==0x01);   // ȴBUSYλ
}  
//ģʽ
void W25QXX_PowerDown(void)   
{ 
  	W25QXX_CS=0;                            //ʹ   
    SPI1_ReadWriteByte(W25X_PowerDown);        //͵  
	W25QXX_CS=1;                            //ȡƬѡ     	      
    delay_us(3);                               //ȴTPD  
}   
//
void W25QXX_WAKEUP(void)   
{  
  	W25QXX_CS=0;                            //ʹ   
    SPI1_ReadWriteByte(W25X_ReleasePowerDown);   //  send W25X_PowerDown command 0xAB    
	W25QXX_CS=1;                            //ȡƬѡ     	      
    delay_us(3);                               //ȴTRES1
}   

























