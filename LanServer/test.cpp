#include <Windows.h>
#include <stdio.h>

#include "StreamQueue.h"
#include "NPacket.h"
#include "LanServer.h"
#include "LanServer_test.h"

CLanServerTest LanServer;

void main()
{
	if (!LanServer.Start(SERVER_IP, SERVER_PORT, 1, false, 100))
		return;

	while (1)
	{
		wprintf(L"main\n");
		Sleep(1000);
	}
}