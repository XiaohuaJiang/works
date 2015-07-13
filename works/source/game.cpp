#include <iostream>  
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <math.h>
#include <sstream>
#include <list>
#include <map>
#include <vector>
#include <iterator>

#define MAX_SIZE 1024
#define SEAT_MSG 0
#define BLIND_MSG 1
#define HOLD_CARD_MSG 2
#define INQUIRE_MSG 3
#define NOTIFY_MSG 4
#define FLOP_MSG 5
#define TURN_MSG 6
#define RIVER_MSG 7
#define SHOWDOWN_MSG 8
#define POT_WIN_MSG 9
#define GAME_OVER_MSG 10
#define SPADES 3 //黑桃
#define HEARTS 2 //红桃
#define CLUBS 1  //梅花
#define DIAMONDS 0 //方片
#define HIGH_CARD 10//高牌
#define ONE_PAIR 11//一对
#define TWO_PAIR 12//两对
#define THREE_OF_A_KIND 13//三条
#define STRAIGHT 14//顺子
#define FLUSH 15//同花
#define FULL_HOUSE 16//葫芦
#define FOUR_OF_A_KIND 17//四条
#define STRAIGHT_FLUSH 18//同花顺
#define ROYAL_STRAIGHT_FLUSH 19 //皇家同花顺
using std::cout;
using std::endl;
using std::string;
using std::map;
using std::vector;
using std::list;
using std::iterator;
using std::max;
using std::min;
using std::ifstream;
using std::ofstream;
using std::stringstream;

struct CPlayerInfoMsg
{
	int m_nJetton;//筹码
	int m_nMoney;//钱
	int m_nStatus;//状态信息
	int m_nOrder;//位置信息：庄家为0，小盲为1，大盲为2，以此类推；
	int m_gBet;//赌注
	string m_gAction;//行动
};

struct CCardInfo
{
	int m_nColor;
	int m_nPoint;
};

struct CPrivateCardInfo
{
	struct CCardInfo card[2];
};

struct CPublicCardInfo
{
	int m_nPublicCardNum;
	struct CCardInfo card[5];
};
 

string g_cMyId;//我的Id号；
int g_nMyOrder;//我的位置；庄家为0，小盲为1，大盲为2，以此类推；
int g_nSmallBlindBet;//小盲注；
int g_nRound=0;//游戏轮次：1为翻牌前；2为收到公牌信息；3为收到转牌信息；4为收到河牌信息；
int g_nGameCount=0;//游戏局数；
int g_nJetton=0;//给玩家分配的初始筹码
int g_nMoney=0;//给玩家分配的初始金钱
int g_nCurrentAlivePlayerNum;//当前存活玩家人数
int g_gBetNum[4]={0};//统计每轮叫注次数
int g_nHandCardLevel;//手牌等级
int g_nHandCardType;//手牌类型
int g_nPublicCardAfterFlopLevel;//公牌轮次公共牌型等级
int g_nPublicCardAfterTurnLevel;//转牌轮次公共牌型等级
int g_nPublicCardAfterRiverLevel;//河牌轮次公共牌型等级
long g_nPublicCardAfterRiverValue;//河牌轮次公牌牌型等级及排序
long g_nCardAfterFlopValue;//公牌轮次牌型等级
long g_nCardAfterTurnValue;//转牌轮次牌型最大等级
long g_nCardAfterRiverValue;//河牌轮次牌型最大等级
int g_nCurrentRoundAllInNum;//当前轮次All-In玩家个数
int g_nCurrentRoundFoldNum;//当前轮次弃牌玩家个数
int g_nCurrentRoundRaiseNum;//当前轮次加注玩家个数
int g_nCurrentRoundCheckNum;//当前轮次让牌玩家个数
int g_nCurrentRoundNeedRaise;//当前轮次需要加注的额度
int g_nCurrentTotalPot;//当前轮次彩池累计总额
int g_nCurrentMyTotalBet;//当前轮次我下注的总额
int g_nCurrentRoundNeedCall;//当前轮次需要跟注的额度
map<string, struct CPlayerInfoMsg> g_gPlayerMap;
struct CPrivateCardInfo g_iMyCard;
struct CPublicCardInfo g_iPublicCard;
struct CCardInfo g_iAllCards[7];
char g_gRecvBuffer[MAX_SIZE];
string g_cLastAction;

int split(const char* pTargetBuffer, string cSubString, vector<string> &gResult)
{
	int nSeparatePos;
	int nResultNum = 0;
	char gBuf[MAX_SIZE];
	strcpy(gBuf, pTargetBuffer);
	char *pBuffer = gBuf;
	while((nSeparatePos = string(pBuffer).find(cSubString)) != string::npos)
	{
		if(nSeparatePos > 0)
		{
			gResult.push_back(string(pBuffer).substr(0, nSeparatePos));
			nResultNum++;
		}
		//str = str.substr(nSeparatePos + 1);
		pBuffer += nSeparatePos + cSubString.length();
	}
	if(strlen(pBuffer) > 0)
	{
		gResult.push_back(string(pBuffer));
		nResultNum++;
	}
	return nResultNum;
}
void clearTheWholeValueBeforeANewSet()
{
	g_nCurrentTotalPot = 0;//彩池清零
	g_gPlayerMap.clear();//清空
	for (int i=0;i<=4;i++)
	{
			g_iPublicCard.card[i].m_nColor=' ';//空格表示花色未知
			g_iPublicCard.card[i].m_nPoint=0;//0表示点数未知
			g_iAllCards[i+2].m_nColor=' ';
			g_iAllCards[i+2].m_nPoint=0;

	}
	for(int i=0;i<=1;i++)
	{
		g_iMyCard.card[i].m_nColor=' ';//空格表示花色未知
		g_iMyCard.card[i].m_nPoint=0;//0表示点数未知
		g_iAllCards[i].m_nColor=' ';
		g_iAllCards[i].m_nPoint=0;
	}
	g_nRound=0;
	for(int i=0;i<=3;i++)
	{
		g_gBetNum[i]=0;
	}	
	 g_nHandCardLevel=-1;//手牌等级
	 g_nHandCardType=-1;//手牌类型
	 g_nCardAfterFlopValue=-1;//公牌轮次牌型等级
	 g_nCardAfterTurnValue=-1;//转牌轮次牌型最大等级
	 g_nCardAfterRiverValue=-1;//河牌轮次牌型最大等级
	 g_nPublicCardAfterRiverValue=-1;//河牌轮次牌型等级及排序
	 g_nPublicCardAfterFlopLevel=-1;//公牌轮次公共牌型等级
	 g_nPublicCardAfterTurnLevel=-1;//转牌轮次公共牌型等级
	 g_nPublicCardAfterRiverLevel=-1;//河牌轮次公共牌型等级
	 g_nCurrentRoundAllInNum=0;//当前轮次All-In玩家个数
	 g_nCurrentRoundFoldNum=0;//当前轮次弃牌玩家个数
	 g_nCurrentRoundRaiseNum=0;//当前轮次弃牌玩家个数
	 g_nCurrentRoundCheckNum=0;//当前轮次让牌玩家个数
     g_nCurrentRoundNeedRaise=0;//当前轮次需要加注的额度
	 g_nCurrentMyTotalBet;//当前轮次我下注的总额
	 g_nCurrentRoundNeedCall;//本局游戏需要跟注的总额
	 g_cLastAction=" ";
}

void sortCards(struct CCardInfo gCard[],int nCardNum)//按牌的点数排序
{
	struct CCardInfo temp ;
	for(int i=0;i<nCardNum-1;++i)
		for(int j=i+1;j<nCardNum;++j)
		{
			if(gCard[i].m_nPoint<gCard[j].m_nPoint)
				{
					temp.m_nColor=gCard[i].m_nColor;
					temp.m_nPoint=gCard[i].m_nPoint;
					gCard[i].m_nColor=gCard[j].m_nColor;
					gCard[i].m_nPoint=gCard[j].m_nPoint;
					gCard[j].m_nColor=temp.m_nColor;
					gCard[j].m_nPoint=temp.m_nPoint;
				}
		}
		
}

bool isStraight(struct CCardInfo gCard[],int nCardNum)//判断nCardNum张牌是不是连续的顺子（不考虑中间插值的情况）
{
	sortCards(gCard,nCardNum);
	for(int i=0;i<nCardNum-1;i++)
		for(int j=i+1;j<nCardNum;j++)
			if(gCard[i].m_nPoint==gCard[j].m_nPoint)
				return false;
	//cout<<"The first card:"<<gCard[0].m_nPoint<<"The last card:"<<gCard[nCardNum-1].m_nPoint<<endl;
	if(gCard[0].m_nPoint-gCard[nCardNum-1].m_nPoint==(nCardNum-1))
	{
		//cout<<"get here!"<<endl;
		return true;
	}
	else if(gCard[0].m_nPoint==14&&gCard[1].m_nPoint==nCardNum)//A 2 3 4 5 是最小的顺子
		return true;
	else 
		return false;
}
bool isFlush(struct CCardInfo gCard[],int nCardNum)
{
	
	for(int i=0;i<nCardNum-1;i++)
			if(gCard[i].m_nColor!=gCard[i+1].m_nColor)
				return false;

		return true;
}
bool isAbleBeStraight(long lNum,int nNum)
{
	long lLon=1000000000L*10;
	int CardLevel=lNum/lLon;
	int nCardPoint[5];
	for(int i=nNum-1;i>=0;i--)
	{
		 nCardPoint[i]=lNum%100;
		 lNum/=100;
	}
	for(int i=0;i<nNum;i++)
	{
		if(nCardPoint[i]-nCardPoint[i+2]<=4)
			return true;
	}
	return false;
}
bool isCurrentCardsAbleBeStraight(int nNum)
{
	/*struct CCardInfo iCard[6];
	for(int i=0;i<=1;i++)
	{
		iCard[i].m_nColor=g_iMyCard.card[i].m_nColor;
		iCard[i].m_nPoint=g_iMyCard.card[i].m_nPoint;
	}
	for(int i=0;i<=3;i++)
	{
		iCard[i+2].m_nColor=g_iPublicCard.card[i].m_nColor;
		iCard[i+2].m_nPoint=g_iPublicCard.card[i].m_nPoint;
	}*/
	sortCards(g_iAllCards,nNum);
	int nNotSameNum=nNum;
	struct CCardInfo temp;
	for(int i=0;i<nNum;i++)
	{
		if(g_iAllCards[i].m_nPoint==g_iAllCards[i+1].m_nPoint)
		{
			temp.m_nColor=g_iAllCards[i+1].m_nColor;
			temp.m_nPoint=g_iAllCards[i+1].m_nPoint;
			for(int j=i+1;j<nNum-1;j++)
				{			
					g_iAllCards[j].m_nColor=g_iAllCards[j+1].m_nColor;
					g_iAllCards[j].m_nPoint=g_iAllCards[j+1].m_nPoint;
				}
			g_iAllCards[nNum-1].m_nColor=temp.m_nColor;
			g_iAllCards[nNum-1].m_nPoint=temp.m_nPoint;
			nNotSameNum--;
		}
			
	}
	if(nNotSameNum<nNum-2)
		return false;
	else
	{
		for(int i=0;i<nNotSameNum-(nNum-2)+1;i++)
		{
			if(g_iAllCards[i].m_nPoint-g_iAllCards[i+nNum-2].m_nPoint<=4)
				return true;
		}
	}
	
	return false;
}
bool isCurrentCardsAbleBeFlush(int nNum)
{
	int gColor[4]={0};
	int nMaxColor;
	for(int i=0;i<nNum;i++)
	{	
			gColor[g_iAllCards[i].m_nColor]++;			
	}
	nMaxColor=gColor[0];
	for(int i=0;i<3;i++)
	{
		nMaxColor=max(nMaxColor,gColor[i+1]);
	}
	if(nMaxColor<nNum-2)
		return false;
	else
		return true;
	
}
bool isCurrentPublicCardsAbleBeStraight(int nNum)
{
	/*struct CCardInfo iCard[6];
	for(int i=0;i<=1;i++)
	{
		iCard[i].m_nColor=g_iMyCard.card[i].m_nColor;
		iCard[i].m_nPoint=g_iMyCard.card[i].m_nPoint;
	}
	for(int i=0;i<=3;i++)
	{
		iCard[i+2].m_nColor=g_iPublicCard.card[i].m_nColor;
		iCard[i+2].m_nPoint=g_iPublicCard.card[i].m_nPoint;
	}*/
	sortCards(g_iPublicCard.card,nNum);
	int nNotSameNum=nNum;
	struct CCardInfo temp;
	cout<<"public card:";
	for(int i=0;i<nNum;i++)
	{
		if(g_iPublicCard.card[i].m_nPoint==g_iPublicCard.card[i+1].m_nPoint)
		{
			temp.m_nColor=g_iPublicCard.card[i+1].m_nColor;
			temp.m_nPoint=g_iPublicCard.card[i+1].m_nPoint;
			for(int j=i+1;j<nNum-1;j++)
				{			
					g_iPublicCard.card[j].m_nColor=g_iPublicCard.card[j+1].m_nColor;
					g_iPublicCard.card[j].m_nPoint=g_iPublicCard.card[j+1].m_nPoint;
				}
			g_iPublicCard.card[nNum-1].m_nColor=temp.m_nColor;
			g_iPublicCard.card[nNum-1].m_nPoint=temp.m_nPoint;
			nNotSameNum--;
			
		}
		cout<<g_iPublicCard.card[i].m_nPoint<<" ";
		
	}
	cout<<endl;	
	if(nNotSameNum<3)
	{
		cout<<"The possibility that public card  be straight is low!"<<endl;
		return false;
	}
	else
	{
		for(int i=0;i<min(nNum-2,nNotSameNum-2);i++)
		{
			if(g_iPublicCard.card[i].m_nPoint-g_iPublicCard.card[i+2].m_nPoint<=4)
			{
			
				cout<<"public card may be straight!"<<endl;
				return true;
			}
		}
	}
	
	cout<<"The possibility that public card  be straight is low!"<<endl;
	return false;
}
bool isCurrentPublicCardsAbleBeFlush(int nNum)
{
	int gColor[4]={0};
	int nMaxColor;
	cout<<"the public card color";
	for(int i=0;i<nNum;i++)
	{	
			gColor[g_iPublicCard.card[i].m_nColor]++;	
			cout<<g_iPublicCard.card[i].m_nColor;
	}
	cout<<endl;
	nMaxColor=gColor[0];
	for(int i=0;i<3;i++)
	{
		
		nMaxColor=max(nMaxColor,gColor[i+1]);
	}
	if(nMaxColor<max(3,nNum-1))
	{
		cout<<"The possibility that public card  be flush is low!"<<endl;
		return false;
	}
	else
		{
			cout<<"public card may be flush!"<<endl;
			return true;
	}
	
}
 /*map<int,list<struct CCardInfo> >  MoreFiveToFiveGroups(struct CCardInfo gCard[],int nCardNum)//从多余五张扑克中选取五张扑克
 {
	map<int,list<struct CCardInfo> > gCardGroup;
        int num = 0;
        for (int a = 0; a < nCardNum-4; a++)
        {
            for (int b = a + 1; b < nCardNum-3; b++)
            {
                for (int c = b + 1; c < nCardNum-2; c++)
                {
                    for (int d = c + 1; d < nCardNum-1; d++)
                    {
                        for (int e = d + 1; e < nCardNum; e++)
                        {
                            list<struct CCardInfo> iPokerGroup;
							struct CCardInfo iPoker;
							iPoker.m_nColor=gCard[a].m_nColor;
							iPoker.m_nPoint=gCard[a].m_nPoint;
							iPokerGroup.push_back(iPoker);
							iPoker.m_nColor=gCard[b].m_nColor;
							iPoker.m_nPoint=gCard[b].m_nPoint;
							iPokerGroup.push_back(iPoker);
							iPoker.m_nColor=gCard[c].m_nColor;
							iPoker.m_nPoint=gCard[c].m_nPoint;
							iPokerGroup.push_back(iPoker);
							iPoker.m_nColor=gCard[d].m_nColor;
							iPoker.m_nPoint=gCard[d].m_nPoint;
							iPokerGroup.push_back(iPoker);
							iPoker.m_nColor=gCard[e].m_nColor;
							iPoker.m_nPoint=gCard[e].m_nPoint;
							iPokerGroup.push_back(iPoker);
                            gCardGroup[num++]=iPokerGroup;
                        }
                    }
                }
            }
        }
        num = 0;
        return gCardGroup;
 }*/
int countFiveCardsLevel(struct CCardInfo gCard[],int nCardNum)
{
	int nPukeLevel = -1;
	if(isFlush(gCard,nCardNum)&&isStraight(gCard,nCardNum))//同花顺
	{
		nPukeLevel =STRAIGHT_FLUSH;
	}
	else if(isFlush(gCard,nCardNum)&&(!isStraight(gCard,nCardNum)))//同花
	{
		nPukeLevel =FLUSH;
	}
	else if(isStraight(gCard,nCardNum))//顺子
	{
		nPukeLevel = STRAIGHT;
	}
	else
	{
		int flag = 0;
		for (int i = 0; i < nCardNum; i++)
		{
			for (int j = i + 1; j < nCardNum; j++)
			{
				if (gCard[i].m_nPoint == gCard[j].m_nPoint)
				{
					flag++;
				}
			}
		}
		switch (flag)
		{
			case 6:
				nPukeLevel =FOUR_OF_A_KIND;break;
			case 4:
				nPukeLevel =FULL_HOUSE;break;
			case 3:
				nPukeLevel =THREE_OF_A_KIND;break;
			case 2:
				nPukeLevel =TWO_PAIR;break;
			case 1:
				nPukeLevel =ONE_PAIR;break;
			case 0:
				nPukeLevel =HIGH_CARD;break;
			default:
				break;
		}  
	}
	return nPukeLevel;
}
void sortFiveCardsNoPair(struct CCardInfo (&gCard)[5])//没有对子且没有顺子时对五张牌进行排序
{
	int nCardNum=5;
	struct CCardInfo temp ;
	for(int i=0;i<nCardNum-1;++i)
		for(int j=i+1;j<nCardNum;++j)
		{
			if(gCard[i].m_nPoint<gCard[j].m_nPoint)
				{
					temp.m_nColor=gCard[i].m_nColor;
					temp.m_nPoint=gCard[i].m_nPoint;
					gCard[i].m_nColor=gCard[j].m_nColor;
					gCard[i].m_nPoint=gCard[j].m_nPoint;
					gCard[j].m_nColor=temp.m_nColor;
					gCard[j].m_nPoint=temp.m_nPoint;
				}
		}
		
}
void sortFiveCardsStraight(struct CCardInfo (&gCard)[5])//有顺子时对五张牌进行排序
{
	int nCardNum=5;
	struct CCardInfo temp ;
	sortFiveCardsNoPair(gCard);
	if(gCard[0].m_nPoint==14&&gCard[1].m_nPoint==5)//如果是最小的顺子将A的值改为1重新排序
	{
		gCard[0].m_nPoint=1;
		sortFiveCardsNoPair(gCard);
	}
		
}
void sortFiveCardsFourOfAKind(struct CCardInfo (&gCard)[5])//有四条时对五张牌进行排序
{
	sortFiveCardsNoPair(gCard);
	if(gCard[0].m_nPoint>gCard[1].m_nPoint)
	{
			struct CCardInfo temp ;
			temp.m_nColor=gCard[0].m_nColor;
			temp.m_nPoint=gCard[0].m_nPoint;
			gCard[0].m_nColor=gCard[4].m_nColor;
			gCard[0].m_nPoint=gCard[4].m_nPoint;
			gCard[4].m_nColor=temp.m_nColor;
			gCard[4].m_nPoint=temp.m_nPoint;
	}
}
void sortFiveCardsFullHouse(struct CCardInfo (&gCard)[5])//有葫芦时对五张牌进行排序
{
	sortFiveCardsNoPair(gCard);
	if(gCard[1].m_nPoint!=gCard[2].m_nPoint)
	{
			struct CCardInfo temp ;
			temp.m_nColor=gCard[0].m_nColor;
			temp.m_nPoint=gCard[0].m_nPoint;
			gCard[0].m_nColor=gCard[3].m_nColor;
			gCard[0].m_nPoint=gCard[3].m_nPoint;
			gCard[3].m_nColor=temp.m_nColor;
			gCard[3].m_nPoint=temp.m_nPoint;
			temp.m_nColor=gCard[1].m_nColor;
			temp.m_nPoint=gCard[1].m_nPoint;
			gCard[1].m_nColor=gCard[4].m_nColor;
			gCard[1].m_nPoint=gCard[4].m_nPoint;
			gCard[4].m_nColor=temp.m_nColor;
			gCard[4].m_nPoint=temp.m_nPoint;
	}
}
void sortFiveCardsThreeOfAKind(struct CCardInfo (&gCard)[5])//有三条时对五张牌进行排序
{
	sortFiveCardsNoPair(gCard);
	struct CCardInfo temp ;
	if((gCard[0].m_nPoint>gCard[1].m_nPoint)&&(gCard[1].m_nPoint==gCard[2].m_nPoint))//三条排在中间
	{
			
			temp.m_nColor=gCard[0].m_nColor;
			temp.m_nPoint=gCard[0].m_nPoint;
			gCard[0].m_nColor=gCard[3].m_nColor;
			gCard[0].m_nPoint=gCard[3].m_nPoint;
			gCard[3].m_nColor=temp.m_nColor;
			gCard[3].m_nPoint=temp.m_nPoint;
	}
	else if(gCard[1].m_nPoint>gCard[2].m_nPoint)//三条排在最后
	{
			
			temp.m_nColor=gCard[0].m_nColor;
			temp.m_nPoint=gCard[0].m_nPoint;
			gCard[0].m_nColor=gCard[3].m_nColor;
			gCard[0].m_nPoint=gCard[3].m_nPoint;
			gCard[3].m_nColor=temp.m_nColor;
			gCard[3].m_nPoint=temp.m_nPoint;
			temp.m_nColor=gCard[1].m_nColor;
			temp.m_nPoint=gCard[1].m_nPoint;
			gCard[1].m_nColor=gCard[4].m_nColor;
			gCard[1].m_nPoint=gCard[4].m_nPoint;
			gCard[4].m_nColor=temp.m_nColor;
			gCard[4].m_nPoint=temp.m_nPoint;
	}
}
void sortFiveCardsTwoPair(struct CCardInfo (&gCard)[5])//有两对时对五张牌进行排序
{
	sortFiveCardsNoPair(gCard);
	int nCardNum = 5;
	struct CCardInfo temp ;
	if(gCard[0].m_nPoint>gCard[1].m_nPoint)//单排在最前面
	{
		temp.m_nColor=gCard[0].m_nColor;
		temp.m_nPoint=gCard[0].m_nPoint;	
		gCard[0].m_nColor=gCard[2].m_nColor;
		gCard[0].m_nPoint=gCard[2].m_nPoint;
		gCard[2].m_nColor=gCard[4].m_nColor;
		gCard[2].m_nPoint=gCard[4].m_nPoint;				
		gCard[4].m_nColor=temp.m_nColor;
		gCard[4].m_nPoint=temp.m_nPoint;
	}
	else if(gCard[1].m_nPoint>gCard[2].m_nPoint&&gCard[2].m_nPoint>gCard[3].m_nPoint)//单排在中间
	{
			
			temp.m_nColor=gCard[2].m_nColor;
			temp.m_nPoint=gCard[2].m_nPoint;
			gCard[2].m_nColor=gCard[4].m_nColor;
			gCard[2].m_nPoint=gCard[4].m_nPoint;
			gCard[4].m_nColor=temp.m_nColor;
			gCard[4].m_nPoint=temp.m_nPoint;
	}
}
void sortFiveCardsOnePair(struct CCardInfo (&gCard)[5])//有一对时对五张牌进行排序
{
	sortFiveCardsNoPair(gCard);
	int nCardNum = 5;
	struct CCardInfo temp ;
	if(gCard[1].m_nPoint==gCard[2].m_nPoint)//对子前面有一张单排
	{
		temp.m_nColor=gCard[0].m_nColor;
		temp.m_nPoint=gCard[0].m_nPoint;	
		gCard[0].m_nColor=gCard[2].m_nColor;
		gCard[0].m_nPoint=gCard[2].m_nPoint;				
		gCard[2].m_nColor=temp.m_nColor;
		gCard[2].m_nPoint=temp.m_nPoint;
	}
	else if(gCard[2].m_nPoint==gCard[3].m_nPoint)//对子前面有两张单排
	{
			
			temp.m_nColor=gCard[0].m_nColor;
			temp.m_nPoint=gCard[0].m_nPoint;
			gCard[0].m_nColor=gCard[2].m_nColor;
			gCard[0].m_nPoint=gCard[2].m_nPoint;
			gCard[2].m_nColor=temp.m_nColor;
			gCard[2].m_nPoint=temp.m_nPoint;
			temp.m_nColor=gCard[1].m_nColor;
			temp.m_nPoint=gCard[1].m_nPoint;
			gCard[1].m_nColor=gCard[3].m_nColor;
			gCard[1].m_nPoint=gCard[3].m_nPoint;
			gCard[3].m_nColor=temp.m_nColor;
			gCard[3].m_nPoint=temp.m_nPoint;
	}
	else if(gCard[3].m_nPoint==gCard[4].m_nPoint)//对子前面有三张单排
	{
			
			temp.m_nColor=gCard[0].m_nColor;
			temp.m_nPoint=gCard[0].m_nPoint;
			gCard[0].m_nColor=gCard[3].m_nColor;
			gCard[0].m_nPoint=gCard[3].m_nPoint;
			gCard[3].m_nColor=gCard[1].m_nColor;
			gCard[3].m_nPoint=gCard[1].m_nPoint;
			gCard[1].m_nColor=gCard[4].m_nColor;
			gCard[1].m_nPoint=gCard[4].m_nPoint;
			gCard[4].m_nColor=gCard[2].m_nColor;
			gCard[4].m_nPoint=gCard[2].m_nPoint;
			gCard[2].m_nColor=temp.m_nColor;
			gCard[2].m_nPoint=temp.m_nPoint;
	}

}
void sortFiveCards(struct CCardInfo (&gCard)[5],int nCardLevel)//按牌的点数排序
{
	switch(nCardLevel)
	{
		case HIGH_CARD:
		case FLUSH:
		case ROYAL_STRAIGHT_FLUSH: 
			sortFiveCardsNoPair(gCard);
			break;
		case STRAIGHT:
		case STRAIGHT_FLUSH:
			sortFiveCardsStraight(gCard);
			break;
		case FOUR_OF_A_KIND:
			sortFiveCardsFourOfAKind(gCard);
			break;
		case FULL_HOUSE:
			sortFiveCardsFullHouse(gCard);
			break;
		case THREE_OF_A_KIND:
			sortFiveCardsThreeOfAKind(gCard);
			break;
		case TWO_PAIR:
			sortFiveCardsTwoPair(gCard);
			break;
		case ONE_PAIR:
			sortFiveCardsOnePair(gCard);
			break;
		default:
			break;
	}
	
		
}
long countFiveCardsValue(struct CCardInfo gCard[],int nCardNum,int nCardType)
{
	long lLon = 1000000000L*10;
	long nCardsLevel = 0L;
	for(int i=0;i<nCardNum;i++)
	{
		int nInterval = 1;
		for(int j=i;j<nCardNum-1;j++)
		{
			nInterval *=10;
			nInterval *=10;
		}
		nCardsLevel+=gCard[i].m_nPoint*nInterval;
	}
	nCardsLevel += nCardType*lLon;
	return nCardsLevel;
}
class CTcpClient
{
private:
	int m_nSocketFd;
	char m_gRecvBuf[MAX_SIZE];
	string m_cPlayerID;
	string m_cPlayerName;
	int __SendMessage(const char *pBuffer, int nSize);
	int __RecieveMessage(char *pBuffer, int nSize);
	int __ClientWrite(const char *pBuffer, int nSize);
	int __ClientRead(char *pBuffer, int nSize);
public:
	CTcpClient(char* argv[]);
	~CTcpClient();
	
	//void connectServer();
	int OnRecvSeatMsg(map<string, struct CPlayerInfoMsg> &gPlayerInfo, char *pBuffer);
	int OnRecvBlindMsg(map<string, struct CPlayerInfoMsg> &gPlayerInfo, char *pBufer);
	int OnRecvHoldCardsMsg(struct CPrivateCardInfo &gHoldCards, char *pBufer);
	int OnRecvFlopCardMsg(struct CPublicCardInfo &gHoldCards, char *pBufer);
	int OnRecvTurnCardsMsg(struct CPublicCardInfo &gHoldCards, char *pBufer);
	int OnRecvRiverCardsMsg(struct CPublicCardInfo &gHoldCards, char *pBufer);
	int OnRecvInquireMsg(map<string, struct CPlayerInfoMsg> &gPlayerInfo, char *pBufer);
	int OnRecvNotifyMsg(map<string, struct CPlayerInfoMsg> &gPlayerInfo, char *pBufer);
	int OnRecvShowDownMsg(map<string, struct CPlayerInfoMsg> &gPlayerInfo, char *pBufer);
	int OnRecvPotWinMsg(map<string, struct CPlayerInfoMsg> &gPlayerInfo, char *pBufer);
	int OnAssessHandCardLevel(void);
	int OnAssessHandCardType(void);
	long OnAssessCardAfterFlop(void);
	long OnAssessCardAfterTurn(void);
	long OnAssessCardAfterRiver(void);
	int OnDecideAction(void);
	int OnGetMsgType(char *pBufer);
};

CTcpClient::CTcpClient(char* argv[])
{
	char* pServerIp =argv[1];
	char* pServerPort = argv[2];
	char* pClientIp = argv[3];
	char* pClientPort = argv[4];
	m_cPlayerID= argv[5];
	g_cMyId=argv[5];
	m_cPlayerName = "zmjxhczj";

	string cRegMsgHead="reg:";
	string cRegMsg=cRegMsgHead+" "+m_cPlayerID+" "+m_cPlayerName+" "+"\n";

	if((m_nSocketFd = socket(AF_INET,SOCK_STREAM,0)) < 0) 
	{
                printf("create socket error: %s(errno:%d)\n)",strerror(errno),errno);
                exit(0);
        }

	struct sockaddr_in iServerAddr;
    memset(&iServerAddr,0,sizeof(iServerAddr));
    iServerAddr.sin_family = AF_INET;
    iServerAddr.sin_port = htons(atoi(pServerPort));

	struct sockaddr_in iClientAddr;
	memset(&iClientAddr,0,sizeof(iClientAddr));
    iClientAddr.sin_family = AF_INET;
    iClientAddr.sin_port = htons(atoi(pClientPort));
	if( inet_pton(AF_INET,pClientIp,&iClientAddr.sin_addr) <=0 )
	{
        printf("inet_pton error for %s\n",pClientIp);
        exit(0);
    }

	if( inet_pton(AF_INET,pServerIp,&iServerAddr.sin_addr) <=0 ) 
	{
        printf("inet_pton error for %s\n",pServerIp);
        exit(0);
    }
	int flag=1,len=sizeof(int);
	if( setsockopt(m_nSocketFd, SOL_SOCKET, SO_REUSEADDR, &flag, len) == -
1) 
   { 
     printf("setsockopt\n"); 
     
   } 
	bind(m_nSocketFd,(const struct sockaddr*)&iClientAddr,sizeof(iClientAddr));

    while( connect(m_nSocketFd,(struct sockaddr*)&iServerAddr,sizeof(iServerAddr))<0) 
	{
       printf("waiting to connect gameserver\n");
       // exit(0);
		sleep(1);
    }

	//if(send( m_nSocketFd,cRegMsg.c_str(),cRegMsg.length(),0 ) < 0)
	if(__SendMessage(cRegMsg.c_str(), cRegMsg.length()) < 0)
	{
        printf("send message error\n");
        exit(0);
    }
}

CTcpClient::~CTcpClient()
{
	if(m_nSocketFd >= 0)
		close(m_nSocketFd);
}

int CTcpClient::__SendMessage(const char *pBuffer, int nSize)
{
	int nTotalLength = 0;
	int nLengh = 0;
	if(m_nSocketFd > 0)
	{
		while(nTotalLength < nSize)
		{
			nLengh = write(m_nSocketFd, pBuffer+nTotalLength, nSize-nTotalLength);
			if(nLengh < 0)
				return -1;
			else
				nTotalLength += nLengh;
		}
		return nTotalLength;
	}
	else
		return -1;
}

int CTcpClient::__RecieveMessage(char *pBuffer, int nSize)
{
	int nTotalLength = 0;
	int nLengh = 0;
	if(m_nSocketFd > 0)
	{
		while(nTotalLength < nSize)
		{
			nLengh = read(m_nSocketFd, pBuffer+nTotalLength, nSize-nTotalLength);
			if(nLengh < 0)
				return -1;
			else
				nTotalLength += nLengh;
		}
		return nTotalLength;
	}
	else
		return -1;
}

int CTcpClient::__ClientWrite(const char *pBuffer, int nSize)
{
	if(m_nSocketFd > 0)
		return write(m_nSocketFd, pBuffer, nSize);
	else
		return -1;
}

int CTcpClient::__ClientRead(char *pBuffer, int nSize)
{
	if(m_nSocketFd > 0)
		return read(m_nSocketFd, pBuffer, nSize);
	else
		return -1;
}

int CTcpClient::OnRecvSeatMsg(map<string, struct CPlayerInfoMsg> &gPlayerInfo, char *pBuffer)
{
	int nLen = 0, nTotalLength = 0;
	char gBuffer[MAX_SIZE];
	vector<string> gSeatInfoMsg;//座次信息
	memset(gBuffer, 0, sizeof(gBuffer));
	nTotalLength = strlen(pBuffer);//当前接收缓存中字符总长度
	while(string(pBuffer).find("/seat \n") == string::npos)
	{
		nLen = __ClientRead(&pBuffer[nTotalLength], MAX_SIZE);//读取实际接收到的字符
		nTotalLength += nLen;//更新总长度
	}
	nLen = string(pBuffer).find("/seat \n");//需要解析的字符串尾部距离接收缓存中首字符的长度
	int nTailLength = strlen("/seat \n");//需要解析的字符串的尾部长度
	strncpy(gBuffer, pBuffer, nLen+nTailLength);//将接收缓存中从最开始一直到待解析字符串尾部结束部分拷贝到gBuffer
	gBuffer[nLen+nTailLength] = 0;

	int nRemainLength = strlen(pBuffer) - nLen - nTailLength;//剩下字符串长度
	for(int i=0; i<nRemainLength; ++i)
		pBuffer[i] = pBuffer[nLen+nTailLength+i];//将解析后剩下的字符串前移到接收字符串首地址
	memset(pBuffer+nRemainLength, 0, MAX_SIZE - nRemainLength);//清空接收缓存已接收到的字符以外的空间

	split(gBuffer, " \n", gSeatInfoMsg);//按换行符拆分座次信息，获取到座次信息每一行数据保存到容器gSeatInfoMsg中
	/*for(unsigned int i=0;i<gSeatInfoMsg.size();i++)//获取座次信息长度
		cout<<gSeatInfoMsg[i]<<endl;*/
	if(gSeatInfoMsg.size())
	{
		if(gSeatInfoMsg[0] != "seat/" || gSeatInfoMsg[gSeatInfoMsg.size()-1] != "/seat")
		{
			cout << "wrong seat vector" << endl;
			return -1;
		}
	}
	else
	{
		cout << "empty seat vector" << endl;
		return -1;
	}
	g_nCurrentAlivePlayerNum=gSeatInfoMsg.size()-2;//当前存活玩家数
	g_nGameCount+=1;
	for(unsigned int i=1; i < gSeatInfoMsg.size()-1; ++i)
	{
		vector<string> gSingleSeatInfoMsg;//每一个玩家的座次信息
		split(gSeatInfoMsg[i].c_str(), ": ", gSingleSeatInfoMsg);
		string cInfo;
		if(gSingleSeatInfoMsg.size())
			cInfo = gSingleSeatInfoMsg[gSingleSeatInfoMsg.size()-1];
		else
			return -1;
		gSingleSeatInfoMsg.clear();
		split(cInfo.c_str(), " ", gSingleSeatInfoMsg);
		if(i == 1)//庄家
		{
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nJetton = atoi(gSingleSeatInfoMsg[1].c_str());
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nMoney = atoi(gSingleSeatInfoMsg[2].c_str());
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nOrder = i-1;
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nStatus = 3;
			if(g_nJetton==0)
				g_nJetton=gPlayerInfo[gSingleSeatInfoMsg[0]].m_nJetton;
			if(g_nMoney==0)
				g_nMoney=gPlayerInfo[gSingleSeatInfoMsg[0]].m_nMoney;
		}
		else if(i == 2)//小盲
		{
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nJetton = atoi(gSingleSeatInfoMsg[1].c_str());
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nMoney = atoi(gSingleSeatInfoMsg[2].c_str());
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nOrder = i-1;
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nStatus = 2;
		}
		else if(i == 3)//大盲
		{
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nJetton = atoi(gSingleSeatInfoMsg[1].c_str());
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nMoney = atoi(gSingleSeatInfoMsg[2].c_str());
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nOrder = i-1;
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nStatus = 1;
		}
		else//普通玩家
		{
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nJetton = atoi(gSingleSeatInfoMsg[1].c_str());
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nMoney = atoi(gSingleSeatInfoMsg[2].c_str());
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nOrder = i-1;
			gPlayerInfo[gSingleSeatInfoMsg[0]].m_nStatus = 0;
		}
	}

	/*cout << m_cPlayerID << " :" << endl;
	for(map<string, struct CPlayerInfoMsg>::iterator itr = gPlayerInfo.begin(); itr != gPlayerInfo.end(); ++itr)
	{
		cout << itr->first << " : m_nJetton = " << itr->second.m_nJetton << " m_nMoney = " << itr->second.m_nMoney << "m_nStatus = " << itr->second.m_nStatus << endl;
	}*/
	return 0;
}

int CTcpClient::OnRecvBlindMsg(map<string, struct CPlayerInfoMsg> &gPlayerInfo, char *pBuffer)
{
	int nLen = 0, nTotalLength = 0;
	char gBuffer[MAX_SIZE];
	vector<string> blindVector;
	string cLine;
	memset(gBuffer, 0, sizeof(gBuffer));
	nTotalLength = strlen(pBuffer);
	while(string(pBuffer).find("/blind \n") == string::npos)
	{
		nLen = __ClientRead(&pBuffer[nTotalLength], MAX_SIZE);
		nTotalLength += nLen;
	}
	nLen = string(pBuffer).find("/blind \n");
	int nTailLength = strlen("/blind \n");
	strncpy(gBuffer, pBuffer, nLen+nTailLength);
	gBuffer[nLen+nTailLength] = 0;

	int nRemainLength = strlen(pBuffer) - nLen - nTailLength;
	for(int i=0; i<nRemainLength; ++i)
		pBuffer[i] = pBuffer[nLen+nTailLength+i];
	memset(pBuffer+nRemainLength, 0, MAX_SIZE - nRemainLength);

	split(gBuffer, " \n", blindVector);
	/*for(unsigned int i=0;i<blindVector.size();i++)
		cout<<blindVector[i]<<endl;*/
	if(blindVector.size())
	{
		if(blindVector[0] != "blind/" || blindVector[blindVector.size()-1] != "/blind")
		{
			cout << "wrong blind vector" << endl;
			return -1;
		}
	}
	else
	{
		cout << "empty blind vector" << endl;
		return -1;
	}

	for(unsigned int i=1; i < blindVector.size()-1; ++i)
	{
		vector<string> singleBlindVector;
		split(blindVector[i].c_str(), ": ", singleBlindVector);
		gPlayerInfo[singleBlindVector[0]].m_nJetton -= atoi(singleBlindVector[1].c_str());
		if(i==1)
			g_nSmallBlindBet=atoi(singleBlindVector[1].c_str());//获取小盲注信息
	}

	/*cout << "after blind :" << endl;
	for(map<string, struct CPlayerInfoMsg>::iterator itr = gPlayerInfo.begin(); itr != gPlayerInfo.end(); ++itr)
	{
		cout << itr->first << " : m_nJetton = " << itr->second.m_nJetton << " m_nMoney = " << itr->second.m_nMoney << "m_nStatus = " << itr->second.m_nStatus << endl;
	}*/
	return 0;
}

int CTcpClient::OnRecvHoldCardsMsg(struct CPrivateCardInfo &holdCards, char *pBuffer)
{
	int nLen = 0, nTotalLength = 0;
	char gBuffer[MAX_SIZE];
	vector<string> holdCardsVector;
	string cLine;
	memset(gBuffer, 0, sizeof(gBuffer));
	nTotalLength = strlen(pBuffer);
	while(string(pBuffer).find("/hold \n") == string::npos)
	{
		nLen = __ClientRead(&pBuffer[nTotalLength], MAX_SIZE);
		nTotalLength += nLen;
	}
	nLen = string(pBuffer).find("/hold \n");
	int nTailLength = strlen("/hold \n");
	strncpy(gBuffer, pBuffer, nLen+nTailLength);
	gBuffer[nLen+nTailLength] = 0;

	int nRemainLength = strlen(pBuffer) - nLen - nTailLength;
	for(int i=0; i<nRemainLength; ++i)
		pBuffer[i] = pBuffer[nLen+nTailLength+i];
	memset(pBuffer+nRemainLength, 0, MAX_SIZE - nRemainLength);

	split(gBuffer, " \n", holdCardsVector);
	/*for(unsigned int i=0;i<holdCardsVector.size();i++)
		cout<< holdCardsVector[i] <<endl;*/
	if(holdCardsVector.size())
	{
		if(holdCardsVector[0] != "hold/" || holdCardsVector[holdCardsVector.size()-1] != "/hold")
		{
			cout << "wrong holds vector" << endl;
			return -1;
		}
	}
	else
	{
		cout << "empty holds vector" << endl;
		return -1;
	}
	g_nRound=1;
	for(unsigned int i=1; i < holdCardsVector.size()-1; ++i)
	{
		vector<string> singleHoldCardsVector;
		split(holdCardsVector[i].c_str(), " ", singleHoldCardsVector);
		switch(singleHoldCardsVector[0][0])
		{
			case 'S':
				holdCards.card[i-1].m_nColor=SPADES;break;
			case 'H':
				holdCards.card[i-1].m_nColor=HEARTS;break;
			case 'C':
				holdCards.card[i-1].m_nColor=CLUBS;break;
			case 'D':
				holdCards.card[i-1].m_nColor=DIAMONDS;break;
			default:
				break;
		}
		if(singleHoldCardsVector[1] == "A")
			holdCards.card[i-1].m_nPoint = 14;
		else if(singleHoldCardsVector[1] == "J")
			holdCards.card[i-1].m_nPoint= 11;
		else if(singleHoldCardsVector[1] == "Q")
			holdCards.card[i-1].m_nPoint = 12;
		else if(singleHoldCardsVector[1] == "K")
			holdCards.card[i-1].m_nPoint = 13;
		else
			holdCards.card[i-1].m_nPoint = atoi(singleHoldCardsVector[1].c_str());
		/*holdCards.card[0].m_nColor = 3;
		holdCards.card[0].m_nPoint = 4;
		holdCards.card[1].m_nColor = 1;
		holdCards.card[1].m_nPoint = 4;*/
		g_iAllCards[i-1].m_nColor=holdCards.card[i-1].m_nColor;
		g_iAllCards[i-1].m_nPoint=holdCards.card[i-1].m_nPoint;
	}

	/*cout << "get holdcards :" << endl;
	for(int i=0; i<2; i++)
	{
		cout << "m_nColor = " << holdCards.card[i].m_nColor << " and m_nPoint = " << holdCards.card[i].m_nPoint << endl;
	}*/
	
		
	g_nHandCardType =OnAssessHandCardType();//评估手牌类型
	g_nHandCardLevel = OnAssessHandCardLevel();//评估手牌等级
	
	return 0;
}

int CTcpClient::OnRecvFlopCardMsg(struct CPublicCardInfo &holdCards, char *pBuffer)
{
	int nLen = 0, nTotalLength = 0;
	char gBuffer[MAX_SIZE];
	vector<string> flopCardsVector;
	string cLine;
	memset(gBuffer, 0, sizeof(gBuffer));
	nTotalLength = strlen(pBuffer);
	while(string(pBuffer).find("/flop \n") == string::npos)
	{
		nLen = __ClientRead(&pBuffer[nTotalLength], MAX_SIZE);
		nTotalLength += nLen;
	}
	nLen = string(pBuffer).find("/flop \n");
	int nTailLength = strlen("/flop \n");
	strncpy(gBuffer, pBuffer, nLen+nTailLength);
	gBuffer[nLen+nTailLength] = 0;

	int nRemainLength = strlen(pBuffer) - nLen - nTailLength;
	for(int i=0; i<nRemainLength; ++i)
		pBuffer[i] = pBuffer[nLen+nTailLength+i];
	memset(pBuffer+nRemainLength, 0, MAX_SIZE - nRemainLength);

	split(gBuffer, " \n", flopCardsVector);
	/*for(unsigned int i=0;i<flopCardsVector.size();i++)
		cout<< flopCardsVector[i] <<endl;*/
	if(flopCardsVector.size())
	{
		if(flopCardsVector[0] != "flop/" || flopCardsVector[flopCardsVector.size()-1] != "/flop")
		{
			cout << "wrong flop vector" << endl;
			return -1;
		}
	}
	else
	{
		cout << "empty flop vector" << endl;
		return -1;
	}
	g_nRound = 2;
	for(unsigned int i=1; i < flopCardsVector.size()-1; ++i)
	{
		vector<string> singleFlopCardsVector;
		split(flopCardsVector[i].c_str(), " ", singleFlopCardsVector);
		switch(singleFlopCardsVector[0][0])
		{
			case 'S':
				holdCards.card[i-1].m_nColor=SPADES;break;
			case 'H':
				holdCards.card[i-1].m_nColor=HEARTS;break;
			case 'C':
				holdCards.card[i-1].m_nColor=CLUBS;break;
			case 'D':
				holdCards.card[i-1].m_nColor=DIAMONDS;break;
			default:
				break;
		}
		if(singleFlopCardsVector[1] == "A")
			holdCards.card[i-1].m_nPoint = 14;
		else if(singleFlopCardsVector[1] == "J")
			holdCards.card[i-1].m_nPoint = 11;
		else if(singleFlopCardsVector[1] == "Q")
			holdCards.card[i-1].m_nPoint = 12;
		else if(singleFlopCardsVector[1] == "K")
			holdCards.card[i-1].m_nPoint = 13;
		else
			holdCards.card[i-1].m_nPoint = atoi(singleFlopCardsVector[1].c_str());
		/*holdCards.card[0].m_nColor = 3;
		holdCards.card[0].m_nPoint = 11;
		holdCards.card[1].m_nColor = 1;
		holdCards.card[1].m_nPoint = 11;
		holdCards.card[2].m_nColor = 3;
		holdCards.card[2].m_nPoint = 3;*/
		g_iAllCards[i+1].m_nColor=holdCards.card[i-1].m_nColor;
		g_iAllCards[i+1].m_nPoint=holdCards.card[i-1].m_nPoint;
	}
		
		
	/*cout << "get flop cards :" << endl;
	for(int i=0; i<3; i++)
	{
		cout << "m_nColor = " << holdCards.card[i].m_nColor << " and m_nPoint = " << holdCards.card[i].m_nPoint << endl;
	}*/
	g_nPublicCardAfterFlopLevel=countFiveCardsLevel(g_iPublicCard.card,3);
	g_nCardAfterFlopValue = OnAssessCardAfterFlop();//判断牌型
	//cout<<"g_nCardAfterFlopValue:"<<g_nCardAfterFlopValue<<endl;
	return 0;
}

int CTcpClient::OnRecvTurnCardsMsg(struct CPublicCardInfo &holdCards, char *pBuffer)
{
	int nLen = 0, nTotalLength = 0;
	char gBuffer[MAX_SIZE];
	vector<string> turnCardsVector;
	string cLine;
	memset(gBuffer, 0, sizeof(gBuffer));
	nTotalLength = strlen(pBuffer);
	while(string(pBuffer).find("/turn \n") == string::npos)
	{
		nLen = __ClientRead(&pBuffer[nTotalLength], MAX_SIZE);
		nTotalLength += nLen;
	}
	nLen = string(pBuffer).find("/turn \n");
	int nTailLength = strlen("/turn \n");
	strncpy(gBuffer, pBuffer, nLen+nTailLength);
	gBuffer[nLen+nTailLength] = 0;

	int nRemainLength = strlen(pBuffer) - nLen - nTailLength;
	for(int i=0; i<nRemainLength; ++i)
		pBuffer[i] = pBuffer[nLen+nTailLength+i];
	memset(pBuffer+nRemainLength, 0, MAX_SIZE - nRemainLength);

	split(gBuffer, " \n", turnCardsVector);
	/*for(unsigned int i=0;i<turnCardsVector.size();i++)
		cout<< turnCardsVector[i] <<endl;*/
	if(turnCardsVector.size())
	{
		if(turnCardsVector[0] != "turn/" || turnCardsVector[turnCardsVector.size()-1] != "/turn")
		{
			cout << "wrong turn vector" << endl;
			return -1;
		}
	}
	else
	{
		cout << "empty turn vector" << endl;
		return -1;
	}
	g_nRound = 3;
	vector<string> singleTurnCardsVector;
	split(turnCardsVector[1].c_str(), " ", singleTurnCardsVector);
	switch(singleTurnCardsVector[0][0])
	{
		case 'S':
			holdCards.card[3].m_nColor=SPADES;break;
		case 'H':
			holdCards.card[3].m_nColor=HEARTS;break;
		case 'C':
			holdCards.card[3].m_nColor=CLUBS;break;
		case 'D':
			holdCards.card[3].m_nColor=DIAMONDS;break;
		default:
			break;
	}
	if(singleTurnCardsVector[1] == "A")
		holdCards.card[3].m_nPoint = 14;
	else if(singleTurnCardsVector[1] == "J")
		holdCards.card[3].m_nPoint = 11;
	else if(singleTurnCardsVector[1] == "Q")
		holdCards.card[3].m_nPoint = 12;
	else if(singleTurnCardsVector[1] == "K")
		holdCards.card[3].m_nPoint = 13;
	else
		holdCards.card[3].m_nPoint = atoi(singleTurnCardsVector[1].c_str());
	/*holdCards.card[3].m_nColor = 2;
	holdCards.card[3].m_nPoint = 4;*/
	g_iAllCards[5].m_nColor=holdCards.card[3].m_nColor;
	g_iAllCards[5].m_nPoint=holdCards.card[3].m_nPoint;
	g_nPublicCardAfterTurnLevel=countFiveCardsLevel(g_iPublicCard.card,4);
	g_nCardAfterTurnValue = OnAssessCardAfterTurn();//判断牌型
	//cout<<"g_nCardAfterTurnValue:"<<g_nCardAfterTurnValue<<endl;
	//cout << "get turn cards :" << endl;
	/*for(int i=0; i<4; i++)
	{
		cout << "m_nColor = " << holdCards.card[i].m_nColor << " and m_nPoint = " << holdCards.card[i].m_nPoint << endl;
	}*/
	return 0;
}

int CTcpClient::OnRecvRiverCardsMsg(struct CPublicCardInfo &holdCards, char *pBuffer)
{
	int nLen = 0, nTotalLength = 0;
	char gBuffer[MAX_SIZE];
	vector<string> riverCardsVector;
	string cLine;
	memset(gBuffer, 0, sizeof(gBuffer));
	nTotalLength = strlen(pBuffer);
	while(string(pBuffer).find("/river \n") == string::npos)
	{
		nLen = __ClientRead(&pBuffer[nTotalLength], MAX_SIZE);
		nTotalLength += nLen;
	}
	nLen = string(pBuffer).find("/river \n");
	int nTailLength = strlen("/river \n");
	strncpy(gBuffer, pBuffer, nLen+nTailLength);
	gBuffer[nLen+nTailLength] = 0;

	int nRemainLength = strlen(pBuffer) - nLen - nTailLength;
	for(int i=0; i<nRemainLength; ++i)
		pBuffer[i] = pBuffer[nLen+nTailLength+i];
	memset(pBuffer+nRemainLength, 0, MAX_SIZE - nRemainLength);

	split(gBuffer, " \n", riverCardsVector);
	/*for(unsigned int i=0;i<riverCardsVector.size();i++)
		cout<< riverCardsVector[i] <<endl;*/
	if(riverCardsVector.size())
	{
		if(riverCardsVector[0] != "river/" || riverCardsVector[riverCardsVector.size()-1] != "/river")
		{
			cout << "wrong river vector" << endl;
			return -1;
		}
	}
	else
	{
		cout << "empty river vector" << endl;
		return -1;
	}
	g_nRound = 4;
	vector<string> singleRiverCardsVector;
	split(riverCardsVector[1].c_str(), " ", singleRiverCardsVector);
	switch(singleRiverCardsVector[0][0])
	{
		case 'S':
			holdCards.card[4].m_nColor=SPADES;break;
		case 'H':
			holdCards.card[4].m_nColor=HEARTS;break;
		case 'C':
			holdCards.card[4].m_nColor=CLUBS;break;
		case 'D':
			holdCards.card[4].m_nColor=DIAMONDS;break;
		default:
			break;
	}
	if(singleRiverCardsVector[1] == "A")
		holdCards.card[4].m_nPoint = 14;
	else if(singleRiverCardsVector[1] == "J")
		holdCards.card[4].m_nPoint = 11;
	else if(singleRiverCardsVector[1] == "Q")
		holdCards.card[4].m_nPoint = 12;
	else if(singleRiverCardsVector[1] == "K")
		holdCards.card[4].m_nPoint = 13;
	else
		holdCards.card[4].m_nPoint = atoi(singleRiverCardsVector[1].c_str());
	g_iAllCards[6].m_nColor=holdCards.card[4].m_nColor;
	g_iAllCards[6].m_nPoint=holdCards.card[4].m_nPoint;
	g_nPublicCardAfterRiverLevel=countFiveCardsLevel(g_iPublicCard.card,5);
	sortFiveCards(g_iPublicCard.card,g_nPublicCardAfterRiverLevel);
	g_nPublicCardAfterRiverValue= countFiveCardsValue(g_iPublicCard.card,5,g_nPublicCardAfterRiverLevel);
	g_nCardAfterRiverValue = OnAssessCardAfterRiver();//判断牌型
	/*cout<<"g_nCardAfterRiverValue:"<<g_nCardAfterRiverValue<<endl;
	cout << "get river cards :" << endl;
	for(int i=0; i<5; i++)
	{
		cout << "m_nColor = " << holdCards.card[i].m_nColor << " and m_nPoint = " << holdCards.card[i].m_nPoint << endl;
	}*/
	return 0;
}

int CTcpClient::OnRecvInquireMsg(map<string, struct CPlayerInfoMsg> &gPlayerInfo, char *pBuffer)
{
	int nLen = 0, nTotalLength = 0;
	char gBuffer[MAX_SIZE];
	vector<string> inquireMsgVector;
	string cLine;
	memset(gBuffer, 0, sizeof(gBuffer));
	nTotalLength = strlen(pBuffer);
	
	while(string(pBuffer).find("/inquire \n") == string::npos)
	{
		nLen = __ClientRead(&pBuffer[nTotalLength], MAX_SIZE);
		nTotalLength += nLen;
	}

	nLen = string(pBuffer).find("/inquire \n");
	int nTailLength = strlen("/inquire \n");
	strncpy(gBuffer, pBuffer, nLen+nTailLength);
	gBuffer[nLen+nTailLength] = 0;

	int nRemainLength = strlen(pBuffer) - nLen - nTailLength;
	for(int i=0; i<nRemainLength; ++i)
		pBuffer[i] = pBuffer[nLen+nTailLength+i];
	memset(pBuffer+nRemainLength, 0, MAX_SIZE - nRemainLength);
	
	split(gBuffer, " \n", inquireMsgVector);
	/*for(unsigned int i=0;i<inquireMsgVector.size();i++)
		cout<< inquireMsgVector[i] <<endl;*/
	if(inquireMsgVector.size())
	{
		if(inquireMsgVector[0] != "inquire/" || inquireMsgVector[inquireMsgVector.size()-1] != "/inquire")
		{
			cout << "wrong inquire vector" << endl;
			return -1;
		}
	}
	else
	{
		cout << "empty inquire vector" << endl;
		return -1;
	}
	int nTemp;
	nTemp=g_nCurrentTotalPot;
	g_nCurrentTotalPot = atoi(inquireMsgVector[inquireMsgVector.size()-2].substr(inquireMsgVector[inquireMsgVector.size()-2].find

(": ")+2).c_str());
	g_nCurrentRoundAllInNum=0;
	g_nCurrentRoundFoldNum=0;
	g_nCurrentRoundRaiseNum=0;
	g_nCurrentRoundCheckNum=0;
	for(unsigned int i=1; i < inquireMsgVector.size()-2; ++i)
	{
		vector<string> singleInquireMsgVector;
		split(inquireMsgVector[i].c_str(), " ", singleInquireMsgVector);
		gPlayerInfo[singleInquireMsgVector[0]].m_nJetton = atoi(singleInquireMsgVector[1].c_str());
		gPlayerInfo[singleInquireMsgVector[0]].m_nMoney = atoi(singleInquireMsgVector[2].c_str());
		gPlayerInfo[singleInquireMsgVector[0]].m_gBet=atoi(singleInquireMsgVector[3].c_str());
		gPlayerInfo[singleInquireMsgVector[0]].m_gAction=singleInquireMsgVector[4];
		if(1==i)g_cLastAction=singleInquireMsgVector[4];
		else if(g_cLastAction=="fold")
			g_cLastAction=singleInquireMsgVector[4];//其他玩家最近一次非弃牌行动
		if(singleInquireMsgVector[4]=="all_in")//统计目前all_in玩家个数
			g_nCurrentRoundAllInNum++;
		if(singleInquireMsgVector[4]=="fold")//统计目前弃牌玩家个数
			g_nCurrentRoundFoldNum++;
		if(singleInquireMsgVector[4]=="raise")
			g_nCurrentRoundRaiseNum++;
		if(singleInquireMsgVector[4]=="check")
		{
			if(singleInquireMsgVector[0]!=g_cMyId)//统计出我自己以外加注玩家个数
				g_nCurrentRoundCheckNum++;
		}
			g_nCurrentRoundCheckNum=0;//统计目前让牌玩家个数
		  g_nCurrentRoundNeedRaise=max(g_nCurrentTotalPot -nTemp-2*g_nSmallBlindBet,2*g_nSmallBlindBet);//本局游戏需要加注的额度
		  g_nCurrentRoundNeedCall=max(g_nCurrentTotalPot -nTemp,g_nCurrentRoundNeedCall);//本局游戏需要跟住的额度
		
		
	}
	//cout<<"raise number:"<<g_nCurrentRoundRaiseNum<<endl;
	/*cout<<"all_in number:"<<g_nCurrentRoundAllInNum<<endl;
	cout<<"fold number:"<<g_nCurrentRoundFoldNum<<endl;
	cout << "get inquire msgs :" << endl;*/
	/*for(map<string, struct CPlayerInfoMsg>::iterator itr = gPlayerInfo.begin(); itr != gPlayerInfo.end(); ++itr)
	{
		cout << itr->first << " : " << endl;
		cout << "m_nJetton = " << itr->second.m_nJetton << " m_nMoney = " << itr->second.m_nMoney << " m_nStatus = " << itr->second.m_nStatus << endl;
		cout << "its action is :";
		for(vector<string>::iterator itrStr = itr->second.m_gAction.begin(); itrStr != itr->second.m_gAction.end(); ++itrStr)
			cout << *itrStr << " ";
		cout << endl << "its m_gBet is : ";
		for(vector<int>::iterator itrStr = itr->second.m_gBet.begin(); itrStr != itr->second.m_gBet.end(); ++itrStr)
			cout << *itrStr << " ";
		cout << endl;
	}*/
	g_gBetNum[g_nRound-1]++;
	return 0;
}
int CTcpClient::OnRecvNotifyMsg(map<string, struct CPlayerInfoMsg> &gPlayerInfo, char *pBuffer)
{
	int nLen = 0, nTotalLength = 0;
	char gBuffer[MAX_SIZE];
	vector<string> notifyMsgVector;
	string cLine;
	memset(gBuffer, 0, sizeof(gBuffer));
	nTotalLength = strlen(pBuffer);
	
	while(string(pBuffer).find("/notify \n") == string::npos)
	{
		nLen = __ClientRead(&pBuffer[nTotalLength], MAX_SIZE);
		nTotalLength += nLen;
	}

	nLen = string(pBuffer).find("/notify \n");
	int nTailLength = strlen("/notify \n");
	strncpy(gBuffer, pBuffer, nLen+nTailLength);
	gBuffer[nLen+nTailLength] = 0;

	int nRemainLength = strlen(pBuffer) - nLen - nTailLength;
	for(int i=0; i<nRemainLength; ++i)
		pBuffer[i] = pBuffer[nLen+nTailLength+i];
	memset(pBuffer+nRemainLength, 0, MAX_SIZE - nRemainLength);
	
	split(gBuffer, " \n", notifyMsgVector);
	/*for(unsigned int i=0;i<notifyMsgVector.size();i++)
		cout<< notifyMsgVector[i] <<endl;*/
	if(notifyMsgVector.size())
	{
		if(notifyMsgVector[0] != "notify/" || notifyMsgVector[notifyMsgVector.size()-1] != "/notify")
		{
			cout << "wrong inquire vector" << endl;
			return -1;
		}
	}
	else
	{
		cout << "empty inquire vector" << endl;
		return -1;
	}

	g_nCurrentTotalPot = atoi(notifyMsgVector[notifyMsgVector.size()-2].substr(notifyMsgVector[notifyMsgVector.size()-2].find

(": ")+2).c_str());
	for(unsigned int i=1; i < notifyMsgVector.size()-2; ++i)
	{
		vector<string> singleInquireMsgVector;
		split(notifyMsgVector[i].c_str(), " ", singleInquireMsgVector);
		gPlayerInfo[singleInquireMsgVector[0]].m_nJetton = atoi(singleInquireMsgVector[1].c_str());
		gPlayerInfo[singleInquireMsgVector[0]].m_nMoney = atoi(singleInquireMsgVector[2].c_str());
		gPlayerInfo[singleInquireMsgVector[0]].m_gBet=atoi(singleInquireMsgVector[3].c_str());
		gPlayerInfo[singleInquireMsgVector[0]].m_gAction=singleInquireMsgVector[4];
	}

	/*cout << "get notify msgs :" << endl;
	for(map<string, struct CPlayerInfoMsg>::iterator itr = gPlayerInfo.begin(); itr != gPlayerInfo.end(); ++itr)
	{
		cout << itr->first << " : " << endl;
		cout << "m_nJetton = " << itr->second.m_nJetton << " m_nMoney = " << itr->second.m_nMoney << " m_nStatus = " << itr->second.m_nStatus << endl;
		cout << "its action is :";
		for(vector<string>::iterator itrStr = itr->second.m_gAction.begin(); itrStr != itr->second.m_gAction.end(); ++itrStr)
			cout << *itrStr << " ";
		cout << endl << "its m_gBet is : ";
		for(vector<int>::iterator itrStr = itr->second.m_gBet.begin(); itrStr != itr->second.m_gBet.end(); ++itrStr)
			cout << *itrStr << " ";
		cout << endl;
	}*/
	return 0;
}
int CTcpClient::OnRecvShowDownMsg(map<string, struct CPlayerInfoMsg> &gPlayerInfo, char *pBuffer)
{
	int nLen = 0, nTotalLength = 0;
	char gBuffer[MAX_SIZE];
	vector<string> inquireMsgVector;
	string cLine;
	memset(gBuffer, 0, sizeof(gBuffer));
	nTotalLength = strlen(pBuffer);
	
	while(string(pBuffer).find("/showdown \n") == string::npos)
	{
		nLen = __ClientRead(&pBuffer[nTotalLength], MAX_SIZE);
		nTotalLength += nLen;
	}

	nLen = string(pBuffer).find("/showdown \n");
	int nTailLength = strlen("/showdown \n");
	strncpy(gBuffer, pBuffer, nLen+nTailLength);
	gBuffer[nLen+nTailLength] = 0;

	int nRemainLength = strlen(pBuffer) - nLen - nTailLength;
	for(int i=0; i<nRemainLength; ++i)
		pBuffer[i] = pBuffer[nLen+nTailLength+i];
	memset(pBuffer+nRemainLength, 0, MAX_SIZE - nRemainLength);
	
	
	return 0;
}
int CTcpClient::OnRecvPotWinMsg(map<string, struct CPlayerInfoMsg> &gPlayerInfo, char *pBuffer)
{
	int nLen = 0, nTotalLength = 0;
	char gBuffer[MAX_SIZE];
	vector<string> inquireMsgVector;
	string cLine;
	memset(gBuffer, 0, sizeof(gBuffer));
	nTotalLength = strlen(pBuffer);
	
	while(string(pBuffer).find("/pot-win \n") == string::npos)
	{
		nLen = __ClientRead(&pBuffer[nTotalLength], MAX_SIZE);
		nTotalLength += nLen;
	}

	nLen = string(pBuffer).find("/pot-win \n");
	int nTailLength = strlen("/pot-win \n");
	strncpy(gBuffer, pBuffer, nLen+nTailLength);
	gBuffer[nLen+nTailLength] = 0;

	int nRemainLength = strlen(pBuffer) - nLen - nTailLength;
	for(int i=0; i<nRemainLength; ++i)
		pBuffer[i] = pBuffer[nLen+nTailLength+i];
	memset(pBuffer+nRemainLength, 0, MAX_SIZE - nRemainLength);
	
	
	return 0;

}
int CTcpClient::OnAssessHandCardType(void)//判断手牌两张牌的类型
{
	int g_nHandCardType = -1;
	int nMaxPoint=max(g_iMyCard.card[0].m_nPoint,g_iMyCard.card[1].m_nPoint);
	int nMinPoint=min(g_iMyCard.card[0].m_nPoint,g_iMyCard.card[1].m_nPoint);
	if(g_iMyCard.card[0].m_nPoint==g_iMyCard.card[1].m_nPoint)
		g_nHandCardType= ONE_PAIR;//一对
	else if((g_iMyCard.card[0].m_nColor==g_iMyCard.card[1].m_nColor)&&((nMaxPoint-nMinPoint==1)||(nMaxPoint-nMinPoint==12)))
		g_nHandCardType= STRAIGHT_FLUSH;//同花顺
	else if((g_iMyCard.card[0].m_nColor==g_iMyCard.card[1].m_nColor)&&(nMaxPoint-nMinPoint<=3))
		g_nHandCardType =TWO_PAIR;//可能成为同花顺的同花
	else if((g_iMyCard.card[0].m_nColor==g_iMyCard.card[1].m_nColor))
		g_nHandCardType= FLUSH;//同花
	else if((nMaxPoint-nMinPoint<=4)||(nMaxPoint-nMinPoint==12)||(nMaxPoint==14&&nMinPoint<=5))
		g_nHandCardType =STRAIGHT;//顺子或者可能成为顺子
	else g_nHandCardType =HIGH_CARD;//高牌
	return g_nHandCardType;
}
/*int CTcpClient::OnAssessHandCardLevel(void)
{
	int nLevel;//评估等级
	int nMaxPoint=max(g_iMyCard.card[0].m_nPoint,g_iMyCard.card[1].m_nPoint);
	int nMinPoint=min(g_iMyCard.card[0].m_nPoint,g_iMyCard.card[1].m_nPoint);
	if(g_iMyCard.card[0].m_nPoint>=12)
		nLevel = 1;
	else if(g_iMyCard.card[0].m_nPoint>=10)
		nLevel = 2;
	else if(g_iMyCard.card[0].m_nPoint>=8)
		nLevel = 3;
	else if(g_iMyCard.card[0].m_nPoint>=6)
		nLevel = 4;
	else if(g_iMyCard.card[0].m_nPoint>=2)
		nLevel = 5;
	else 
		nLevel = 6;
	return nLevel;
				
}*/
	
int CTcpClient::OnAssessHandCardLevel(void)
{
	int nLevel;//评估等级
	int nMaxPoint=max(g_iMyCard.card[0].m_nPoint,g_iMyCard.card[1].m_nPoint);
	int nMinPoint=min(g_iMyCard.card[0].m_nPoint,g_iMyCard.card[1].m_nPoint);
	
	if(g_nCurrentAlivePlayerNum>6)
	{
		if(g_iMyCard.card[0].m_nPoint==g_iMyCard.card[1].m_nPoint)
		{	
			if(g_iMyCard.card[0].m_nPoint>=13)
				nLevel = 1;
			else if(g_iMyCard.card[0].m_nPoint>=11)
				nLevel = 2;
			else if(g_iMyCard.card[0].m_nPoint==10)
				nLevel = 3;
			else if(g_iMyCard.card[0].m_nPoint==9)
				nLevel = 4;
			else if(g_iMyCard.card[0].m_nPoint==8)
				nLevel = 5;
			else if(g_iMyCard.card[0].m_nPoint==7)
				nLevel = 6;
			else nLevel = 10;
		}
		else if(g_iMyCard.card[0].m_nColor==g_iMyCard.card[1].m_nColor)
		{
			if(nMaxPoint==14&&nMinPoint>=12)
				nLevel = 1;
			else if((nMaxPoint==14&&nMinPoint>=10)||(nMaxPoint==13&&nMinPoint==12))
				nLevel = 2;
			else if((nMaxPoint==13&&nMinPoint>=10)||(nMaxPoint==12&&nMinPoint==11))
				nLevel =3;
			else if((nMaxPoint==14&&nMinPoint>=8)||(nMaxPoint==12&&nMinPoint==10))
				nLevel =4;
			else if((nMaxPoint==14)||(nMaxPoint>=12&&nMinPoint>=8))
				nLevel =5;
			else if((nMaxPoint>=12)||(nMaxPoint==11&&nMinPoint==9))
				nLevel =6;
			else nLevel = 10;
		}
		else 
		{
			if(nMaxPoint==14&&nMinPoint>=12)
				nLevel = 1;
			else if((nMaxPoint==14&&nMinPoint>=10)||(nMaxPoint==13&&nMinPoint==12))
				nLevel = 2;
			else if((nMaxPoint==14)||(nMaxPoint==13&&nMinPoint>=11)||(nMaxPoint==12&&nMinPoint==11))
				nLevel = 3;
			else if((nMaxPoint==13)||(nMaxPoint==12&&nMinPoint>=10)||(nMaxPoint==11&&nMinPoint==10))
				nLevel = 4;
			else if((nMaxPoint==12)||(nMaxPoint==11&&nMinPoint>=9)||(nMaxPoint==10&&nMinPoint==9))
				nLevel = 5;
			else if((nMaxPoint==11)||(nMaxPoint==10&&nMinPoint>=8)||(nMaxPoint==9&&nMinPoint==8))
				nLevel = 6;
			else nLevel =10;
		}

	}
	else if(g_nCurrentAlivePlayerNum>4)
	{
		if(g_iMyCard.card[0].m_nPoint==g_iMyCard.card[1].m_nPoint)
		{	
			if(g_iMyCard.card[0].m_nPoint>=12)
				nLevel = 1;
			else if(g_iMyCard.card[0].m_nPoint>=10)
				nLevel = 2;
			else if(g_iMyCard.card[0].m_nPoint==9)
				nLevel = 3;
			else if(g_iMyCard.card[0].m_nPoint==8)
				nLevel = 4;
			else if(g_iMyCard.card[0].m_nPoint==7)
				nLevel = 5;
			else if(g_iMyCard.card[0].m_nPoint==6)
				nLevel = 6;
			else nLevel = 10;
		}
		else if(g_iMyCard.card[0].m_nColor==g_iMyCard.card[1].m_nColor)
		{
			if(nMaxPoint==14&&nMinPoint>=11)
				nLevel = 1;
			else if((nMaxPoint==14&&nMinPoint>=10)||(nMaxPoint==13&&nMinPoint==12))
				nLevel = 2;
			else if((nMaxPoint==13&&nMinPoint>=10)||(nMaxPoint==12&&nMinPoint==11))
				nLevel =2;
			else if((nMaxPoint==14&&nMinPoint>=8)||(nMaxPoint==12&&nMinPoint==10))
				nLevel =3;
			else if((nMaxPoint==14)||(nMaxPoint>=12&&nMinPoint>=8))
				nLevel =4;
			else if((nMaxPoint>=12)||(nMaxPoint==11&&nMinPoint==9))
				nLevel =5;
			else if((nMaxPoint>=11)||(nMaxPoint==10&&nMinPoint==8))
				nLevel =6;
			else nLevel = 10;
		}
		else 
		{
			if(nMaxPoint==14&&nMinPoint>=12)
				nLevel = 1;
			else if((nMaxPoint==14&&nMinPoint>=10)||(nMaxPoint==13&&nMinPoint==12))
				nLevel = 2;
			else if((nMaxPoint==14)||(nMaxPoint==13&&nMinPoint>=10)||(nMaxPoint==12&&nMinPoint==11))
				nLevel = 2;
			else if((nMaxPoint==13)||(nMaxPoint==12&&nMinPoint>=9)||(nMaxPoint==11&&nMinPoint==10))
				nLevel = 3;
			else if((nMaxPoint==12)||(nMaxPoint==11&&nMinPoint>=8)||(nMaxPoint==10&&nMinPoint==9))
				nLevel = 4;
			else if((nMaxPoint==11)||(nMaxPoint==10&&nMinPoint>=7)||(nMaxPoint==9&&nMinPoint==8))
				nLevel = 5;
			else if((nMaxPoint==10)||(nMaxPoint==9&&nMinPoint>=6)||(nMaxPoint==8&&nMinPoint==7))
				nLevel = 6;
			else nLevel =10;
		}
	}
	else if(g_nCurrentAlivePlayerNum>2)
	{
		if(g_iMyCard.card[0].m_nPoint==g_iMyCard.card[1].m_nPoint)
		{	
			if(g_iMyCard.card[0].m_nPoint>=12)
				nLevel = 1;
			else if(g_iMyCard.card[0].m_nPoint>=10)
				nLevel = 2;
			else if(g_iMyCard.card[0].m_nPoint==9)
				nLevel = 3;
			else if(g_iMyCard.card[0].m_nPoint==8)
				nLevel = 4;
			else if(g_iMyCard.card[0].m_nPoint==7)
				nLevel = 5;
			else if(g_iMyCard.card[0].m_nPoint>=4)
				nLevel = 6;
			else nLevel = 10;
		}
		else if(g_iMyCard.card[0].m_nColor==g_iMyCard.card[1].m_nColor)
		{
			if(nMaxPoint==14&&nMinPoint>=11)
				nLevel = 1;
			else if((nMaxPoint==14&&nMinPoint>=10)||(nMaxPoint==13&&nMinPoint==12))
				nLevel = 2;
			else if((nMaxPoint==13&&nMinPoint>=10)||(nMaxPoint==12&&nMinPoint==11))
				nLevel =2;
			else if((nMaxPoint==14&&nMinPoint>=8)||(nMaxPoint==12&&nMinPoint==10))
				nLevel =3;
			else if((nMaxPoint==14)||(nMaxPoint>=12&&nMinPoint>=8))
				nLevel =4;
			else if((nMaxPoint>=12)||(nMaxPoint==11&&nMinPoint==9))
				nLevel =5;
			else if((nMaxPoint>=10)||(nMaxPoint==9&&nMinPoint==8))
				nLevel =6;
			else nLevel = 10;
		}
		else 
		{
			if(nMaxPoint==14&&nMinPoint>=12)
				nLevel = 1;
			else if((nMaxPoint==14&&nMinPoint>=10)||(nMaxPoint==13&&nMinPoint==12))
				nLevel = 2;
			else if((nMaxPoint==14)||(nMaxPoint==13&&nMinPoint>=9)||(nMaxPoint==12&&nMinPoint==11))
				nLevel = 2;
			else if((nMaxPoint==13)||(nMaxPoint==12&&nMinPoint>=8)||(nMaxPoint==11&&nMinPoint==10))
				nLevel = 3;
			else if((nMaxPoint==12)||(nMaxPoint==11&&nMinPoint>=7)||(nMaxPoint==10&&nMinPoint==9))
				nLevel = 4;
			else if((nMaxPoint==11)||(nMaxPoint==10&&nMinPoint>=6)||(nMaxPoint==9&&nMinPoint==8))
				nLevel = 5;
			else if((nMaxPoint==10)||(nMaxPoint==9&&nMinPoint>=5)||(nMaxPoint==8&&nMinPoint==7))
				nLevel = 6;
			else nLevel =10;
		}
		
	}
	else
		{
			if(g_iMyCard.card[0].m_nPoint==g_iMyCard.card[1].m_nPoint)
		{	
			if(g_iMyCard.card[0].m_nPoint>=12)
				nLevel = 1;
			else if(g_iMyCard.card[0].m_nPoint>=10)
				nLevel = 2;
			else if(g_iMyCard.card[0].m_nPoint>=8)
				nLevel = 3;
			else if(g_iMyCard.card[0].m_nPoint>=6)
				nLevel = 4;
			else if(g_iMyCard.card[0].m_nPoint>=4)
				nLevel = 5;
			else 
				nLevel = 6;
			
		}
		else if(g_iMyCard.card[0].m_nColor==g_iMyCard.card[1].m_nColor)
		{
			if(nMaxPoint==14&&nMinPoint>=12)
				nLevel = 1;
			else if((nMaxPoint==14&&nMinPoint>=10)||(nMaxPoint==13&&nMinPoint==12))
				nLevel = 2;
			else if((nMaxPoint==14)||(nMaxPoint==13&&nMinPoint>=9)||(nMaxPoint==12&&nMinPoint==11))
				nLevel =3;
			else if((nMaxPoint==13)||(nMaxPoint==12&&nMinPoint>=8)||(nMaxPoint==11&&nMinPoint==10))
				nLevel =4;
			else if((nMaxPoint==12)||(nMaxPoint==11&&nMinPoint>=7)||(nMaxPoint>=12&&nMinPoint>=8))
				nLevel =5;
			else 
				nLevel =6;
			
		}
		else 
		{
			if(nMaxPoint==14&&nMinPoint>=12)
				nLevel = 1;
			else if((nMaxPoint==14&&nMinPoint>=10)||(nMaxPoint==13&&nMinPoint==12))
				nLevel = 2;
			else if((nMaxPoint==14)||(nMaxPoint==13&&nMinPoint>=9)||(nMaxPoint==12&&nMinPoint==11))
				nLevel = 3;
			else if((nMaxPoint==13)||(nMaxPoint==12&&nMinPoint>=8)||(nMaxPoint==11&&nMinPoint==10))
				nLevel = 4;
			else if((nMaxPoint>=11)||(nMaxPoint==10&&nMinPoint>=7)||(nMaxPoint==9&&nMinPoint==8))
				nLevel = 5;
			else if((nMaxPoint-nMinPoint<=5))
				nLevel = 6;
			
		}
		
		}
	return nLevel;

}

long CTcpClient::OnAssessCardAfterFlop(void)
{
	int nLevel;//评估等级
	struct CCardInfo gFiveCard[5];
	for(int i=0;i<=1;i++)
	{
		gFiveCard[i].m_nColor = g_iMyCard.card[i].m_nColor;
		gFiveCard[i].m_nPoint = g_iMyCard.card[i].m_nPoint;
	}
	for(int i=0;i<=2;i++)
	{
		gFiveCard[i+2].m_nColor = g_iPublicCard.card[i].m_nColor;
		gFiveCard[i+2].m_nPoint = g_iPublicCard.card[i].m_nPoint;
	}
	
	nLevel = countFiveCardsLevel(gFiveCard,5);
	sortFiveCards(gFiveCard,nLevel);
	long lCardsValue = countFiveCardsValue(gFiveCard,5,nLevel);
	return lCardsValue;

}
long CTcpClient::OnAssessCardAfterTurn(void)
{
	const int nCardNum =6;
	struct CCardInfo gSixCard[nCardNum];
	int nLevel;//评估等级
	long lCardsValue=0L;
	for(int i=0;i<=1;i++)
	{
		gSixCard[i].m_nColor = g_iMyCard.card[i].m_nColor;
		gSixCard[i].m_nPoint = g_iMyCard.card[i].m_nPoint;
	}
	for(int i=0;i<=3;i++)
	{
		gSixCard[i+2].m_nColor = g_iPublicCard.card[i].m_nColor;
		gSixCard[i+2].m_nPoint = g_iPublicCard.card[i].m_nPoint;
	}
	
	struct CCardInfo iPoker[5];
        for (int a = 0; a < nCardNum-4; a++)
        {
            for (int b = a + 1; b < nCardNum-3; b++)
            {
                for (int c = b + 1; c < nCardNum-2; c++)
                {
                    for (int d = c + 1; d < nCardNum-1; d++)
                    {
                        for (int e = d + 1; e < nCardNum; e++)
                        {
                         							
							iPoker[0].m_nColor=gSixCard[a].m_nColor;
							iPoker[0].m_nPoint=gSixCard[a].m_nPoint;
							iPoker[1].m_nColor=gSixCard[b].m_nColor;
							iPoker[1].m_nPoint=gSixCard[b].m_nPoint;
							iPoker[2].m_nColor=gSixCard[c].m_nColor;
							iPoker[2].m_nPoint=gSixCard[c].m_nPoint;
							iPoker[3].m_nColor=gSixCard[d].m_nColor;
							iPoker[3].m_nPoint=gSixCard[d].m_nPoint;
							iPoker[4].m_nColor=gSixCard[e].m_nColor;
							iPoker[4].m_nPoint=gSixCard[e].m_nPoint;
							//cout<<"the poker:"<<iPoker[0].m_nPoint<<" "<<iPoker[1].m_nPoint<<" "<<iPoker[2].m_nPoint<<" "<<iPoker[3].m_nPoint<<" "<<iPoker[4].m_nPoint<<endl;
							nLevel = countFiveCardsLevel(iPoker,5);
							//cout<<"turn the level:"<<nLevel<<endl;
							sortFiveCards(iPoker,nLevel);
							long lSixCardValue=countFiveCardsValue(iPoker,5,nLevel);
							//cout<<"turn the lSixCardValue:"<<lSixCardValue<<endl;
							if(lCardsValue <lSixCardValue)
								lCardsValue=lSixCardValue;
                        }
                    }
                }
            }
       
		}
// cout<<"the turnround max value"<<lCardsValue<<endl;
 return lCardsValue;
}
long CTcpClient::OnAssessCardAfterRiver(void)
{
	const int nCardNum =7;
	struct CCardInfo gSevenCard[nCardNum];
	int nLevel;//评估等级
	long lCardsValue=0L;
	for(int i=0;i<=1;i++)
	{
		gSevenCard[i].m_nColor = g_iMyCard.card[i].m_nColor;
		gSevenCard[i].m_nPoint = g_iMyCard.card[i].m_nPoint;
	}
	for(int i=0;i<=4;i++)
	{
		gSevenCard[i+2].m_nColor = g_iPublicCard.card[i].m_nColor;
		gSevenCard[i+2].m_nPoint = g_iPublicCard.card[i].m_nPoint;
	}
	
	struct CCardInfo iPoker[5];
        for (int a = 0; a < nCardNum-4; a++)
        {
            for (int b = a + 1; b < nCardNum-3; b++)
            {
                for (int c = b + 1; c < nCardNum-2; c++)
                {
                    for (int d = c + 1; d < nCardNum-1; d++)
                    {
                        for (int e = d + 1; e < nCardNum; e++)
                        {
                         							
							iPoker[0].m_nColor=gSevenCard[a].m_nColor;
							iPoker[0].m_nPoint=gSevenCard[a].m_nPoint;
							iPoker[1].m_nColor=gSevenCard[b].m_nColor;
							iPoker[1].m_nPoint=gSevenCard[b].m_nPoint;
							iPoker[2].m_nColor=gSevenCard[c].m_nColor;
							iPoker[2].m_nPoint=gSevenCard[c].m_nPoint;
							iPoker[3].m_nColor=gSevenCard[d].m_nColor;
							iPoker[3].m_nPoint=gSevenCard[d].m_nPoint;
							iPoker[4].m_nColor=gSevenCard[e].m_nColor;
							iPoker[4].m_nPoint=gSevenCard[e].m_nPoint;
							//cout<<"the river poker:"<<iPoker[0].m_nPoint<<" "<<iPoker[1].m_nPoint<<" "<<iPoker[2].m_nPoint<<" "<<iPoker[3].m_nPoint<<" "<<iPoker[4].m_nPoint<<endl;
							nLevel = countFiveCardsLevel(iPoker,5);
							//cout<<"river the level:"<<nLevel<<endl;
							sortFiveCards(iPoker,nLevel);
							long lSevenCardValue=countFiveCardsValue(iPoker,5,nLevel);
							//cout<<"river the lSevenCardValue:"<<lSevenCardValue<<endl;
							if( lCardsValue <lSevenCardValue)
								lCardsValue=lSevenCardValue;
                        }
                    }
                }
            }
       
		}
 return lCardsValue;
}
int CTcpClient::OnDecideAction(void)
{
	string cActionMsg;
	int nRaiseBet;
	g_nMyOrder = g_gPlayerMap[g_cMyId].m_nOrder;
	int nMyTotalBet=g_gPlayerMap[g_cMyId].m_nJetton+g_gPlayerMap[g_cMyId].m_nMoney;
	int nMyCurrentLeafJetton=g_gPlayerMap[g_cMyId].m_nJetton;
	int nUnfoldPlayerNum=g_nCurrentAlivePlayerNum-g_nCurrentRoundFoldNum;
	long lLon=1000000000L*10;
	int nMaxPoint=max(g_iMyCard.card[0].m_nPoint,g_iMyCard.card[0].m_nPoint);
	int nMinPoint=min(g_iMyCard.card[0].m_nPoint,g_iMyCard.card[0].m_nPoint);

	//cout<<"g_nCurrentAlivePlayerNum:"<<g_nCurrentAlivePlayerNum<<"g_nCurrentRoundFoldNum"<<g_nCurrentRoundFoldNum<<"nUnfoldPlayerNum:"<<nUnfoldPlayerNum<<endl;
	//if((nMyTotalBet<g_nJetton)||(nMyTotalBet>3*(g_nJetton+g_nMoney)))//
	if(nMyTotalBet<4*g_nSmallBlindBet)
	{
		cActionMsg ="all_in \n";
	}
	else if(nUnfoldPlayerNum==1)//只剩我一人时
	{
		cActionMsg ="call \n";
	}	
	else if(g_nCurrentRoundCheckNum==nUnfoldPlayerNum-1)//其他人都让牌时不要弃牌
	{
		cActionMsg ="check \n";
	}
	else 
	{
			if(g_nRound == 1)
			{
				if(g_nGameCount<0)//小于100局
				{
					switch(g_nHandCardType)
					{
						case ONE_PAIR:
							{
								if(g_iMyCard.card[0].m_nPoint>=12)
								{
									if(g_gBetNum[g_nRound-1]==1)
									{
											nRaiseBet = g_nSmallBlindBet;
											stringstream sStream;
											sStream>>nRaiseBet;
											string cRaiseBet = sStream.str();
											cActionMsg = "raise "+cRaiseBet+" "+"\n";
									}
									else 
									{
											
										cActionMsg ="call \n";
									}
								}
								else 
									cActionMsg ="fold \n";
							}
							break;
						case STRAIGHT_FLUSH:
						case FLUSH:
							{
								if(nMinPoint>=12)
									cActionMsg ="check \n";
								else 
									cActionMsg ="fold \n";
							}					

						default:
							cActionMsg ="fold \n";
							break;
					}
					
				
				}
				else
				{
				
							switch(g_nHandCardType)
								{
									case ONE_PAIR:
										{
											if(g_nHandCardLevel<=3)
											{
												if(g_gBetNum[g_nRound-1]==1)
													{
														nRaiseBet = g_nCurrentRoundNeedRaise;
														stringstream sStream;
														sStream>>nRaiseBet;
														string cRaiseBet = sStream.str();
														cActionMsg = "raise "+cRaiseBet+" "+"\n";
													}
												else 
												{
											
													cActionMsg ="call \n";
												}
											}
											else if(g_nHandCardLevel<=6)
											{
													if(g_gBetNum[g_nRound-1]==1)
													{
														cActionMsg ="call \n";
													}
													else 
													{
														if(g_nCurrentRoundAllInNum==0)
														{
															if(g_nCurrentRoundRaiseNum==0)
																cActionMsg ="check \n";
															else
															{
																if(g_nHandCardLevel<=5)
																	cActionMsg ="check \n";
																else cActionMsg ="fold \n";
															}
														}
													
														else cActionMsg ="fold \n";
													}
											}
											else 
											{
												if(g_gBetNum[g_nRound-1]==1)
													{
														if(g_nCurrentRoundAllInNum==0)
															cActionMsg ="check \n";
														else cActionMsg ="fold \n";
													}
												else 
												{
													if(g_nCurrentRoundAllInNum>0||g_nCurrentRoundRaiseNum>0||nUnfoldPlayerNum>6)
														cActionMsg ="fold \n";
													
													else cActionMsg ="check \n";
											
												}
										
											}
											break;
										}
									case STRAIGHT_FLUSH:
									case TWO_PAIR:
									case FLUSH:
										{
											if(g_nHandCardLevel<=2)
											{
		
													cActionMsg ="call \n";
											}
											else if(g_nHandCardLevel<=6)
											{
													if(g_gBetNum[g_nRound-1]==1)
													{
														cActionMsg ="call \n";
													}
													else 
													{
														if(g_nCurrentRoundAllInNum==0)
														{
															if(g_nCurrentRoundRaiseNum==0)
																cActionMsg ="check \n";
															else
															{
																if(g_nHandCardLevel<=5)
																	cActionMsg ="check \n";
																else cActionMsg ="fold \n";
															}
														}
													
														else cActionMsg ="fold \n";
													}
											}
											else 
											{
												if(g_gBetNum[g_nRound-1]==1)
													{
														if(g_nCurrentRoundAllInNum==0)
															cActionMsg ="check \n";
														else cActionMsg ="fold \n";
													}
												else 
												{
													if(g_nCurrentRoundAllInNum>0||g_nCurrentRoundRaiseNum>0||nUnfoldPlayerNum>6)
														cActionMsg ="fold \n";
													
													else cActionMsg ="check \n";
											
												}
										
											}
											break;
										}
			
									case STRAIGHT:
										{
											if(g_nHandCardLevel<=2)
											{		
													cActionMsg ="call \n";
											}
											else if(g_nHandCardLevel<=6)
											{
													if(g_gBetNum[g_nRound-1]==1)
													{
														cActionMsg ="call \n";
													}
													else 
													{
														if(g_nCurrentRoundAllInNum==0)
														{
															if(g_nCurrentRoundRaiseNum==0)
																cActionMsg ="check \n";
															else
															{
																if(g_nHandCardLevel<=4&&nUnfoldPlayerNum<=4)
																	cActionMsg ="check \n";
																else cActionMsg ="fold \n";
															}
														}
													
														else cActionMsg ="fold \n";
													}
											}
											else 
											{

														cActionMsg ="fold \n";
											}
											break;
										}
									case HIGH_CARD:
										{
											 if(g_nHandCardLevel<=3)
											{
													if(g_gBetNum[g_nRound-1]==1)
													{
														cActionMsg ="call \n";
													}
													else 
													{
														if(g_nCurrentRoundAllInNum==0)
														{
															if(g_nCurrentRoundRaiseNum==0)
																cActionMsg ="check \n";
															else
															{
																if(g_nHandCardLevel<=2&&nUnfoldPlayerNum<=4)
																	cActionMsg ="check \n";
																else cActionMsg ="fold \n";
															}
														}
													
														else cActionMsg ="fold \n";
													}
											}
											else 
											{

														cActionMsg ="fold \n";
											}
											break;
										}
									default:
										cActionMsg ="check \n";	
										break;

						}
					}
			}
			else if(g_nRound == 2)
			{
				//cout<<"Action--g_nCardAfterFlopValue:"<<g_nCardAfterFlopValue<<endl;
				long lLon=1000000000L*10;
				int nCardAfterFlopLevel=g_nCardAfterFlopValue/lLon;
				//cout<<"the level:"<<nCardAfterFlopLevel<<endl;
				switch(nCardAfterFlopLevel)
				{
					case ROYAL_STRAIGHT_FLUSH:		
					case STRAIGHT_FLUSH:
					case FOUR_OF_A_KIND:
					case FULL_HOUSE:
					case FLUSH:
						{
							if(g_gBetNum[g_nRound-1]<=3)
							{
								nRaiseBet = g_nCurrentRoundNeedRaise+g_nSmallBlindBet;
								stringstream sStream;
								sStream>>nRaiseBet;
								string cRaiseBet = sStream.str();
								cActionMsg = "raise "+cRaiseBet+" "+"\n";
							}
							else cActionMsg ="call \n";
							break;

						}
					case STRAIGHT:
						{
							if(g_nHandCardLevel<=2)
								{
									if(g_gBetNum[g_nRound-1]<=2)
									{
										nRaiseBet = g_nCurrentRoundNeedRaise+g_nSmallBlindBet;
										stringstream sStream;
										sStream>>nRaiseBet;
										string cRaiseBet = sStream.str();
										cActionMsg = "raise "+cRaiseBet+" "+"\n";
									}
									else cActionMsg ="call \n";
								}
							else {
									if(g_gBetNum[g_nRound-1]<=2)
									{
										nRaiseBet =g_nCurrentRoundNeedRaise;
										stringstream sStream;
										sStream>>nRaiseBet;
										string cRaiseBet = sStream.str();
										cActionMsg = "raise "+cRaiseBet+" "+"\n";
									}
									else cActionMsg ="call \n";
								}
							break;
						}
					case THREE_OF_A_KIND:
						{
								if(g_nHandCardType==ONE_PAIR)							
									{
										if(g_gBetNum[g_nRound-1]==1)
										{
											nRaiseBet = g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet;
											stringstream sStream;
											sStream>>nRaiseBet;
											string cRaiseBet = sStream.str();
											cActionMsg = "raise "+cRaiseBet+" "+"\n";
										}
										else cActionMsg = "call \n";
									}
								else if(g_nPublicCardAfterFlopLevel==THREE_OF_A_KIND&&g_nHandCardLevel>=2) 
									cActionMsg = "fold \n";
								else if(g_nPublicCardAfterFlopLevel==THREE_OF_A_KIND&&g_nHandCardLevel==1) 
									cActionMsg = "check \n";
								else
									{
										if(g_gBetNum[g_nRound-1]==1)
										{
											nRaiseBet = 4*g_nCurrentRoundNeedRaise;
											stringstream sStream;
											sStream>>nRaiseBet;
											string cRaiseBet = sStream.str();
											cActionMsg = "raise "+cRaiseBet+" "+"\n";
										}
										else cActionMsg = "call \n";
									}
							break;
						}
					case TWO_PAIR:
						{
							switch(g_nPublicCardAfterFlopLevel)
							{
								case STRAIGHT_FLUSH:
								case FLUSH:
								case STRAIGHT:
									{
										if(g_nCurrentRoundAllInNum>0||g_nCurrentRoundRaiseNum>0)
											cActionMsg="fold \n";
										else
											{
												if(g_cLastAction=="check"&&g_nCardAfterFlopValue>=121212101002)
												{
													nRaiseBet = g_nCurrentRoundNeedRaise;
													stringstream sStream;
													sStream>>nRaiseBet;
													string cRaiseBet = sStream.str();
													cActionMsg = "raise "+cRaiseBet+" "+"\n";
												}
												else cActionMsg = "call \n";

											}
									}
								break;
								case ONE_PAIR:
									{
											if(g_cLastAction=="check")
											{
												cActionMsg = "check \n";

											}
											
											else if(g_nCurrentRoundAllInNum>0)
												cActionMsg = "fold \n";
											else if(g_nCurrentRoundRaiseNum>0)//有人加注的情况下
											{
												if((g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet)>=max(nMyCurrentLeafJetton/4,2*g_nSmallBlindBet))//加注额度比较大时弃牌
													cActionMsg= "fold \n";
												else if(nUnfoldPlayerNum>4&&g_nMyOrder>g_nCurrentAlivePlayerNum-3)//位置靠后且前面玩家未弃牌个数比较多时弃牌
														cActionMsg= "fold \n";
												else if(g_nHandCardType==HIGH_CARD&&nMaxPoint<10)//手牌为单牌时且点数比较小时
													cActionMsg= "fold \n";
												else
												{
													if(g_gBetNum[g_nRound-1]<=2)//最多跟注两次
													cActionMsg= "call \n";
													else cActionMsg= "fold \n";
												}
												
											}
											else cActionMsg = "call \n";
										
									}
									break;
								case HIGH_CARD:
									{
											if(g_cLastAction=="check")
											{
												cActionMsg = "check \n";

											}
											
											else if(g_nCurrentRoundAllInNum>0)
												cActionMsg = "fold \n";
											else if(g_nCurrentRoundRaiseNum>0)//有人加注的情况下
											{
												if((g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet)>=max(nMyCurrentLeafJetton/4,2*g_nSmallBlindBet))//加注额度比较大时弃牌
													cActionMsg= "fold \n";
												else if(nUnfoldPlayerNum>4&&g_nMyOrder>g_nCurrentAlivePlayerNum-3)//位置靠后且前面玩家未弃牌个数比较多时弃牌
														cActionMsg= "fold \n";
												else if(g_nHandCardType==HIGH_CARD&&nMaxPoint<10)//手牌为单牌时且点数比较小时
													cActionMsg= "fold \n";
												else
												{
													if(g_gBetNum[g_nRound-1]<=2)//最多跟注两次
													{
														if(isCurrentPublicCardsAbleBeStraight(3)||isCurrentPublicCardsAbleBeFlush(3))//如果公牌可能出现顺子或者同花则弃牌
															cActionMsg= "fold \n";
														else cActionMsg= "call \n";
													}
													else cActionMsg= "fold \n";
												}
												
											}
											else cActionMsg = "call \n";
										
									}
									break;
								default:
									cActionMsg = "fold \n";
									break;

							}
						break;
					}
					case ONE_PAIR:
						{
							switch(g_nPublicCardAfterFlopLevel)
							{
								case STRAIGHT_FLUSH:
								case FLUSH:
								case STRAIGHT:
									{
										if(g_nCurrentRoundAllInNum>0||g_nCurrentRoundRaiseNum>0)
											cActionMsg="fold \n";
										else
											 cActionMsg = "call \n";
		
									}
								break;
								case ONE_PAIR:
									{
											if(g_cLastAction=="check")
											{
												cActionMsg = "check \n";

											}
											
											else if(g_nCurrentRoundAllInNum>0||g_nCurrentRoundRaiseNum>0)
												cActionMsg = "fold \n";
											
											else cActionMsg = "call \n";
										
									}
									break;
								case HIGH_CARD:
									{
											if(g_cLastAction=="check")
											{
												cActionMsg = "check \n";

											}
											
											else if(g_nCurrentRoundAllInNum>0)
												cActionMsg = "fold \n";
											else if(g_nCurrentRoundRaiseNum>0)//有人加注的情况下
											{
												if((g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet)>=max(nMyCurrentLeafJetton/4,2*g_nSmallBlindBet))//加注额度比较大时弃牌
													cActionMsg= "fold \n";
												else if(nUnfoldPlayerNum>4&&g_nMyOrder>g_nCurrentAlivePlayerNum-3)//位置靠后且前面玩家未弃牌个数比较多时弃牌
														cActionMsg= "fold \n";
												else if(g_nHandCardType==HIGH_CARD&&nMaxPoint<10)//手牌为单牌时且点数比较小时
													cActionMsg= "fold \n";
												else
												{
													if(g_gBetNum[g_nRound-1]<=2)//最多跟注两次
													{
														if(isCurrentPublicCardsAbleBeStraight(3)||isCurrentPublicCardsAbleBeFlush(3))//如果公牌可能出现顺子或者同花则弃牌
															cActionMsg= "fold \n";
														else if(g_nCardAfterFlopValue>111313140203) cActionMsg= "call \n";
														else cActionMsg= "fold \n";
													}
													else cActionMsg= "fold \n";
												}
												
											}
											else cActionMsg = "call \n";
										
									}
									break;
								default:
									cActionMsg = "check \n";
									break;
								}
							
							break;
						}
					case HIGH_CARD:
						if(isCurrentCardsAbleBeStraight(5)&&isCurrentCardsAbleBeFlush(6))//等待是否可能出现顺子
						{
							if(g_nCurrentRoundAllInNum>0||g_nCurrentRoundRaiseNum>=1)
								cActionMsg = "fold \n";
							else cActionMsg = "check \n";
						}

						else if(isCurrentCardsAbleBeStraight(5)||isCurrentCardsAbleBeFlush(5))
						{
								if(g_nCurrentRoundAllInNum>1||g_nCurrentRoundRaiseNum>=1)
								cActionMsg = "fold \n";
							else cActionMsg = "check \n";

						}
						else cActionMsg = "fold \n";
						break;
					default:
						cActionMsg = "check \n";
						break;
				}
			}
			else if(g_nRound == 3)
			{
				long lLon=1000000000L*10;
				int nCardAfterTurnLevel=g_nCardAfterTurnValue/lLon;
				//cout<<"the turn round card level:"<<nCardAfterTurnLevel<<endl;
				switch(nCardAfterTurnLevel)
				{
					case ROYAL_STRAIGHT_FLUSH:		
					case STRAIGHT_FLUSH:
					case FOUR_OF_A_KIND:
					case FULL_HOUSE:
					case FLUSH:
						{
							if(g_nPublicCardAfterTurnLevel<=FLUSH)
								{	
									if(g_gBetNum[g_nRound-1]<=3)
									{
										nRaiseBet = g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet;
										stringstream sStream;
										sStream>>nRaiseBet;
										string cRaiseBet = sStream.str();
										cActionMsg = "raise "+cRaiseBet+" "+"\n";
									}
								else cActionMsg = "call \n";
							}
						
							else 
		
									cActionMsg = "call \n";
									break;
								

						}
					case STRAIGHT:
						{
							if(g_nPublicCardAfterTurnLevel<STRAIGHT&&g_nHandCardLevel<=2)
							{
								if(g_gBetNum[g_nRound-1]==1)
								{
									nRaiseBet = g_nCurrentRoundNeedRaise+g_nSmallBlindBet;
									stringstream sStream;
									sStream>>nRaiseBet;
									string cRaiseBet = sStream.str();
									cActionMsg = "raise "+cRaiseBet+" "+"\n";
								}
								else cActionMsg = "call \n";
							}
							else if(g_nPublicCardAfterTurnLevel<STRAIGHT&&g_nHandCardLevel<=4)

							{
								if(g_gBetNum[g_nRound-1]==1)
								{
									nRaiseBet = g_nCurrentRoundNeedRaise;
									stringstream sStream;
									sStream>>nRaiseBet;
									string cRaiseBet = sStream.str();
									cActionMsg = "raise "+cRaiseBet+" "+"\n";
								}
								else cActionMsg = "call \n";
							}
							else cActionMsg = "check \n";
							break;
						}
					case THREE_OF_A_KIND:
						{
					
							if(g_nHandCardType==ONE_PAIR&&g_nPublicCardAfterTurnLevel<THREE_OF_A_KIND)							
							{
								if(g_gBetNum[g_nRound-1]==1)
								{
									nRaiseBet = g_nCurrentRoundNeedRaise+g_nSmallBlindBet;
									stringstream sStream;
									sStream>>nRaiseBet;
									string cRaiseBet = sStream.str();
									cActionMsg = "raise "+cRaiseBet+" "+"\n";
								}
								else cActionMsg = "call \n";
							}
							else if(g_nPublicCardAfterTurnLevel==THREE_OF_A_KIND&&g_nHandCardLevel>=4) 
								cActionMsg = "fold \n";
							else if(g_nPublicCardAfterTurnLevel==THREE_OF_A_KIND&&g_nHandCardLevel==1) 
								cActionMsg = "check \n";
							else
							{
										
								cActionMsg = "call \n";
							}
							break;
						}
					case TWO_PAIR:
						{
							switch(g_nPublicCardAfterTurnLevel)
							{
								case STRAIGHT_FLUSH:
								case FLUSH:
								case STRAIGHT:
									{
										if(g_nCurrentRoundAllInNum>0||g_nCurrentRoundRaiseNum>0)
											cActionMsg="fold \n";
										else
											{
												if(g_cLastAction=="check"&&g_nCardAfterTurnValue>=121212070702&&g_gBetNum[g_nRound-1]==1)//最多跟注两次)
												{
													nRaiseBet = g_nCurrentRoundNeedRaise;
													stringstream sStream;
													sStream>>nRaiseBet;
													string cRaiseBet = sStream.str();
													cActionMsg = "raise "+cRaiseBet+" "+"\n";
												}
												else cActionMsg = "call \n";

											}
									}
								break;
								case ONE_PAIR:
									{
											if(g_cLastAction=="check")
											{
												cActionMsg = "check \n";

											}
											
											else if(g_nCurrentRoundAllInNum>0)
												cActionMsg = "fold \n";
											else if(g_nCurrentRoundRaiseNum>0)//有人加注的情况下
											{
												if((g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet)>=max(nMyCurrentLeafJetton/4,2*g_nSmallBlindBet))//加注额度比较大时弃牌
													cActionMsg= "fold \n";
												else if(nUnfoldPlayerNum>4&&g_nMyOrder>g_nCurrentAlivePlayerNum-3)//位置靠后且前面玩家未弃牌个数比较多时弃牌
														cActionMsg= "fold \n";
												else if(g_nHandCardType==HIGH_CARD&&nMaxPoint<10)//手牌为单牌时且点数比较小时
													cActionMsg= "fold \n";
												else
												{
													if(g_gBetNum[g_nRound-1]<=2)//最多跟注两次
														cActionMsg= "call \n";
													else cActionMsg= "fold \n";
												}
												
											}
											else cActionMsg = "call \n";
										
									}
									break;
								case HIGH_CARD:
									{
											if(g_cLastAction=="check")
											{
												cActionMsg = "check \n";

											}
											
											else if(g_nCurrentRoundAllInNum>0)
												cActionMsg = "fold \n";
											else if(g_nCurrentRoundRaiseNum>0)//有人加注的情况下
											{
												if((g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet)>=max(nMyCurrentLeafJetton/4,2*g_nSmallBlindBet))//加注额度比较大时弃牌
													cActionMsg= "fold \n";
												else if(nUnfoldPlayerNum>4&&g_nMyOrder>g_nCurrentAlivePlayerNum-3)//位置靠后且前面玩家未弃牌个数比较多时弃牌
														cActionMsg= "fold \n";
												else if(g_nHandCardType==HIGH_CARD&&nMaxPoint<10)//手牌为单牌时且点数比较小时
													cActionMsg= "fold \n";
												else
												{
													if(g_gBetNum[g_nRound-1]<=2)//最多跟注两次
													{
														if(isCurrentPublicCardsAbleBeStraight(4)||isCurrentPublicCardsAbleBeFlush(4))//如果公牌可能出现顺子或者同花则弃牌
															cActionMsg= "fold \n";
														else cActionMsg= "call \n";
													}
													else cActionMsg= "fold \n";
												}
												
											}
											else if(g_nCardAfterTurnValue>=121212101002)
											{
												if(g_gBetNum[g_nRound-1]==1)//最多跟注两次
												{
													nRaiseBet = g_nCurrentRoundNeedRaise;
													stringstream sStream;
													sStream>>nRaiseBet;
													string cRaiseBet = sStream.str();
													cActionMsg = "raise "+cRaiseBet+" "+"\n";
												}
												else cActionMsg = "call \n";
											}
												
											else	cActionMsg = "call \n";
										
									}
									break;
								default:
									cActionMsg = "check \n";
									break;

							}
						break;
					}
					case ONE_PAIR:
						{
							switch(g_nPublicCardAfterTurnLevel)
							{
								case STRAIGHT_FLUSH:
								case FLUSH:
								case STRAIGHT:
									{
										if(g_nCurrentRoundAllInNum>0||g_nCurrentRoundRaiseNum>0)
											cActionMsg="fold \n";
										else
											 cActionMsg = "call \n";
		
									}
								break;
								case ONE_PAIR:
									{
											if(g_cLastAction=="check")
											{
												cActionMsg = "check \n";

											}
											
											else if(g_nCurrentRoundAllInNum>0||g_nCurrentRoundRaiseNum>0)
												cActionMsg = "fold \n";
											
											else cActionMsg = "call \n";
										
									}
									break;
								case HIGH_CARD:
									{
											if(g_cLastAction=="check")
											{
												cActionMsg = "check \n";

											}
											
											else if(g_nCurrentRoundAllInNum>0)
												cActionMsg = "fold \n";
											else if(g_nCurrentRoundRaiseNum>0)//有人加注的情况下
											{
												if((g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet)>=max(nMyCurrentLeafJetton/4,2*g_nSmallBlindBet))//加注额度比较大时弃牌
													cActionMsg= "fold \n";
												else if(nUnfoldPlayerNum>4&&g_nMyOrder>g_nCurrentAlivePlayerNum-3)//位置靠后且前面玩家未弃牌个数比较多时弃牌
														cActionMsg= "fold \n";
												else if(g_nHandCardType==HIGH_CARD&&nMaxPoint<10)//手牌为单牌时且点数比较小时
													cActionMsg= "fold \n";
												else
												{
													if(g_gBetNum[g_nRound-1]<=2)//最多跟注两次
													{
														if(isCurrentPublicCardsAbleBeStraight(4)||isCurrentPublicCardsAbleBeFlush(4))//如果公牌可能出现顺子或者同花则弃牌
															cActionMsg= "fold \n";
														else if(g_nCardAfterTurnValue>111313140203) cActionMsg= "call \n";
														else cActionMsg= "fold \n";
													}
													else cActionMsg= "fold \n";
												}
												
											}
											else cActionMsg = "call \n";
										
									}
									break;
								default:
									cActionMsg = "check \n";
									break;
								}
							
							break;
						}
					case HIGH_CARD:
						if(isCurrentCardsAbleBeStraight(6)&&isCurrentCardsAbleBeFlush(6))//等待是否可能出现顺子
						{
							if(g_nCurrentRoundAllInNum>0||g_nCurrentRoundRaiseNum>=1)
								cActionMsg = "fold \n";
							else cActionMsg = "call \n";
						}

						else if(isCurrentCardsAbleBeStraight(6)||isCurrentCardsAbleBeFlush(6))
						{
								if(g_nCurrentRoundAllInNum>1||g_nCurrentRoundRaiseNum>=1)
								cActionMsg = "fold \n";
							else cActionMsg = "call \n";

						}
						else cActionMsg = "fold \n";
						break;
					default:
						cActionMsg = "check \n";
						break;
					
				}
			}
			else 
			{
				long lLon=1000000000L*10;
				int nCardAfterRiverLevel=g_nCardAfterRiverValue/lLon;

				switch(nCardAfterRiverLevel)
				{
					case ROYAL_STRAIGHT_FLUSH:
								cActionMsg ="all_in \n";
								break;
					case STRAIGHT_FLUSH:
								{
										if(g_nPublicCardAfterRiverValue==g_nCardAfterRiverValue)
											cActionMsg ="check \n";
										else 										
											cActionMsg ="call \n";			
										break;
								}
					case FOUR_OF_A_KIND:
								{
										if(g_nPublicCardAfterRiverValue==g_nCardAfterRiverValue)
										{
											if(g_nHandCardLevel<=2)
												cActionMsg ="check \n";
											else cActionMsg ="fold \n";
										}
										else 
										{
											if(g_gBetNum[g_nRound-1]<=3)
											{
												nRaiseBet = g_nCurrentRoundNeedRaise+g_nSmallBlindBet;
												stringstream sStream;
												sStream>>nRaiseBet;
												string cRaiseBet = sStream.str();
												cActionMsg = "raise "+cRaiseBet+" "+"\n";
											}
											else cActionMsg = "call \n";
										}
										
										break;
									}
					case FULL_HOUSE:
						{
									if(g_nPublicCardAfterRiverValue==g_nCardAfterRiverValue)
									{
										if(g_nHandCardLevel<=2)
												cActionMsg ="check \n";
											else cActionMsg ="fold \n";
									}
									else
									{
										if(g_gBetNum[g_nRound-1]<=3)
											{
												nRaiseBet = g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet;
												stringstream sStream;
												sStream>>nRaiseBet;
												string cRaiseBet = sStream.str();
												cActionMsg = "raise "+cRaiseBet+" "+"\n";
											}
										else cActionMsg ="call \n";
									}
									
									break;
								}
					case FLUSH:
						{
							if(g_nPublicCardAfterRiverValue==g_nCardAfterRiverValue)
							{
								if(g_nHandCardLevel<=2)
												cActionMsg ="check \n";
											else cActionMsg ="fold \n";
							}		
							else 
							{
								if(g_gBetNum[g_nRound-1]<=3)
											
									{
										nRaiseBet = g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet;
										stringstream sStream;
										sStream>>nRaiseBet;
										string cRaiseBet = sStream.str();
										cActionMsg = "raise "+cRaiseBet+" "+"\n";
									}
								else cActionMsg ="call \n";
							}
							
							break;

						}
					case STRAIGHT:
						{
							if(g_nCardAfterRiverValue==141413121110)//最大的顺子
							{
								if(g_gBetNum[g_nRound-1]<=3)
											
									{
										nRaiseBet = g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet;
										stringstream sStream;
										sStream>>nRaiseBet;
										string cRaiseBet = sStream.str();
										cActionMsg = "raise "+cRaiseBet+" "+"\n";
									}
								else cActionMsg ="call \n";
							}
							else if((g_nPublicCardAfterRiverValue==g_nCardAfterRiverValue))
								{
									
									if(nUnfoldPlayerNum<=2&&g_nHandCardLevel<=4)
										cActionMsg ="check \n";
									else if(nUnfoldPlayerNum<=4&&g_nHandCardLevel<=2)
										cActionMsg ="check \n";
									else if(g_nCurrentRoundAllInNum>0||g_nHandCardLevel>6)
										cActionMsg ="fold \n";	
									else cActionMsg ="check \n";

								}
							else if(g_nHandCardLevel<=2)
								{
									if(g_gBetNum[g_nRound-1]<=3)
									{
										nRaiseBet = g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet;
										stringstream sStream;
										sStream>>nRaiseBet;
										string cRaiseBet = sStream.str();
										cActionMsg = "raise "+cRaiseBet+" "+"\n";
									}
									else cActionMsg = "call \n";
								}
							else 
									 cActionMsg = "call \n";
								
							break;
						}
					case THREE_OF_A_KIND:
						{
					
							if(g_nPublicCardAfterRiverLevel<ONE_PAIR)							
							{
								if(g_gBetNum[g_nRound-1]<=2)
								{
									nRaiseBet = g_nCurrentRoundNeedRaise+g_nSmallBlindBet;
									stringstream sStream;
									sStream>>nRaiseBet;
									string cRaiseBet = sStream.str();
									cActionMsg = "raise "+cRaiseBet+" "+"\n";
								}
								else cActionMsg = "call \n";
								cActionMsg = "call \n";

							}
							else if(g_nPublicCardAfterRiverLevel==ONE_PAIR)
							{
								if(g_nHandCardLevel<=2)
									cActionMsg = "call \n";
								else cActionMsg = "check \n";
							}
							else 
							{
								if(g_nHandCardLevel>=2)
									cActionMsg = "fold \n";
								else cActionMsg = "check \n";
							}
						
					
							break;
						}
					case TWO_PAIR:
						{
							switch(g_nPublicCardAfterRiverLevel)
							{
								case STRAIGHT_FLUSH:
								case FLUSH:
								case STRAIGHT:
									{
										if(g_nCurrentRoundAllInNum>0||g_nCurrentRoundRaiseNum>0)
											cActionMsg="fold \n";
										else
											{
												if(g_cLastAction=="check"&&g_nCardAfterRiverValue>=121212070702&&g_gBetNum[g_nRound-1]==1)//最多跟注两次)
												{
													nRaiseBet = g_nCurrentRoundNeedRaise;
													stringstream sStream;
													sStream>>nRaiseBet;
													string cRaiseBet = sStream.str();
													cActionMsg = "raise "+cRaiseBet+" "+"\n";
												}
												else cActionMsg = "call \n";

											}
									}
								break;
								case ONE_PAIR:
									{
											if(g_cLastAction=="check")
											{
												cActionMsg = "check \n";

											}
											
											else if(g_nCurrentRoundAllInNum>0)
												cActionMsg = "fold \n";
											else if(g_nCurrentRoundRaiseNum>0)//有人加注的情况下
											{
												if((g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet)>=max(nMyCurrentLeafJetton/4,2*g_nSmallBlindBet))//加注额度比较大时弃牌
													cActionMsg= "fold \n";
												else if(nUnfoldPlayerNum>4&&g_nMyOrder>g_nCurrentAlivePlayerNum-3)//位置靠后且前面玩家未弃牌个数比较多时弃牌
														cActionMsg= "fold \n";
												else if(g_nHandCardType==HIGH_CARD&&nMaxPoint<10)//手牌为单牌时且点数比较小时
													cActionMsg= "fold \n";
												else
												{
													if(g_gBetNum[g_nRound-1]<=2)//最多跟注两次
														cActionMsg= "call \n";
													else cActionMsg= "fold \n";
												}
												
											}
											else cActionMsg = "call \n";
										
									}
									break;
								case HIGH_CARD:
									{
											if(g_cLastAction=="check")
											{
												cActionMsg = "check \n";

											}
											
											else if(g_nCurrentRoundAllInNum>0)
												cActionMsg = "fold \n";
											else if(g_nCurrentRoundRaiseNum>0)//有人加注的情况下
											{
												if((g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet)>=max(nMyCurrentLeafJetton/4,2*g_nSmallBlindBet))//加注额度比较大时弃牌
													cActionMsg= "fold \n";
												else if(nUnfoldPlayerNum>4&&g_nMyOrder>g_nCurrentAlivePlayerNum-3)//位置靠后且前面玩家未弃牌个数比较多时弃牌
														cActionMsg= "fold \n";
												else if(g_nHandCardType==HIGH_CARD&&nMaxPoint<10)//手牌为单牌时且点数比较小时
													cActionMsg= "fold \n";
												else
												{
													if(g_gBetNum[g_nRound-1]<=2)//最多跟注两次
													{
														if(isCurrentPublicCardsAbleBeStraight(5)||isCurrentPublicCardsAbleBeFlush(5))//如果公牌可能出现顺子或者同花则弃牌
															cActionMsg= "fold \n";
														else cActionMsg= "call \n";
													}
													else cActionMsg= "fold \n";
												}
												
											}
											else if(g_nCardAfterRiverValue>=121212101002)
											{
												if(g_gBetNum[g_nRound-1]==1)//最多跟注两次
												{
													nRaiseBet = g_nCurrentRoundNeedRaise;
													stringstream sStream;
													sStream>>nRaiseBet;
													string cRaiseBet = sStream.str();
													cActionMsg = "raise "+cRaiseBet+" "+"\n";
												}
												else cActionMsg = "call \n";
											}
												
											else	cActionMsg = "call \n";
										
									}
									break;
								default:
									cActionMsg = "fold \n";
									break;

							}
						break;
					}
					case ONE_PAIR:
						{
							switch(g_nPublicCardAfterRiverLevel)
							{
								case STRAIGHT_FLUSH:
								case FLUSH:
								case STRAIGHT:
									{
										if(g_nCurrentRoundAllInNum>0||g_nCurrentRoundRaiseNum>0)
											cActionMsg="fold \n";
										else
											 cActionMsg = "call \n";
		
									}
								break;
								case ONE_PAIR:
									{
											if(g_cLastAction=="check")
											{
												cActionMsg = "check \n";

											}
											
											else if(g_nCurrentRoundAllInNum>0||g_nCurrentRoundRaiseNum>0)
												cActionMsg = "fold \n";
											
											else
											{
												if(nUnfoldPlayerNum>2)
												{
														if(g_nCardAfterRiverValue<111313140203)
															cActionMsg = "fold \n";
														else cActionMsg="call \n";
												}
												else 
												{
														if(g_nCardAfterRiverValue<111111140203)
															cActionMsg = "fold \n";
														else cActionMsg="call \n";
												}
											}
									}
									break;
								case HIGH_CARD:
									{
											if(g_cLastAction=="check")
											{
												cActionMsg = "check \n";

											}
											
											else if(g_nCurrentRoundAllInNum>0)
												cActionMsg = "fold \n";
											else if(g_nCurrentRoundRaiseNum>0)//有人加注的情况下
											{
												if((g_nCurrentRoundNeedRaise+2*g_nSmallBlindBet)>=max(nMyCurrentLeafJetton/4,2*g_nSmallBlindBet))//加注额度比较大时弃牌
													cActionMsg= "fold \n";
												else if(nUnfoldPlayerNum>4&&g_nMyOrder>g_nCurrentAlivePlayerNum-3)//位置靠后且前面玩家未弃牌个数比较多时弃牌
														cActionMsg= "fold \n";
												else if(g_nHandCardType==HIGH_CARD&&nMaxPoint<10)//手牌为单牌时且点数比较小时
													cActionMsg= "fold \n";
												else
												{
													if(g_gBetNum[g_nRound-1]<=2)//最多跟注两次
													{
														if(isCurrentPublicCardsAbleBeStraight(5)||isCurrentPublicCardsAbleBeFlush(5))//如果公牌可能出现顺子或者同花则弃牌
															cActionMsg= "fold \n";
														else 
														{
															if(nUnfoldPlayerNum>4)
															{
																	if(g_nCardAfterRiverValue<111313140203)
																		cActionMsg = "fold \n";
																	else cActionMsg="call \n";
															}
															else 
															{
																	if(g_nCardAfterRiverValue<111111140203)
																		cActionMsg = "fold \n";
																	else cActionMsg="call \n";
															}
															
														}
														
													}
													else cActionMsg= "fold \n";
												}
												
											}
											else cActionMsg = "call \n";
										
									}
									break;
								default:
									break;
								}
							
							break;
						}
					
					case HIGH_CARD:
						if(nUnfoldPlayerNum>2)
						{
								
							cActionMsg = "fold \n";
								
						}
						else 
						{
							if(g_nCardAfterRiverValue<101312020304||g_nCurrentRoundAllInNum>0||g_nCurrentRoundRaiseNum>0)
								cActionMsg = "fold \n";
							else cActionMsg="check \n";
						}
						break;
					default:
						cActionMsg = "check \n";
						break;
				}
			}
		
	}
	if(__ClientWrite(cActionMsg.c_str(),cActionMsg.length()) < 0)
	{
        printf("send message error\n");
        exit(0);
    }
	return 1;
}

int CTcpClient::OnGetMsgType(char *pBuffer)
{
	while(strlen(pBuffer)<2)
		__RecieveMessage(pBuffer, 2);
	char buf[3];
	buf[0]=pBuffer[0];
	buf[1]=pBuffer[1];
	buf[2]='\0';
	int type;
	if (string(buf)=="se")
		type = SEAT_MSG;
	else if (string(buf)=="bl")
		type = BLIND_MSG ;
	else if (string(buf)=="ho")
		type = HOLD_CARD_MSG;
	else if (string(buf)=="in")
		type = INQUIRE_MSG;
	else if (string(buf)=="fl")
		type = FLOP_MSG;
	else if (string(buf)=="tu")
		type = TURN_MSG;
	else if (string(buf)=="ri")
		type = RIVER_MSG;
	else if (string(buf)=="no")
		type = NOTIFY_MSG;
	else if (string(buf)=="sh")
		type = SHOWDOWN_MSG;
	else if (string(buf)=="po")
		type = POT_WIN_MSG;
	else if (string(buf)=="ga")
		type = GAME_OVER_MSG;
	return type;
			
}

int main(int argc,char* argv[])  
{
       if(argc!=6)
	   {
			cout<<"the error input,Please input again!"<<endl;
			return 0;
	   }
		bool isWholeGameOverFlag =false;//本场比赛结束标志
		bool isCurrentGameOverFlag =false;//当前局游戏结束标志
		clearTheWholeValueBeforeANewSet();
		CTcpClient iGamePlayer(argv);
		while(!isWholeGameOverFlag)
		{  		
			int messageType = iGamePlayer.OnGetMsgType(g_gRecvBuffer);
			//cout << "heihei:"<<messageType<<endl;
			switch(messageType)
			{
			case SEAT_MSG:
				iGamePlayer.OnRecvSeatMsg(g_gPlayerMap, g_gRecvBuffer);
				break;
			case BLIND_MSG:
				iGamePlayer.OnRecvBlindMsg(g_gPlayerMap, g_gRecvBuffer);		//收盲注消息
				break;
			case HOLD_CARD_MSG:
				iGamePlayer.OnRecvHoldCardsMsg(g_iMyCard, g_gRecvBuffer);
				break;
			case INQUIRE_MSG:
				//cout<<"before recv inquire msg: "<<endl;
				iGamePlayer.OnRecvInquireMsg(g_gPlayerMap, g_gRecvBuffer);			//收询问消息
				//cout<<"after recv inquire msg: "<<endl;
				iGamePlayer.OnDecideAction();		//采取决策
				//cout<<"after send action msg: "<<endl;
				break;
			case FLOP_MSG:
				//cout<<"before recv flop msg: "<<endl;
				iGamePlayer.OnRecvFlopCardMsg(g_iPublicCard,g_gRecvBuffer);
				//cout<<"after recv flop msg: "<<endl;
				break;
			case TURN_MSG:
				//cout<<"before recv turn msg: "<<endl;
				iGamePlayer.OnRecvTurnCardsMsg(g_iPublicCard,g_gRecvBuffer);
				//cout<<"after recv turn msg: "<<endl;
				break;
			case RIVER_MSG:
				//cout<<"before recv river msg: "<<endl;
				iGamePlayer.OnRecvRiverCardsMsg(g_iPublicCard,g_gRecvBuffer);
				//cout<<"after recv river msg: "<<endl;
				break;
			case NOTIFY_MSG:
				//cout<<"recv notify msg: "<<endl;
				break;
			case SHOWDOWN_MSG:
				//cout<<"recv showdown msg: "<<endl;
				iGamePlayer.OnRecvShowDownMsg(g_gPlayerMap,g_gRecvBuffer);
				break;
			case POT_WIN_MSG:
				//cout<<"recv pot-win msg: "<<endl;
				iGamePlayer.OnRecvPotWinMsg(g_gPlayerMap,g_gRecvBuffer);
				//cout<<"after recv pot-win msg: "<<g_gRecvBuffer<<endl;
			
				clearTheWholeValueBeforeANewSet();
				break;
			case GAME_OVER_MSG:
				isWholeGameOverFlag = true;
				cout<<"game over!"<<endl;
				break;
			default:
				cout<<"default"<<endl;
				break;
				
			}
		}
        return 0;  
} 

