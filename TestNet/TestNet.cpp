// TestNet.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Echo.h"
#include "WorkQueue.h"
#include "Packet.h"
#include "Manager.h"
#include "Log.h"
#include "winConsole.h"
#include <signal.h>
#include <vector>
#include <ctime>

using std::tr1::bind;
using namespace std::tr1::placeholders;

bool run = true;

BOOL WINAPI HandlerRoutine(DWORD ctrlTpye)
{
	run = false;
	return TRUE;
}

void processFunc(const char* cmd)
{
	ConsoleColor consoleColor(FOREGROUND_BLUE|FOREGROUND_INTENSITY);
	CONSOLE->printf("%s\n", cmd);
}

int _tmain(int argc, _TCHAR* argv[])
{
	WinConsole console("TestNet", bind(&processFunc, _1));
	CNetInit net;
	CPacket packet;
	CEcho echo(bind(&CPacket::Send, &packet, _1, _2, _3));
	CWorkQueue workQueu(bind(&CEcho::Process, &echo, _1, _2, _3), bind(&CEcho::End, &echo, _1));
	CManager manager(bind(&CPacket::Process, &packet, _1, _2, _3), bind(&CWorkQueue::End, &workQueu, _1));
	packet.Init(bind(&CWorkQueue::Post, &workQueu, _1, _2, _3), bind(&CManager::Send, &manager, _1, _2, _3));
	workQueu.Start(1);
	manager.Start();

	if (SetConsoleCtrlHandler(HandlerRoutine, TRUE) == 0)
		LOG("Fail to call SetConsoleCtrlHandler.");

	typedef std::vector<int> Clients;
	typedef Clients::iterator ClientsIter;
	manager.StartServer("192.168.2.102", 9999);
	Sleep(1000); //Server must wait for a moment to accept connection.
	Clients clients;
	for (int i=0; i<10; ++i)
	{
		int clientID = manager.AddClient("192.168.2.102", 9999);
		if (clientID == 0)
		{
			LOG("Fail to add client.");
			continue;
		}
		clients.push_back(clientID);
		echo.AddAsynClient(clientID);
	}
	int count = 0;
	char message[512];
	while (run)
	{
		console.process();
		static int lastSend = (int)time(NULL);
		int now = (int)time(NULL);
		if (now - lastSend > 1)
		{
			lastSend = now;
			++count;
			for (ClientsIter iter=clients.begin(); iter!=clients.end(); ++iter)
			{
				sprintf_s(message, sizeof(message), "Client[%d] send message[%d].", *iter, count);
				echo.Send(*iter, message, strlen(message)+1);
			}
		}
	}

	manager.Stop();
	workQueu.Stop();

	return 0;
}

