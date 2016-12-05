/*
# 서버간 통신 모듈 설계

클라역할 - CLanClient

서버역할 - CLanServer


위의 이름으로 또는 이와 비슷한 이름으로 LAN 통신 (내부 네트워크 통신) 유저-서버 통신 모듈과는 구분 되도록 명침.

강제 사항은 아니지만 남들도 보기 쉽게 이름을 정하는게 매너.


- 설계 가이드.

* 너무 복잡한 클래스 설계는 하지 않는다.
이는 간단한 설계부터 경험 후 필요한 경우에 복잡한 객체지향 구조를 가지는게 좋음.
의미없는 디자인패턴은 쓰지 않는다. 디자인 패턴의 남발은 속도저하 유지보수 비용 증가와 직결됨.

판매용 엔진이라면 어쩔 수 없이 소스를 감추고 사용자 코딩 영역을 분리하기 위해서
계층분리, 모듈화, 패턴적용 등을 하지만 이는 성능저하와 확장의 어려움이 있음.

* IOCP 모델을 사용함.

* 클래스 내부에 워커 스레드를 가짐.

* 접속,보내기 등은 모두 비동기로 처리. (요청 후 이벤트 함수로 통보)

* 이벤트 함수 (접속완료,받기완료,보내기완료) 는 순수가상 함수로 제작.

* 실제 사용부는 위 두개의 클래스를 상속 사용하도록 제작.

* 내부에서 자체적으로 세션 연결을 관리함.



CLanServer


- bool Start(...) 오픈 IP / 포트 / 워커스레드 수 / 나글옵션 / 최대접속자 수
- void Stop(...)
- int GetClientCount(...)
- SendPacket(ClientID, Packet *)

virtual void OnClientJoin(Client 정보 / ClientID / 기타등등) = 0;   < Accept 후 접속처리 완료 후 호출.
virtual void OnClientLeave(ClientID) = 0;   	            < Disconnect 후 호출
virtual bool OnConnectionRequest(ClientIP,Port) = 0;        < accept 직후
return false; 시 클라이언트 거부.
return true; 시 접속 허용

virtual void OnRecv(ClientID, CPacket *) = 0;              < 패킷 수신 완료 후
virtual void OnSend(ClientID, int sendsize) = 0;           < 패킷 송신 완료 후

virtual void OnWorkerThreadBegin() = 0;                    < 워커스레드 GQCS 바로 하단에서 호출
virtual void OnWorkerThreadEnd() = 0;                      < 워커스레드 1루프 종료 후

virtual void OnError(int errorcode, wchar *) = 0;

*/

#ifndef __LANSERVER__H__
#define __LANSERVER__H__

#define MAX_SESSION 100
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
	BOOL Start(WCHAR *wOpenIP, int iPort, int iWorkerThdNum, BOOL bNagle, int iMaxConnection); //오픈 IP / 포트 / 워커스레드 수 / 나글옵션 / 최대접속자 수	
	void Stop();

	int GetClientCount();

	virtual void SendPacket(__int64 ClientID, CNPacket *pPacket) = 0;
	virtual void OnClientJoin(CSession *pClient, __int64 ClientID) = 0;   // Accept 후 접속처리 완료 후 호출.
	virtual void OnClientLeave(__int64 ClientID) = 0;   						// Disconnect 후 호출
	virtual bool OnConnectionRequest(WCHAR *ClientIP, int Port) = 0;				// accept 직후

	virtual void OnRecv(__int64 ClientID, CNPacket *pPacket) = 0;						// 패킷 수신 완료 후
	virtual void OnSend(__int64 ClientID, int sendsize) = 0;					// 패킷 송신 완료 후

	virtual void OnWorkerThreadBegin() = 0;								// 워커스레드 GQCS 바로 하단에서 호출
	virtual void OnWorkerThreadEnd() = 0;								// 워커스레드 1루프 종료 후

	virtual void OnError(int errorCode, WCHAR *errorString) = 0;
	
private :
	static unsigned __stdcall WorkerThread(LPVOID workerArg);
	static unsigned __stdcall AcceptThread(LPVOID acceptArg);

	void RecvPost();
	void SendPost();

public :
	void WorkerThead_Update(LPVOID workerArg);
	void AcceptThead_Update(LPVOID acceptArg);

protected :
	HANDLE hIOCP;

	HANDLE hAcceptThread;
	HANDLE hWorkerThread[MAX_THREAD];

	SOCKET listen_sock;

	CSession Session[MAX_SESSION];

	int iWorkerThdNum;
	__int64 iSessionID;
};

#endif