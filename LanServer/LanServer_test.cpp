#include <winsock2.h>
#include <stdio.h>

#include "StreamQueue.h"
#include "NPacket.h"
#include "LanServer.h"
#include "LanServer_test.h"

stClient Client[MAX_CLIENT];

CLanServerTest::CLanServerTest()
	: CLanServer()
{
	Initial();
	iClientID = 0;
}

CLanServerTest::~CLanServerTest() 
 {}

void CLanServerTest::Initial()
{
	for (int iCnt = 0; iCnt < MAX_CLIENT; iCnt++)
		Client[iCnt]._Session = NULL;
}

void CLanServerTest::SendPacket(__int64 ClientID, CNPacket *pPacket)
{
	int iCnt = 0;

	for (iCnt = 0; iCnt < MAX_CLIENT; iCnt++)
	{
		if (Client[iCnt]._Session->_iSessionID == ClientID)
			break;
	}

	Client[iCnt]._Session->SendQ.Put((char *)pPacket->GetBufferPtr(), pPacket->GetDataSize());
	SendPost(Client[iCnt]._Session);
}

void CLanServerTest::OnClientJoin(CSession *pSession, __int64 ClientID)		// Accept �� ����ó�� �Ϸ� �� ȣ��.
{
	int iCnt = 0;

	for (iCnt = 0; iCnt < MAX_CLIENT; iCnt++)
	{
		if (Client[iCnt]._Session == NULL)
			break;
	}

	pSession->_iSessionID = iClientID++;
	Client[iCnt]._Session = pSession;
}

void CLanServerTest::OnClientLeave(__int64 ClientID)   					// Disconnect �� ȣ��
{

}

bool CLanServerTest::OnConnectionRequest(WCHAR *ClientIP, int Port)		// accept ����
{
	if (iSessionCount >= MAX_CLIENT)	return false;

	return true;
}

void CLanServerTest::OnRecv(__int64 ClientID, CNPacket *pPacket)			// ��Ŷ ���� �Ϸ� ��
{
	CNPacket nPacket;

	while (pPacket->GetDataSize() > 0)
	{
		short sHeader;
		__int64 iPayload;

		*pPacket >> sHeader;
		*pPacket >> iPayload;

		if (sHeader != sizeof(iPayload))
			return;

		////////////////////////////////////////////////////////////////
		// ��Ŷ ����
		////////////////////////////////////////////////////////////////
		nPacket << (short)8;
		nPacket << iPayload;
	}

	SendPacket(ClientID, &nPacket);
}

void CLanServerTest::OnSend(__int64 ClientID, int sendsize)				// ��Ŷ �۽� �Ϸ� ��
{
	
}

void CLanServerTest::OnWorkerThreadBegin()								// ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ��
{

}

void CLanServerTest::OnWorkerThreadEnd()								// ��Ŀ������ 1���� ���� ��
{
	
}

void CLanServerTest::OnError(int errorCode, WCHAR *errorString)
{
	wprintf(L"ErrorCode : %d, ErrorMsg : %s\n", errorCode, errorString);
}
