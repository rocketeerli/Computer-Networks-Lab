/*
* THIS FILE IS FOR IP FORWARD TEST
*/
#include "sysInclude.h"
#include <vector>
using std::vector;
#include <iostream>
using std::cout;
// system support
extern void fwd_LocalRcv(char *pBuffer, int length);

extern void fwd_SendtoLower(char *pBuffer, int length, unsigned int nexthop);

extern void fwd_DiscardPkt(char *pBuffer, int type);

extern unsigned int getIpv4Address( );

// implemented by students

struct routeTableItem
{
	unsigned int destIP;     //目的IP
	unsigned int mask;       // 掩码
	unsigned int masklen;    // 掩码长度
	unsigned int nexthop;    // 下一跳
};

vector<routeTableItem> m_table; 

void stud_Route_Init()
{
	m_table.clear();
	return;
}

void stud_route_add(stud_route_msg *proute)
{
	routeTableItem newTableItem;
	newTableItem.masklen = ntohl(proute->masklen);  //将一个无符号长整形数从网络字节顺序转换为主机字节顺序
	newTableItem.mask = (1<<31)>>(ntohl(proute->masklen)-1); //
	newTableItem.destIP = ntohl(proute->dest);
	newTableItem.nexthop = ntohl(proute->nexthop);
	m_table.push_back(newTableItem);
	return;
}

int stud_fwd_deal(char *pBuffer, int length)
{

	int TTL = (int)pBuffer[8];  //存储TTL
	int headerChecksum = ntohl(*(unsigned short*)(pBuffer+10)); 
	int DestIP = ntohl(*(unsigned int*)(pBuffer+16));
    int headsum = pBuffer[0] & 0xf; 


	if(DestIP == getIpv4Address()) //判断分组地址与本机地址是否相同
	{
		fwd_LocalRcv(pBuffer, length); //将 IP 分组上交本机上层协议 
		return 0;
	}

	if(TTL <= 0) //TTL 判断 小于0 不能转发 丢弃 IP 分组
	{
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_TTLERROR); //丢弃 IP 分组
		return 1;
	}

	//设置匹配位
	bool Match = false;
	unsigned int longestMatchLen = 0;
	int bestMatch = 0;
	// 判断掩码是否匹配
	for(int i = 0; i < m_table.size(); i ++)
	{
		if(m_table[i].masklen > longestMatchLen && m_table[i].destIP == (DestIP & m_table[i].mask)) // 
		{
			bestMatch = i;
			Match = true;
			longestMatchLen = m_table[i].masklen;
		}
	}

	if(Match) //匹配成功
	{
		char *buffer = new char[length];
            memcpy(buffer,pBuffer,length);
            buffer[8]--; //TTL - 1
		int sum = 0;
            unsigned short int localCheckSum = 0;
            for(int j = 1; j < 2 * headsum +1; j ++)
            {
                if (j != 6){ 
                    sum = sum + (buffer[(j-1)*2]<<8)+(buffer[(j-1)*2+1]);
                    sum %= 65535; 
                }
            }
            //重新计算校验和
           	localCheckSum = htons(~(unsigned short int)sum);
		memcpy(buffer+10, &localCheckSum, sizeof(unsigned short));
		// 发给下一层协议
		fwd_SendtoLower(buffer, length, m_table[bestMatch].nexthop);
		return 0;
	}
	else //匹配失败
	{
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_NOROUTE); //丢弃 IP 分组
		return 1;
	}
	return 1;
}
