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
	virtual void OnClientJoin(CSession *pSession, __int64 ClientID);   // Accept 후 접속처리 완료 후 호출.
	virtual void OnClientLeave(__int64 ClientID);   					// Disconnect 후 호출
	virtual bool OnConnectionRequest(WCHAR *ClientIP, int Port);		// accept 직후

	virtual void OnRecv(__int64 ClientID, CNPacket *pPacket);			// 패킷 수신 완료 후
	virtual void OnSend(__int64 ClientID, int sendsize);				// 패킷 송신 완료 후

	virtual void OnWorkerThreadBegin();								// 워커스레드 GQCS 바로 하단에서 호출
	virtual void OnWorkerThreadEnd();								// 워커스레드 1루프 종료 후

	virtual void OnError(int errorCode, WCHAR *errorString);
private :
	__int64 iClientID;
};

#endif