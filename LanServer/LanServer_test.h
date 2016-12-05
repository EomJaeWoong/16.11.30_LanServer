#ifndef __LANSERVER_TEST__H__
#define __LANSERVER_TEST__H__

#define MAX_CLIENT 500
#define SERVER_IP L"127.0.0.1"
#define SERVER_PORT 6000

struct stClient
{
	CSession *_Session;

	short sHeader;
	__int64 iPayload;
};

class CLanServerTest : public CLanServer
{
public:
	CLanServerTest();
	virtual ~CLanServerTest();

public :
	void Initial();

	virtual void SendPacket(__int64 ClientID, CNPacket *pPacket);
	virtual void OnClientJoin(CSession *pSession, __int64 ClientID);   // Accept �� ����ó�� �Ϸ� �� ȣ��.
	virtual void OnClientLeave(__int64 ClientID);   					// Disconnect �� ȣ��
	virtual bool OnConnectionRequest(WCHAR *ClientIP, int Port);		// accept ����

	virtual void OnRecv(__int64 ClientID, CNPacket *pPacket);			// ��Ŷ ���� �Ϸ� ��
	virtual void OnSend(__int64 ClientID, int sendsize);				// ��Ŷ �۽� �Ϸ� ��

	virtual void OnWorkerThreadBegin();								// ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ��
	virtual void OnWorkerThreadEnd();								// ��Ŀ������ 1���� ���� ��

	virtual void OnError(int errorCode, WCHAR *errorString);
private :
	__int64 iClientID;
};

#endif