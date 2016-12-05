/*
# ������ ��� ��� ����

Ŭ���� - CLanClient

�������� - CLanServer


���� �̸����� �Ǵ� �̿� ����� �̸����� LAN ��� (���� ��Ʈ��ũ ���) ����-���� ��� ������ ���� �ǵ��� ��ħ.

���� ������ �ƴ����� ���鵵 ���� ���� �̸��� ���ϴ°� �ų�.


- ���� ���̵�.

* �ʹ� ������ Ŭ���� ����� ���� �ʴ´�.
�̴� ������ ������� ���� �� �ʿ��� ��쿡 ������ ��ü���� ������ �����°� ����.
�ǹ̾��� ������������ ���� �ʴ´�. ������ ������ ������ �ӵ����� �������� ��� ������ �����.

�Ǹſ� �����̶�� ��¿ �� ���� �ҽ��� ���߰� ����� �ڵ� ������ �и��ϱ� ���ؼ�
�����и�, ���ȭ, �������� ���� ������ �̴� �������Ͽ� Ȯ���� ������� ����.

* IOCP ���� �����.

* Ŭ���� ���ο� ��Ŀ �����带 ����.

* ����,������ ���� ��� �񵿱�� ó��. (��û �� �̺�Ʈ �Լ��� �뺸)

* �̺�Ʈ �Լ� (���ӿϷ�,�ޱ�Ϸ�,������Ϸ�) �� �������� �Լ��� ����.

* ���� ���δ� �� �ΰ��� Ŭ������ ��� ����ϵ��� ����.

* ���ο��� ��ü������ ���� ������ ������.



CLanServer


- bool Start(...) ���� IP / ��Ʈ / ��Ŀ������ �� / ���ۿɼ� / �ִ������� ��
- void Stop(...)
- int GetClientCount(...)
- SendPacket(ClientID, Packet *)

virtual void OnClientJoin(Client ���� / ClientID / ��Ÿ���) = 0;   < Accept �� ����ó�� �Ϸ� �� ȣ��.
virtual void OnClientLeave(ClientID) = 0;   	            < Disconnect �� ȣ��
virtual bool OnConnectionRequest(ClientIP,Port) = 0;        < accept ����
return false; �� Ŭ���̾�Ʈ �ź�.
return true; �� ���� ���

virtual void OnRecv(ClientID, CPacket *) = 0;              < ��Ŷ ���� �Ϸ� ��
virtual void OnSend(ClientID, int sendsize) = 0;           < ��Ŷ �۽� �Ϸ� ��

virtual void OnWorkerThreadBegin() = 0;                    < ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ��
virtual void OnWorkerThreadEnd() = 0;                      < ��Ŀ������ 1���� ���� ��

virtual void OnError(int errorcode, wchar *) = 0;

*/

#ifndef __LANSERVER__H__
#define __LANSERVER__H__

#define MAX_THREAD 50

struct CSession
{
	OVERLAPPED _SendOverlapped;
	OVERLAPPED _RecvOverlapped;

	SOCKET _socket;
	WCHAR _IP[16];
	int _iPort;

	__int64 _iSessionID;

	CAyaStreamSQ SendQ;
	CAyaStreamSQ RecvQ;

	BOOL _bSendFlag;
	LONG _lIOCount;
};

class CLanServer
{
public :
	CLanServer();
	CLanServer(int iWorkerThdNum);
	virtual ~CLanServer();

public :
	BOOL Start(WCHAR *wOpenIP, int iPort, int iWorkerThdNum, BOOL bNagle, int iMaxConnection); //���� IP / ��Ʈ / ��Ŀ������ �� / ���ۿɼ� / �ִ������� ��	
	void Stop();

	int GetClientCount();

	virtual void SendPacket(__int64 ClientID, CNPacket *pPacket) = 0;
	virtual void OnClientJoin(CSession *pSession, __int64 ClientID) = 0;   // Accept �� ����ó�� �Ϸ� �� ȣ��.
	virtual void OnClientLeave(__int64 ClientID) = 0;   						// Disconnect �� ȣ��
	virtual bool OnConnectionRequest(WCHAR *ClientIP, int Port) = 0;				// accept ����

	virtual void OnRecv(__int64 ClientID, CNPacket *pPacket) = 0;						// ��Ŷ ���� �Ϸ� ��
	virtual void OnSend(__int64 ClientID, int sendsize) = 0;					// ��Ŷ �۽� �Ϸ� ��

	virtual void OnWorkerThreadBegin() = 0;								// ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ��
	virtual void OnWorkerThreadEnd() = 0;								// ��Ŀ������ 1���� ���� ��

	virtual void OnError(int errorCode, WCHAR *errorString) = 0;
	
private :
	static unsigned __stdcall WorkerThread(LPVOID workerArg);
	static unsigned __stdcall AcceptThread(LPVOID acceptArg);

	void RecvPost(CSession *pSession);
	void SendPost(CSession *pSession);

public :
	int WorkerThead_Update(LPVOID workerArg);
	int AcceptThead_Update(LPVOID acceptArg);

protected :
	HANDLE hIOCP;

	HANDLE hAcceptThread;
	HANDLE hWorkerThread[MAX_THREAD];

	SOCKET listen_sock;

	int iWorkerThdNum;
	__int64 iSessionID;

	int iSessionCount;
};

#endif