#pragma once


#include <WinSock2.h>
#include <Windows.h>

#include <list>

#define BUFF_SIZE 1024
#define WM_SHOW_RECV_MSG WM_USER+111

class MyThreadPool;

enum  IO_TYPE
{
	IO_ACCEPT,
	IO_READ,
	IO_WRITE,
	IO_UNKNOWN
};

class CIOCPBufData
{
public:
	OVERLAPPED m_Overlapped;
	WSABUF m_WSABuf;
	IO_TYPE m_eIOType;
	char m_szBuffer[BUFF_SIZE];
	CIOCPBufData(IO_TYPE type)
	{
		m_eIOType = type;
		memset(&m_Overlapped, 0, sizeof(m_Overlapped));
		memset(&m_szBuffer, 0, BUFF_SIZE);
		m_WSABuf.buf = m_szBuffer;
		m_WSABuf.len = BUFF_SIZE;
	}
};

class CSockData
{
public:
	SOCKET m_rawSocket;
	SOCKET m_acceptSocket;

	CSockData()
	{
		m_rawSocket = INVALID_SOCKET;
		m_acceptSocket = INVALID_SOCKET;
	}
};

class CMyIOCPServer
{
public:
	CMyIOCPServer(void);
	~CMyIOCPServer();

	bool StartServer(int nPort);
	bool PostSend(const char* szMsg, int nLen);

	void StopServer();
	void SetWnd(void* pWnd);
	bool SetAcceptEx(SOCKET hListenSocket);

private:
	void Init();
	bool InitWinSock();
	CSockData* AssignSockWithIOCP(SOCKET hSocket);
	bool PostAccept(CSockData* pSockData);
	bool PostRecv(CSockData* pSockData);

	//激活一个工作线程
	void RunWorkerThread();
	static int ThreadPoolProc(void* pParam);
private:
	LONG m_nWorkingThreadNums;
	void* m_pWnd;
	SOCKET m_hListenSocket;
	HANDLE m_hIOCP;
	MyThreadPool* m_pThreadPool;
	std::list<SOCKET> m_listSock;
};

