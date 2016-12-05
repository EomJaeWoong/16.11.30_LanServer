#include <windows.h>
#include <process.h>
#include <WS2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")

#include "StreamQueue.h"
#include "NPacket.h"
#include "LanServer.h"


CLanServer::CLanServer()
{
	iWorkerThdNum = MAX_THREAD;
	__int64 iSessionID = 0;
}

CLanServer::CLanServer(int iWorkerThdNum)
{
	iWorkerThdNum > MAX_THREAD ? MAX_THREAD : iWorkerThdNum;
}

CLanServer::~CLanServer()
{

}

BOOL CLanServer::Start(WCHAR *wOpenIP, int iPort, int iWorkerThdNum, BOOL bNagle, int iMaxConnection)
{
	int retval;
	DWORD dwThreadID;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return FALSE;

	// Completion Port 생성
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hIOCP == NULL)
		return FALSE;

	// Thread 생성
	hAcceptThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		AcceptThread,
		this,
		0,
		(unsigned int *)&dwThreadID
		);

	for (int iCnt = 0; iCnt < iWorkerThdNum; iCnt++)
	{
		hWorkerThread[iCnt] = (HANDLE)_beginthreadex(
			NULL,
			0,
			WorkerThread,
			this,
			0,
			(unsigned int *)&dwThreadID
			);
	}

	// socket 생성
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
		return FALSE;

	//bind
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(iPort);
	InetPton(AF_INET, wOpenIP, &serverAddr.sin_addr);
	retval = bind(listen_sock, (SOCKADDR *)&serverAddr, sizeof(SOCKADDR_IN));
	if (retval == SOCKET_ERROR)
		return FALSE;

	//listen
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
		return FALSE;

	return TRUE;
}

void CLanServer::Stop()
{

}

void CLanServer::WorkerThead_Update(LPVOID workerArg)
{
	int retval;
	CSession pSession;
	WSABUF wsaBuf;

	while (1)
	{
		DWORD dwTransferred = 0;
		OVERLAPPED *pOverlapped = NULL;
		CSession *pSession = NULL;

		retval = GetQueuedCompletionStatus(hIOCP, &dwTransferred, (PULONG_PTR)&pSession,
			(LPOVERLAPPED *)pOverlapped, INFINITE);

		// Error, 종료 처리
		if (retval == FALSE && (pOverlapped == NULL || pSession == NULL))
		{ }	//IOCP 에러 서버 종료

		else if (dwTransferred == 0 && pSession == NULL && pOverlapped == NULL)
		{ } //워커스레드 정상 종료

		else if (dwTransferred == 0)
		{
			if (retval == FALSE)
			{
				//WSAGetLastError
				//필요한 로그 찍기
			}

			//정상종료
		}

		//recv
		else if (pOverlapped == &pSession->_RecvOverlapped)
		{

		}

		//send
		else if (pOverlapped == &pSession->_SendOverlapped)
		{

		}

		if (0 == InterlockedDecrement((LONG *)&pSession->_lIOCount))
		{ }	//Session Release
	}
}

void CLanServer::AcceptThead_Update(LPVOID acceptArg)
{
	CSession *pSession = new CSession;
	SOCKADDR_IN clientSock;
	WCHAR clientIP[16];

	while (1)
	{
		pSession->_socket = accept(listen_sock, (SOCKADDR *)&clientSock, (int *)sizeof(SOCKADDR_IN));
		InetNtop(AF_INET, &clientSock.sin_addr, clientIP, 16);

		if (!OnConnectionRequest(clientIP, ntohs(clientSock.sin_port)))		// accept 직후
		{
		}	//클라이언트 거부

		if (!CreateIoCompletionPort((HANDLE)pSession->_socket, hIOCP, (DWORD)&pSession, 0))
			return;

		OnClientJoin(pSession, iSessionID++);
	}
}

static unsigned __stdcall WorkerThread(LPVOID workerArg)
{
	((CLanServer *)workerArg)->WorkerThead_Update(workerArg);
}

static unsigned __stdcall AcceptThread(LPVOID acceptArg)
{
	((CLanServer *)acceptArg)->AcceptThead_Update(acceptArg);
}

int CLanServer::GetClientCount()
{

}