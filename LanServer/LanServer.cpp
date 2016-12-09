#include <winsock2.h>
#include <process.h>
#include <WS2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")

#include "StreamQueue.h"
#include "NPacket.h"
#include "LanServer.h"


CLanServer::CLanServer()
{
	iWorkerThdNum = MAX_THREAD;
	iSessionID = 0;
	iSessionCount = 0;
}

CLanServer::CLanServer(int iWorkerThdNum)
{
	iWorkerThdNum > MAX_THREAD ? MAX_THREAD : iWorkerThdNum;
}

CLanServer::~CLanServer()
{

}

//----------------------------------------------------------------------------------------------------
// LanServer 시작
//----------------------------------------------------------------------------------------------------
BOOL CLanServer::Start(WCHAR *wOpenIP, int iPort, int iWorkerThdNum, BOOL bNagle, int iMaxConnection)
{
	int retval;
	DWORD dwThreadID;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// 윈속 초기화
	//////////////////////////////////////////////////////////////////////////////////////////////////
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return FALSE;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Completion Port 생성
	//////////////////////////////////////////////////////////////////////////////////////////////////
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hIOCP == NULL)
		return FALSE;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// socket 생성
	//////////////////////////////////////////////////////////////////////////////////////////////////
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
		return FALSE;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//bind
	//////////////////////////////////////////////////////////////////////////////////////////////////
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(iPort);
	InetPton(AF_INET, wOpenIP, &serverAddr.sin_addr);
	retval = bind(listen_sock, (SOCKADDR *)&serverAddr, sizeof(SOCKADDR_IN));
	if (retval == SOCKET_ERROR)
		return FALSE;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//listen
	//////////////////////////////////////////////////////////////////////////////////////////////////
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
		return FALSE;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Thread 생성
	//////////////////////////////////////////////////////////////////////////////////////////////////
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

	return TRUE;
}

void CLanServer::Stop()
{

}

int CLanServer::WorkerThead_Update(LPVOID workerArg)
{
	int retval;
	WSABUF wsaBuf;
	CNPacket nPacket;

	while (1)
	{
		DWORD dwTransferred = 0;
		OVERLAPPED *pOverlapped = NULL;
		CSession *pSession = NULL;

		retval = GetQueuedCompletionStatus(hIOCP, &dwTransferred, (LPDWORD)&pSession,
			(LPOVERLAPPED *)&pOverlapped, INFINITE);

		OnWorkerThreadBegin();
		
		/////////////////////////////////////////////////////////////////////////////////////////////
		// Error, 종료 처리
		//////////////////////////////////////////////////////////////////////////////////////////////////

		// IOCP 에러 서버 종료
		if (retval == FALSE && (pOverlapped == NULL || pSession == NULL))
		{
			int iErrorCode = WSAGetLastError();
			OnError(iErrorCode, L"IOCP Error\n");

			return -1;
		}

		//워커스레드 정상 종료
		else if (dwTransferred == 0 && pSession == NULL && pOverlapped == NULL)
		{
			OnError(0, L"Worker Thread Done.\n");
			return 0;
		}

		else if (dwTransferred == 0)
		{
			if (retval == FALSE)
			{
				// ?
				int iErrorCode = WSAGetLastError();
				OnError(iErrorCode, L"");
				Disconnect(pSession);

				return iErrorCode;
			}

			//정상종료
			return 0;
		}
		
		//recv
		else if (pOverlapped == &pSession->_RecvOverlapped)
		{
			pSession->RecvQ.MoveWritePos(dwTransferred);
			RecvPost(pSession);
			int iSize = nPacket.Put(pSession->RecvQ.GetReadBufferPtr(), pSession->RecvQ.GetUseSize());
			pSession->RecvQ.RemoveData(iSize);
			OnRecv(pSession->_iSessionID, &nPacket);
			nPacket.Clear();
		}

		//send
		else if (pOverlapped == &pSession->_SendOverlapped)
		{
			pSession->SendQ.RemoveData(dwTransferred);
			pSession->_bSendFlag = FALSE;

			OnSend(pSession->_iSessionID, dwTransferred);

			if (pSession->_bSendFlag)	return -1;
		}

		//Session Release
		if (0 == InterlockedDecrement((LONG *)&pSession->_lIOCount))
			ReleaseSession(pSession);

		OnWorkerThreadEnd();
	}

	return 0;
}

int CLanServer::AcceptThead_Update(LPVOID acceptArg)
{
	HANDLE retval;

	CSession *pSession = new CSession;
	int addrlen = sizeof(SOCKADDR_IN);
	SOCKADDR_IN clientSock;
	WCHAR clientIP[16];

	while (1)
	{
		pSession->_socket = accept(listen_sock, (SOCKADDR *)&clientSock, &addrlen);
		if (pSession->_socket == INVALID_SOCKET)
		{
			int iErrorCode = GetLastError();
			OutputDebugString((WCHAR *)iErrorCode);
		}

		InetNtop(AF_INET, &clientSock.sin_addr, clientIP, 16);

		if (!OnConnectionRequest(clientIP, ntohs(clientSock.sin_port)))		// accept 직후
		{
			continue;
		}	//클라이언트 거부

		//Session 초기화
		wcscpy_s(pSession->_IP, 16, clientIP);
		pSession->_iPort = ntohs(clientSock.sin_port);
		pSession->RecvQ.ClearBuffer();
		pSession->SendQ.ClearBuffer();

		memset(&pSession->_RecvOverlapped, 0, sizeof(OVERLAPPED));
		memset(&pSession->_SendOverlapped, 0, sizeof(OVERLAPPED));

		pSession->_bSendFlag = FALSE;
		pSession->_lIOCount = 0;

		retval = CreateIoCompletionPort((HANDLE)pSession->_socket, hIOCP, (DWORD)pSession, 0);
		if (!retval)
			continue;

		OnClientJoin(pSession, pSession->_iSessionID);

		RecvPost(pSession);
		InterlockedIncrement((LONG *)&iSessionCount);
	}

	return 0;
}

unsigned __stdcall CLanServer::WorkerThread(LPVOID workerArg)
{
	return ((CLanServer *)workerArg)->WorkerThead_Update(workerArg);
}

unsigned __stdcall CLanServer::AcceptThread(LPVOID acceptArg)
{
	return ((CLanServer *)acceptArg)->AcceptThead_Update(acceptArg);
}

void CLanServer::RecvPost(CSession *pSession)
{
	int retval, iCount;
	DWORD dwRecvSize, dwflag = 0;
	WSABUF wBuf;

	wBuf.buf = pSession->RecvQ.GetWriteBufferPtr();
	wBuf.len = pSession->RecvQ.GetNotBrokenPutSize();

	InterlockedIncrement((LONG *)&pSession->_lIOCount);
	retval = WSARecv(pSession->_socket, &wBuf, 1, &dwRecvSize, &dwflag, &pSession->_RecvOverlapped, NULL);

	if (retval == SOCKET_ERROR)
	{
		int iErrorCode = GetLastError();
		if (iErrorCode != WSA_IO_PENDING)
		{
			if (0 == InterlockedDecrement((LONG *)&pSession->_lIOCount))
				//Session Release
				ReleaseSession(pSession);

			return;
		}
	}
}

void CLanServer::SendPost(CSession *pSession)
{
	int retval, iCount;
	DWORD dwRecvSize, dwflag = 0;
	WSABUF wBuf;

	wBuf.buf = pSession->SendQ.GetReadBufferPtr();
	wBuf.len = pSession->SendQ.GetUseSize();

	InterlockedIncrement((LONG *)&pSession->_lIOCount);
	pSession->_bSendFlag = TRUE;
	retval = WSASend(pSession->_socket, &wBuf, 1, &dwRecvSize, dwflag, &pSession->_SendOverlapped, NULL);
	if (retval == SOCKET_ERROR)
	{
		if (GetLastError() != WSA_IO_PENDING)
		{
			if (0 == InterlockedDecrement(&pSession->_lIOCount))			
				//Session Release
				ReleaseSession(pSession);

			return;
		}
	}
}

int CLanServer::GetClientCount()
{
	return iSessionCount;
}

void CLanServer::Disconnect(CSession *pSession)
{
	shutdown(pSession->_socket, SD_BOTH);
	pSession->_socket = INVALID_SOCKET;
}

void CLanServer::ReleaseSession(CSession *pSession)
{
	if (pSession->SendQ.GetUseSize() != 0)
		return;

	if (pSession->RecvQ.GetUseSize() != 0)
		return;

	shutdown(pSession->_socket, SD_BOTH);
	pSession->_socket = INVALID_SOCKET;
}