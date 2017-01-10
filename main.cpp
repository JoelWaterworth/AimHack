#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <iostream>

#define TRIGGER_KEY 0x56 // The V Virtual Key

DWORD EntityList = 0x4A1B54;
DWORD LocalPlayer = 0xA72C8C;
DWORD LifeState = 0x25B;
DWORD Team = 0xF0;
DWORD CrossHairId = 0x2410;
DWORD EntitySize = 0x10;

struct Module {
	DWORD dwBase;
	DWORD dwSize;
};

class Debugger {
private:
	HANDLE __process;
	DWORD __pId;
public:
	bool Attach(char* ProcessName) {
		HANDLE PHANDLE = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		PROCESSENTRY32 CSENTRY;
		CSENTRY.dwFlags = sizeof(CSENTRY);
		do {
			if (!strcmp(CSENTRY.szExeFile, ProcessName)) {
				__pId = CSENTRY.th32ProcessID; //sets process' id
				CloseHandle(PHANDLE); //closes then opent handle
				__process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, __pId); //opens process
				return true; // operation was successful
			}
		}
		while (Process32Next(PHANDLE, &CSENTRY));
		return false; // operation failed (process was not found)
	}
	Module GetModule(char* ModuleName) {
		//same as in attach function, except for a dll in memory...
		HANDLE module = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, __pId);
		MODULEENTRY32 ModEntry;
		ModEntry.dwSize = sizeof(ModEntry);
		do {
			if (!strcmp(ModEntry.szModule, (char*)ModuleName)) {
				CloseHandle(module);
				return{ (DWORD)ModEntry.hModule, ModEntry.modBaseSize };
			}
		} while (Module32Next(module, &ModEntry));
		return{ (DWORD)NULL, (DWORD)NULL };
	}
	template<typename T> T read(DWORD Addr) {
		T __read;
		ReadProcessMemory(__process, (void*)Addr, &__read, sizeof(T), NULL);
		return __read;
	}
};

Debugger debugger;
DWORD dwClient, dwEngine;
static class Entity {
public:
	DWORD GetBaseEntity(int PlayerNumber) {
		return debugger.read<DWORD>(dwClient + EntityList + (EntitySize * PlayerNumber));
	}
	bool PlayerIsDead(int PlayerNumber) {
		DWORD BaseEntity = GetBaseEntity(PlayerNumber);
		if (BaseEntity) {
			return debugger.read<bool>(BaseEntity + LifeState);
		}
	}
	int GetTeam(int PlayerNumber) {
		DWORD BaseEntity = GetBaseEntity(PlayerNumber);
		if (BaseEntity) {
			return debugger.read<int>(BaseEntity + Team);
		}
	}
};

static class Game {
public:
	void GetPlayerBase(DWORD* retAddr) {
		(DWORD)*retAddr = debugger.read<DWORD>(dwClient + LocalPlayer);
	}
	int GetTeam(){ //retrieves current team (ct/t)
		DWORD PlayerBase;
		GetPlayerBase(&PlayerBase);
		if (PlayerBase)
			return debugger.read<int>(PlayerBase + Team);
	}
	void GetCrosshairId(int* retAddr) {
		DWORD PlayerBase;
		GetPlayerBase(&PlayerBase);
		if (PlayerBase)
			(int)*retAddr = debugger.read<int>(PlayerBase + CrossHairId) - 1;
	}
};

void Click(){
	mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
	Sleep(1);
	mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

DWORD WINAPI TriggerThead(LPVOID PARAMS) {
	while (1) {
		Sleep(1);
		if (GetAsyncKeyState(TRIGGER_KEY) < 0 && TRIGGER_KEY != 1) {
			int PlayerNumber = NULL;
			Game::GetCrosshairId(&PlayerNumber);
			if (PlayerNumber < 64 && PlayerNumber >= 0 && Entity::GetTeam(PlayerNumber) != Game::GetTeam() && Entity::PlayerIsDead(PlayerNumber) = false) {
				Click();
			}
		}
	}
}

int main() {

	Module Client, Engine;

	while (!debugger.Attach("csgo.exe")) {
		std::cout << ".";
		Sleep(500);
	}
	Client = debugger.GetModule("Client.dll");
	dwClient = Client.dwBase;
	Engine = debugger.GetModule("Engine.dll");
	dwEngine = Engine.dwBase;

	CreateThread(0, 0, &TriggerThead, 0, 0, 0);
	std::cout << "Trigger thread is running...";

	while (1) {
		if (GetAsyncKeyState(VK_F4)) {
			exit(0);
		}
	}
}