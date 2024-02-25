#include "CMyIOCPServer.h"
#include "myThreadPool.h"

#include <Mswsock.h>

#pragma comment(lib, "ws2_32.lib")

CMyIOCPServer::CMyIOCPServer():m_nWorkingThreadNums(0), m_pWnd(nullptr),
	m_hListenSocket(INVALID_SOCKET), m_hIOCP(nullptr), m_pThreadPool(nullptr)
{
	Init();
}

CMyIOCPServer::~CMyIOCPServer()
{
}

bool CMyIOCPServer::StartServer(int nPort)
{
	int nRet = false;
	do
	{
		if (!InitWinSock())
		{
			break;
		}
		
		m_hListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (m_hListenSocket == INVALID_SOCKET)
		{
			break;
		}
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(nPort);
		addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		if (bind(m_hListenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
		{
			break;
		}
		if (listen(m_hListenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			break;
		}
		
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (m_hIOCP == NULL)
		{
			break;
		}
		//iocp 与 listen socket绑定
		CSockData* pSockData = AssignSockWithIOCP(m_hListenSocket);
		//启动工作线程
		RunWorkerThread();
		//投递接受请求
		PostAccept(pSockData);
		nRet = true;
	} while (false); 

	return nRet;
}



void CMyIOCPServer::Init()
{
	//获取CPU核心数
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int nMaxThreadCount = si.dwNumberOfProcessors;
	//创建线程池
	m_pThreadPool = new MyThreadPool(nMaxThreadCount);
}

bool CMyIOCPServer::InitWinSock()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return false;
	}
	if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		return false;
	}
	return true;
}

CSockData* CMyIOCPServer::AssignSockWithIOCP(SOCKET hSocket)
{
	CSockData* pSockData = nullptr;
	if (hSocket != INVALID_SOCKET)
	{
		pSockData = new CSockData();
		pSockData->m_rawSocket = hSocket;
		CreateIoCompletionPort((HANDLE)hSocket, m_hIOCP, (ULONG_PTR)pSockData, 0);
	}
	return pSockData;
}

bool CMyIOCPServer::PostAccept(CSockData* pSockData)
{
	bool bRet = false;
	if (pSockData)
	{
		DWORD dwRecv = 0;
		CIOCPBufData* pIOCPBufData = new CIOCPBufData(IO_ACCEPT);

		pSockData->m_acceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

		DWORD dwBytes = 0;
		//AcceptEx 函数接受新连接，返回本地和远程地址 第四个参数为0，表示不接收数据
		//第五个参数 为 为本地地址信息保留的字节数，必须比所使用传送协议的最大地址长度大16个字节
		//第六个参数 为 为远程地址信息保留的字节数，必须比所使用传送协议的最大地址长度大16个字节
		if (AcceptEx(m_hListenSocket, pSockData->m_rawSocket, pIOCPBufData->m_szBuffer, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			&dwBytes, &pIOCPBufData->m_Overlapped))
		{
			bRet = true;
		}
		else
		{
			//WSAGetLastError 返回 ERROR_IO_PENDING，则表示操作已成功启动，仍在进行中
			int nErr = WSAGetLastError();
			if (nErr != WSA_IO_PENDING)
			{
				delete pIOCPBufData;
				bRet = false;
			}
			else
			{
				bRet = true;
			 }
		}
	}
	return bRet;
}

bool CMyIOCPServer::PostRecv(CSockData* pSockData)
{
	bool bRet = false;
	if(pSockData)
	{
		bRet = true;
		CIOCPBufData* pIOCPBufData = new CIOCPBufData(IO_READ);
		DWORD dwRecv = 0, dwFlags = 0;
		//投递接收请求
		int nRet = WSARecv(pSockData->m_rawSocket, &(pIOCPBufData->m_WSABuf), 1, &dwRecv, &dwFlags, &(pIOCPBufData->m_Overlapped), NULL);
		if (SOCKET_ERROR == nRet && (WSA_IO_PENDING != WSAGetLastError()))
		{
			bRet = false;
		}
	 }
	return false;
}

bool CMyIOCPServer::PostSend(const char* szMsg, int nLen)
{
	bool bRet = false;
	//遍历m_listSock ，获取每个socket 
	for (auto it : m_listSock)
	{
		CIOCPBufData* pIOCPBufData = new CIOCPBufData(IO_WRITE);
		memcpy_s(pIOCPBufData->m_szBuffer, BUFF_SIZE, szMsg, nLen);
		DWORD dwSend = 0;
		WSASend(it, &(pIOCPBufData->m_WSABuf), 1, &dwSend, 0, &(pIOCPBufData->m_Overlapped), NULL);
	}
	return bRet;
}

void CMyIOCPServer::RunWorkerThread()
{
	if (m_pThreadPool)
	{
		m_pThreadPool->EnQueue(ThreadPoolProc, this);
	}
}

int CMyIOCPServer::ThreadPoolProc(void* pParam)
{
	if (pParam == nullptr)
	{
		return -1;
	}

	CMyIOCPServer* pServer = (CMyIOCPServer*)pParam;
	CSockData* pSockData = nullptr;
	CIOCPBufData* pIOCPBufData = nullptr;
	InterlockedIncrement(&(pServer->m_nWorkingThreadNums));
	while (true)
	{
		DWORD dwBytes = 0;
		//从IOCP中获取完成的请求
		bool bIoRet = GetQueuedCompletionStatus(pServer->m_hIOCP, &dwBytes, (PULONG_PTR)&pSockData, (LPOVERLAPPED*) & pIOCPBufData, INFINITE);
		if (!bIoRet)
			continue;

		if (dwBytes == 0 && pIOCPBufData && (
			pIOCPBufData->m_eIOType == IO_READ || pIOCPBufData->m_eIOType == IO_WRITE))
		{
			closesocket(pSockData->m_rawSocket);
			//释放资源
			delete pSockData;
			delete pIOCPBufData;
			continue;
		}
		
		if (bIoRet && pSockData && pIOCPBufData)
		{
			switch (pIOCPBufData->m_eIOType)
			{
			case IO_ACCEPT:
			{
				if(pSockData->m_acceptSocket == INVALID_SOCKET)
				{
					continue;
				}
				//激活一个线程
				pServer->RunWorkerThread();
				CSockData*  pTmpSockData = pServer->AssignSockWithIOCP(pSockData->m_acceptSocket);
				if (pTmpSockData)
				{
					//投递接收请求
					pServer->PostRecv(pTmpSockData);
				}
				//投递下一个接受请求
				pServer->PostAccept(pSockData);
				delete pIOCPBufData;
			}
			break;
			case IO_READ:
			{
				//处理接收到的数据
				SendMessage(HWND(pServer->m_pWnd), WM_SHOW_RECV_MSG, 0, (LPARAM)pIOCPBufData->m_szBuffer);
				
				delete pIOCPBufData;
				//投递下一个接受请求
				pServer->PostRecv(pSockData);
			}
			break;
			case IO_WRITE:
			{

				delete pIOCPBufData;
			}
			break;
			default:
				break;
			}
		}
		else if(!pSockData && !pIOCPBufData)
		{
			//退出
			break;
		}	
	}

	InterlockedDecrement(&(pServer->m_nWorkingThreadNums));
	return 0;
}

