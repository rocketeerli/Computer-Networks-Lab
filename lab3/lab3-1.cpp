/*
* THIS FILE IS FOR IP TEST
*/
// system support
#include "sysInclude.h"
#include <stdio.h>
#include <malloc.h>

extern void ip_DiscardPkt(char* pBuffer,int type);

extern void ip_SendtoLower(char*pBuffer,int length);

extern void ip_SendtoUp(char *pBuffer,int length);

extern unsigned int getIpv4Address();

// implemented by students

int stud_ip_recv(char *pBuffer,unsigned short length)
{
    int version = pBuffer[0] >> 4;  
    int headLength = pBuffer[0] & 0xf; 
	int TTL = (unsigned short)pBuffer[8]; 
	int headCheckSum = ntohs(*(unsigned short *)(pBuffer + 10));
	int dstAddr = ntohl(*(unsigned int*)(pBuffer + 16));
	
	//TTL值错误
	if (TTL <= 0){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_TTL_ERROR);
		return 1;
	}
	
	//IP版本号错
	if (version != 4){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_VERSION_ERROR);
		return 1;
	}
	
	//头部长度错
	if (headLength < 5){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_HEADLEN_ERROR);
		return 1;
	}
	
	//目的地址错
	if (dstAddr != getIpv4Address() && dstAddr != 0xffff){
		ip_DiscardPkt(pBuffer,STUD_IP_TEST_DESTINATION_ERROR);  
		return 1;
	}
	
	//校验和错应该最后检验错误
	unsigned short sum = 0; 
	unsigned short tempNum = 0; 
	for (int i = 0; i < headLength * 2; i++){
		tempNum = ((unsigned char)pBuffer[i*2]<<8) + (unsigned char)pBuffer[i*2 + 1];
		if (0xffff - sum < tempNum)
			sum = sum + tempNum + 1;
		else
			sum = sum + tempNum;
	}
	if (sum != 0xffff){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_CHECKSUM_ERROR);
		return 1;
	}

	//成功接受
 	ip_SendtoUp(pBuffer,length); 
	return 0;
}

int stud_ip_Upsend(char *pBuffer,unsigned short len,unsigned int srcAddr,
				   unsigned int dstAddr,byte protocol,byte ttl)
{
	char *IPBuffer = (char *)malloc((20 + len) * sizeof(char));
	memset(IPBuffer, 0, len+20);  
	IPBuffer[0] = 0x45;	//版本号+头长度
	unsigned short totalLength =  htons(len + 20);	//分组总长度
	memcpy(IPBuffer + 2, &totalLength, 2);
	IPBuffer[8] = ttl;  	//ttl
	IPBuffer[9] = protocol; //协议
      
	unsigned int src = htonl(srcAddr);  
	unsigned int dis = htonl(dstAddr);  
	memcpy(IPBuffer + 12, &src, 4);  //源与目的IP地址
	memcpy(IPBuffer + 16, &dis, 4);  
      
	unsigned short sum = 0; 
	unsigned short tempNum = 0; 
	unsigned short headCheckSum = 0;

	//计算checksum
	for (int i = 0; i < 10; i++){
		tempNum = ((unsigned char)IPBuffer[i*2]<<8) + (unsigned char)IPBuffer[i*2 + 1];
		if (0xffff - sum < tempNum)
			sum = sum + tempNum + 1;
		else
			sum = sum + tempNum;
	}
	headCheckSum = htons(0xffff - sum);  
	memcpy(IPBuffer + 10, &headCheckSum, 2);  
	memcpy(IPBuffer + 20, pBuffer, len);    
	ip_SendtoLower(IPBuffer,len+20);  
	return 0;  
}
