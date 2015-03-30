#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h> 
#include <windows.h> 
#include <time.h> 
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sys/types.h> 

using namespace std;

#pragma  comment(lib, "Ws2_32.lib") 

#define DEF_DNS_ADDRESS "10.3.9.4"	//�ⲿDNS��������ַ
#define LOCAL_ADDRESS "127.0.0.1"		//����DNS��������ַ
#define DNS_PORT 53						//����DNS�����53�˿�
#define BUF_SIZE 512
#define LENGTH 65
#define AMOUNT 300
#define NOTFOUND -1

//DNS�����ײ�
typedef struct DNSHeader
{
    unsigned short ID;
    unsigned short Flags;
    unsigned short QuestNum;
    unsigned short AnswerNum;
    unsigned short AuthorNum;
    unsigned short AdditionNum;
} DNSHDR, *pDNSHDR;

//DNS����������ṹ
typedef struct translate
{
	string IP;						//IP��ַ
	string domain;					//����
} Translate;

//IDת����ṹ
typedef struct IDChange
{
	unsigned short oldID;			//ԭ��ID
	BOOL done;						//����Ƿ���ɽ���
	SOCKADDR_IN client;				//�������׽��ֵ�ַ
} IDTransform;

Translate DNS_table[AMOUNT];		//DNS����������
IDTransform IDTransTable[AMOUNT];	//IDת����
int IDcount = 0;					//ת�����е���Ŀ����
char url[LENGTH];					//����
SYSTEMTIME sys;                     //ϵͳʱ��
int Day, Hour, Minute, Second, Milliseconds;//����ϵͳʱ��ı���
int judge_not_V6=0;

//��������ȡ����������
int GetTable(char *tablePath)
{
	int i=0, j, pos=0;
	string table[AMOUNT];

	ifstream infile(tablePath, ios::in);	//�Զ��뷽ʽ���ı��ļ�

	if(! infile) {
		cerr << "Open" << tablePath << "error!" <<endl;
		exit(1);
	}

	//ÿ�δ��ļ��ж���һ�У�ֱ�������ļ�������Ϊֹ
	while (getline(infile, table[i]) && i < AMOUNT)
		i++;

	if (i == AMOUNT-1)
		cout << "The DNS table memory is full. " << endl;

	for (j = 0; j <=i-1; j++) {
		pos = table[j].find(' ');
		if (pos > table[j].size())
			cout << "The record is not in a correct format. " << endl;
		else {
			DNS_table[j].IP = table[j].substr(0, pos);
			DNS_table[j].domain = table[j].substr(pos+1);
		}
	}

	infile.close();		//�ر��ļ�
	cout << "Load records succeed. " << endl;

	return i-1;			//������������������Ŀ����
}

//��������ȡDNS�����е�����
void GetUrl(char *recvbuf, int recvnum)
{
	char urlname[LENGTH];
	int i = 0, j, k = 0;
	int t=0;
	memset(url, 0, LENGTH);
	memcpy(urlname, &(recvbuf[sizeof(DNSHDR)]), recvnum-16);	//��ȡ�������е�������ʾ
	//urlname[recvnum-29]='\0';
	int len = strlen(urlname);
	if(recvbuf[recvnum-3]!=28)
	{
		//cout<<"bsdjwhdj"<<endl;
		judge_not_V6=1;
		//return;
	}
	else
		judge_not_V6=0;
	//����ת��
	while (i < len) {
		if (urlname[i] > 0 && urlname[i] <= 63)
		{   
			t=t+urlname[i];
			for (j = urlname[i], i++; j > 0; j--, i++, k++)
				url[k] = urlname[i];
		}		
		if (urlname[i] != 0) {
			url[k] = '.';
		    k++;
		}
	}

	url[k] = '\0';
}

//�������ж��Ƿ��ڱ����ҵ�DNS�����е��������ҵ������±�
int IsFind(char* url, int num)
{
	int find = NOTFOUND;
	char* domain;

	for (int i = 0; i < num; i++) {
		domain = (char *)DNS_table[i].domain.c_str();
		if (strcmp(domain, url) == 0) {	//�ҵ�
			find = i;
			break;
		}
	}

	return find;
}

//������������IDת��Ϊ�µ�ID��������Ϣд��IDת������
unsigned short RegisterNewID (unsigned short oID, SOCKADDR_IN temp, BOOL ifdone)
{
	srand(time(NULL));
	IDTransTable[IDcount].oldID = oID;
	IDTransTable[IDcount].client = temp;
	IDTransTable[IDcount].done  = ifdone;
	IDcount++;

	return (unsigned short)(IDcount-1);	//�Ա����±���Ϊ�µ�ID
}

//��������ӡ ʱ�� newID ���� ���� IP
void DisplayInfo(unsigned short newID, int find)
{
	//��ӡʱ��
	GetLocalTime( &sys );
	if(sys.wMilliseconds >= Milliseconds)
	{
	    cout << setiosflags(ios::right) << setw(7) << setfill(' ') << (((sys.wDay - Day) * 24 + sys.wHour - Hour) * 60 + sys.wMinute - Minute) * 60 + sys.wSecond - Second;//���ÿ��Ϊ7��right���뷽ʽ
	    cout << '.' << setiosflags(ios::right) << setw(3) << setfill('0') << sys.wMilliseconds - Milliseconds;
	}
	else {
		cout << setiosflags(ios::right) << setw(7) << setfill(' ') << (((sys.wDay - Day) * 24 + sys.wHour - Hour) * 60 + sys.wMinute - Minute) * 60 + sys.wSecond - Second - 1;//���ÿ��Ϊ7��right���뷽ʽ
	    cout << '.' << setiosflags(ios::right) << setw(3) << setfill('0') << 1000 + sys.wMilliseconds - Milliseconds;
	}
	cout << "    ";

	//��ӡת�����µ�ID
	cout.setf(ios::left);
	cout << setiosflags(ios::left) << setw(4) << setfill(' ') << newID;
	cout << "    ";

	//�ڱ���û���ҵ�DNS�����е�����
	if (find == NOTFOUND) 
	{  
		//�м̹���
		cout.setf(ios::left);
		cout << setiosflags(ios::left) << setw(6) << setfill(' ') << "�м�";
		cout << "    ";
		//��ӡ����
		cout.setf(ios::left);
		cout << setiosflags(ios::left) << setw(20) << setfill(' ') << url;
		cout << "    ";
		//��ӡIP
		cout.setf(ios::left);
		cout << setiosflags(ios::left) << setw(20) << setfill(' ') << endl;
		
	}

	//�ڱ����ҵ�DNS�����е�����
	else {
	    if(DNS_table[find].IP == "0.0.0.0")  //������վ����
		{
			//���ι���
			cout.setf(ios::left); 
		    cout << setiosflags(ios::left) << setw(6) << setfill(' ') << "����";
		    cout << "    ";
			//��ӡ����(��*)
			cout.setf(ios::left); 
		    cout << "*" << setiosflags(ios::left) << setw(19) << setfill(' ') << url;
		    cout << "    ";
			//��ӡIP
			cout.setf(ios::left); 
		    cout << setiosflags(ios::left) << setw(20) << setfill(' ') << endl;
		}

		//�������Ϊ��ͨIP��ַ������ͻ����������ַ
		else {
			//����������
			cout.setf(ios::left);
		    cout << setiosflags(ios::left) << setw(6) << setfill(' ') << "������";
		    cout << "    ";
			//��ӡ����
			cout.setf(ios::left);
		    cout << "*" << setiosflags(ios::left) << setw(19) << setfill(' ') << url;
		    cout << "    ";
			//��ӡIP
			cout.setf(ios::left);
		    cout << setiosflags(ios::left) << setw(20) << setfill(' ') << DNS_table[find].IP << endl;
		}
	}
}


int main(int argc, char** argv) 
{ 
    WSADATA wsaData; 
    SOCKET  socketServer, socketLocal;				//����DNS���ⲿDNS�����׽���
    SOCKADDR_IN serverName, clientName, localName;	//����DNS���ⲿDNS����������������׽��ֵ�ַ
    char sendbuf[BUF_SIZE];
    char recvbuf[BUF_SIZE]; 
    char tablePath[100];
    char outerDns[16];
    int iLen_cli, iSend, iRecv;
    int num;
	struct fd_set socket_set1;//�׽��ּ���
	
	FD_ZERO(&socket_set1);
	struct timeval timeout;
	//��ʱ��ʱ���趨
	timeout.tv_sec=2;
	timeout.tv_usec=0;

	strcpy(outerDns, DEF_DNS_ADDRESS);
	strcpy(tablePath, "dnsrelay.dat");

	num = GetTable(tablePath);						//��ȡ����������

	//����ϵͳ��ʱ��
	GetLocalTime(&sys);
	Day          = sys.wDay;
    Hour         = sys.wHour;
	Minute       = sys.wMinute;
	Second       = sys.wSecond;
	Milliseconds = sys.wMilliseconds;

	for (int i=0; i < AMOUNT; i++) {				//��ʼ��IDת����
		IDTransTable[i].oldID = 0;
		IDTransTable[i].done  = FALSE;
		memset(&(IDTransTable[i].client), 0, sizeof(SOCKADDR_IN));
	}

    WSAStartup(MAKEWORD(2,2), &wsaData);			//��ʼ��ws2_32.dll��̬���ӿ�

	//��������DNS���ⲿDNS�׽���
    socketServer = socket(AF_INET, SOCK_DGRAM, 0);
	socketLocal = socket(AF_INET, SOCK_DGRAM, 0);
	
	//���ñ���DNS���ⲿDNS�����׽���
	localName.sin_family = AF_INET;
	localName.sin_port = htons(DNS_PORT);
	localName.sin_addr.s_addr = inet_addr(LOCAL_ADDRESS);

	serverName.sin_family = AF_INET;
	serverName.sin_port = htons(DNS_PORT);
	serverName.sin_addr.s_addr = inet_addr(outerDns);

	//�󶨱���DNS��������ַ
	if (bind(socketLocal, (SOCKADDR*)&localName, sizeof(localName))) {
		cout << "Binding Port 53 failed." << endl;
		exit(1);
	}
	else
		cout << "Binding Port 53 succeed." << endl;

	//����DNS�м̷������ľ������
	whi:while (1) 
	{
		FD_ZERO(&socket_set1);//���fdset�������ļ��������ϵ��
		FD_SET(socketServer, &socket_set1);//���������fdset����ϵ�� 
		iLen_cli = sizeof(clientName);
		memset(recvbuf, 0, BUF_SIZE);
		
		//����DNS����
		iRecv = recvfrom(socketLocal, recvbuf, sizeof(recvbuf), 0, (SOCKADDR*)&clientName, &iLen_cli);

		if (iRecv == SOCKET_ERROR) {
			//cout << "Recvfrom Failed: " << WSAGetLastError() << endl;
			continue;
		}
		else if (iRecv == 0) 
			break;
		
		else {//�յ�����	
				GetUrl(recvbuf, iRecv);				//��ȡ����
				int find = IsFind(url, num);		//�������������в���

			//��������������û���ҵ�
			if (find == NOTFOUND) 
			{
				//IDת����ת���󽫽��յ��������ķ��͸��ⲿ������
				unsigned short *pID = (unsigned short *)malloc(sizeof(unsigned short));
				memcpy(pID, recvbuf, sizeof(unsigned short));
				unsigned short nID = htons(RegisterNewID(ntohs(*pID), clientName, FALSE));
				memcpy(recvbuf, &nID, sizeof(unsigned short));

				//��ӡ ʱ�� newID ���� ���� IP
				DisplayInfo(ntohs(nID), find);
				
				//��recvbufת����ָ�����ⲿDNS������
				iSend = sendto(socketServer, recvbuf, iRecv, 0, (SOCKADDR*)&serverName, sizeof(serverName));
				
				if (iSend == SOCKET_ERROR) {
					cout << "sendto Failed: " << WSAGetLastError() << endl;
					continue;
				}
				else if (iSend == 0)
					break;
				
				free(pID);	//�ͷŶ�̬������ڴ�
				
				//��ʱ����
				//cout<<select(0,&socket_set1,0,0, &timeout)<<endl;
				if(select(0,&socket_set1,0,0, &timeout)==0)
				{
					printf("timeout!\n");
					goto whi;
				}
				if(select(0,&socket_set1,0,0, &timeout)==-1)
				{
					printf("select error!\n");
					goto whi;
				}
				//cout << "url2:"<<url <<endl;
				
				//���������ⲿDNS����������Ӧ����
				iRecv = recvfrom(socketServer, recvbuf, sizeof(recvbuf), 0, (SOCKADDR*)&clientName, &iLen_cli);
				
				
				//IDת��
				pID = (unsigned short *)malloc(sizeof(unsigned short));
				memcpy(pID, recvbuf, sizeof(unsigned short));
				int m = ntohs(*pID);
				unsigned short oID = htons(IDTransTable[m].oldID);
				memcpy(recvbuf, &oID, sizeof(unsigned short));
				IDTransTable[m].done = TRUE;

				//��IDת�����л�ȡ����DNS�����ߵ���Ϣ
				clientName = IDTransTable[m].client;

				//��recvbufת���������ߴ�
				iSend = sendto(socketLocal, recvbuf, iRecv, 0, (SOCKADDR*)&clientName, sizeof(clientName));
				//iSend = sendto(socketLocal, recvbuf, iRecv, 0,(SOCKADDR*)&clientName, sizeof(clientName));
				if (iSend == SOCKET_ERROR) {
					cout << "sendto Failed: " << WSAGetLastError() << endl;
					continue;
				}
				else if (iSend == 0)
					break;

				free(pID);	//�ͷŶ�̬������ڴ�
				continue;
			}

			//���������������ҵ�
			else  {	
				//��ȡ�����ĵ�ID
				unsigned short *pID = (unsigned short *)malloc(sizeof(unsigned short));
				memcpy(pID, recvbuf, sizeof(unsigned short));

				//ת��ID
				unsigned short nID = RegisterNewID(ntohs(*pID), clientName, FALSE);

				//��ӡ ʱ�� newID ���� ���� IP
				
				DisplayInfo(nID, find);

				//������Ӧ���ķ���
				memcpy(sendbuf, recvbuf, iRecv);						//����������
				unsigned short a = htons(0x8180);						//8180��Ӧ���ı�־,QR��Ϊ1
				memcpy(&sendbuf[2], &a, sizeof(unsigned short));		//�޸ı�־��

				//�޸Ļش�����
				if (strcmp(DNS_table[find].IP.c_str(), "0.0.0.0") == 0|| judge_not_V6==0)
					a = htons(0x0000);	//���ι��ܣ�RR��Ϊ0
				else
					a = htons(0x0001);	//���������ܣ��ش���Ϊ1

				memcpy(&sendbuf[6], &a, sizeof(unsigned short));
				int curLen = 0;

				//����DNS��Ӧ����
				char answer[16];
				unsigned short Name = htons(0xc00c);
				memcpy(answer, &Name, sizeof(unsigned short));
				curLen += sizeof(unsigned short);

				unsigned short TypeA = htons(0x0001);
				memcpy(answer+curLen, &TypeA, sizeof(unsigned short));
				curLen += sizeof(unsigned short);

				unsigned short ClassA = htons(0x0001);
				memcpy(answer+curLen, &ClassA, sizeof(unsigned short));
				curLen += sizeof(unsigned short);

				unsigned long timeLive = htonl(0x7b);
				memcpy(answer+curLen, &timeLive, sizeof(unsigned long));
				curLen += sizeof(unsigned long);

				unsigned short IPLen = htons(0x0004);
				memcpy(answer+curLen, &IPLen, sizeof(unsigned short));
				curLen += sizeof(unsigned short);

				unsigned long IP = (unsigned long) inet_addr(DNS_table[find].IP.c_str());
				memcpy(answer+curLen, &IP, sizeof(unsigned long));
				curLen += sizeof(unsigned long);
				curLen += iRecv;
				
				//�����ĺ���Ӧ���ֹ�ͬ���DNS��Ӧ���Ĵ���sendbuf
				memcpy(sendbuf+iRecv, answer, curLen);

				//����DNS��Ӧ����
				iSend = sendto(socketLocal, sendbuf, curLen, 0, (SOCKADDR*)&clientName, sizeof(clientName));
					
				if (iSend == SOCKET_ERROR) {
					cout << "sendto Failed: " << WSAGetLastError() << endl;
					continue;}
				else if (iSend == 0)
				
				break;
				
			     free(pID);		//�ͷŶ�̬������ڴ�
				continue;
			}
		}
	}

    closesocket(socketServer);	//�ر��׽���
	closesocket(socketLocal);
    WSACleanup();				//�ͷ�ws2_32.dll��̬���ӿ��ʼ��ʱ�������Դ
	system("pause");
    return 0;
}
