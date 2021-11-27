// StudioServer.cpp: ���������� ����� ����� ��� ����������� ����������.
//

#include "stdafx.h"
#include <Windows.h>
#include <string>
#include <tchar.h>
#include <iostream>
#include <conio.h>
#include <regex>
#include <vector>

const std::string serviceUsernameSSH = "teacher";
const std::string servicePasswordSSH = "2122pass";

const std::string platformToolsPath = "\"/home/itschool/Programs/android-sdk/platform-tools/";

std::string ExecCmd(const wchar_t* cmd)
{
    std::string strResult;
    HANDLE hPipeRead, hPipeWrite;

    SECURITY_ATTRIBUTES saAttr = {sizeof(SECURITY_ATTRIBUTES)};
    saAttr.bInheritHandle = TRUE; // Pipe handles are inherited by child process.
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe to get results from child's stdout.
    if (!CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0))
        return strResult;

    STARTUPINFOW si = {sizeof(STARTUPINFOW)};
    si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.hStdOutput  = hPipeWrite;
    si.hStdError   = hPipeWrite;
    si.wShowWindow = SW_HIDE; // Prevents cmd window from flashing.
                              // Requires STARTF_USESHOWWINDOW in dwFlags.

    PROCESS_INFORMATION pi = { 0 };

	LPTSTR szCmdline = _tcsdup(cmd);
	BOOL fSuccess = CreateProcess(NULL, szCmdline, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
    if (! fSuccess)
    {
        CloseHandle(hPipeWrite);
        CloseHandle(hPipeRead);
        return strResult;
    }

    bool bProcessEnded = false;
    for (; !bProcessEnded ;)
    {
        // Give some timeslice (50 ms), so we won't waste 100% CPU.
        bProcessEnded = WaitForSingleObject( pi.hProcess, 50) == WAIT_OBJECT_0;

        // Even if process exited - we continue reading, if
        // there is some data available over pipe.
        for (;;)
        {
            char buf[1024];
            DWORD dwRead = 0;
            DWORD dwAvail = 0;

            if (!::PeekNamedPipe(hPipeRead, NULL, 0, NULL, &dwAvail, NULL))
                break;

            if (!dwAvail) // No data available, return
                break;

            if (!::ReadFile(hPipeRead, buf, min(sizeof(buf) - 1, dwAvail), &dwRead, NULL) || !dwRead)
                // Error, the child process might ended
                break;

            buf[dwRead] = 0;
            strResult += buf;
        }
    } //for

    CloseHandle(hPipeWrite);
    CloseHandle(hPipeRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return strResult;
} //ExecCmd

const std::vector<const std::string> split(std::string str, const std::string splitStr)
{
	std::vector<const std::string> v;
	size_t position = 0;
	while ((position = str.find(splitStr)) != std::string::npos)
	{
		const std::string token = str.substr(0, position);
		v.push_back(token);
		str.erase(0, position + splitStr.length());
	}
	v.push_back(str.substr(0, position));
	return v;
}

void try_again_message(const std::string message)
{
	system("cls");
	std::cerr << message << std::endl << std::endl
		<< "������� ����� �������, ����� ��������� ������� �����������..." << std::endl << std::endl;
	_getch();
	system("cls");
}

void try_again_message(const std::string message, const std::string processName, const std::string systemMessage)
{
	system("cls");
	std::cerr << message << std::endl << std::endl
		<< "�������� ������ ��������� �� �������� " << processName << " �������������:" << std::endl
		<< std::endl << systemMessage << std::endl << std::endl
		<< "������� ����� �������, ����� ��������� ������� �����������..." << std::endl << std::endl;
	_getch();
	system("cls");
}

int _tmain(int argc, _TCHAR* argv[])
{
	//Prepare steps
	setlocale(LC_ALL, "Russian");

	std::cout << "�����������:" << std::endl <<
		"�� ����� �������� ����������� ����� �� �������� ������ ���� �������������!" << std::endl << std::endl;

	//Asking for server
	//Server choose by user
serverChoose:
	std::string serverIP = "192.168.0.";
	int serverIPEnding;

	std::cout << "������� ����� ��������� IP ������ ������ �������: 192.168.0.";
	std::cin >> serverIPEnding;

	system("cls");

	if(serverIPEnding < 1 || serverIPEnding > 254) 
	{
		std::cerr << "������� �������� ��������� ��� IP ������ �������. ���������� ��� ���." << std::endl << std::endl;
		goto serverChoose;
	}

	serverIP += std::to_string(serverIPEnding);

	//Begin of preparing
	adb_init:
	system("cls");
	std::cout << "����� � ���������� ������ ����������� Android Debug Bridge..." << std::endl;

	//Kill last launched adb server
	if (!ExecCmd(TEXT("\"C:\\adb\\adb\" kill-server")).empty()) {
		try_again_message("�� ���� ���������� ����������� ANDROID DEBUG BRIDGE.\n\n��������� ����� C:\\adb � ���� �� �����-�� ������� ��� ������ ���, �� ���������� � ���������� � �������� ������ ��� ��������� ��������� �������.");
		goto adb_init;
	}

	//Finding android device
	find_android_dev:
	system("cls");
	std::cout << "������ Android Debug Bridge � ����� ���������, ������������ � ��������..." << std::endl;
	auto devicesResponse = ExecCmd(TEXT("\"C:\\adb\\adb\" devices"));
	devicesResponse = devicesResponse.erase(0, 24);
	if (devicesResponse.find("device") == std::string::npos) {
		if (devicesResponse.find("unauthorized") != std::string::npos) {
			try_again_message("��������� \"��������\" ������� �� ����� �������� ��� ����� ����������, � ����� ���������� ��� ���.");
			goto adb_init;
		} else {
			try_again_message("���������� ��������� �� ������.\n\n���� �� ��� ����������� �� ������ USB, �� ��������� �������� �� � ���������� ������������ ������� �� USB. �����, ���������� �� ������ ��������: ����� �� �� ����������� � ����������?\n\n��������, �������� USB ������ ��� ������. ���� ��������, ��� �� � �������, ��...", "Android Debug Bridge �� ���� ��������", devicesResponse);
			goto find_android_dev;
		}
	}

	//���, ����������� �������� ID ����������. ������ �� ������������.
	/*std::regex rgx("\n(.*)\t");
	std::smatch sm;

	if (!std::regex_search(devicesResponse, sm, rgx)) {
		std::cerr << "�� ������� ���������� ID ������ ����������" << std::endl;
		std::cerr << "�������� ��� ��������� �������������:" << std::endl << std::endl;
		std::cerr << "adb devices returns:" << std::endl << devicesResponse << std::endl << std::endl;
		std::cerr << "������� ����� ������� ��� ���������� ������ ���������..." << std::endl;
		_getch();
		return EXIT_FAILURE;
	}

	std::string deviceID = sm[1];*/

	//Get device model
	//get_device_model:
	system("cls");
	std::cout << "��������� ������ ��������..." << std::endl;
	auto android_model = ExecCmd(TEXT("\"C:\\adb\\adb\" shell getprop ro.product.model"));
	//remove \r and \n that me receive from Android device
	android_model.pop_back(); //remove '\n'
	// '\r' sent randomly by Android. remove it by loop
	while (android_model[android_model.size() - 1] == '\r' && !android_model.empty())
	{
		android_model.pop_back();
	}
	if (android_model != "SM-P601" && android_model != "SM-T585")
	{
	method_choise:
		system("cls");
		std::cout << "�� ����������� ���������������� ������ �� ����������.\n���������� ������ �� �������������!\n\n�� ����� �������� ��� ������ ������ � Android �����������:\n1. ����� ��� Android 4.4.2;\n2. ����� ��� Android 8.1\n\n����� ������� �� ������ ������������?\n������� ��� ����� �����: ";
		char choise = _getch();
		switch(choise)
		{
		case '1':
			android_model = "SM-P601";
			break;
		case '2':
			android_model = "SM-T585";
			break;
		default:
			system("cls");
			try_again_message("�������� �����! ���������� ��� ���...");
			goto method_choise;
		}

	}

	//Get IP address
	get_android_ip:
	system("cls");
	std::cout << "��������� IP ������ � ��������..." << std::endl;

	std::string androidIP;

	if (android_model == "SM-P601")
	{
		auto netCfgResponse = ExecCmd(TEXT("\"C:\\adb\\adb\" shell netcfg"));
		if (netCfgResponse.find("wlan0    UP") == std::string::npos) {
			try_again_message("���������� ��������� �� ��������� ����������� � �������� ���� Wi-Fi.\n\n���������, ������� �� �� �������� Wi-Fi.\n\n���� �� �������, ��� � Wi-Fi �� � �������, ��...", "�������� ������������� �� ���������� ���������� (netcfg)", netCfgResponse);
			goto get_android_ip;
		}

		std::regex rgxNetCfg("wlan0    UP                               (.*)  0x0");
		std::smatch smNetCfg;

		if (!std::regex_search(netCfgResponse, smNetCfg, rgxNetCfg)) {
			try_again_message("�� ������� ���������� IP ����� ������ ����������", "�������� ������������� �� ���������� ���������� (netcfg)", netCfgResponse);
			goto adb_init;
		}

		//remove mask from ip
		std::string androidIPMask = smNetCfg[1];

		std::regex rgxIP("(.*)/");
		std::smatch smIP;
		if (!std::regex_search(androidIPMask, smIP, rgxIP)) {
			try_again_message("�� ������� ������� IP ����� ���������� �� ����� �������.\n\n��������, ���������� ���������������� ������ ���������� Android ����������.\n\n���������, ��� ������� Android ����� IP �����, � ������� ���:\n" + androidIPMask + "\n� ����� ������...");
			goto adb_init;
		}

		androidIP = smIP[1];
		//Removing spaces in IP
		std::string::iterator end_pos = std::remove(androidIP.begin(), androidIP.end(), ' ');
		androidIP.erase(end_pos, androidIP.end());
		if (androidIP.find("0.0.0.0") != std::string::npos) 
		{
			try_again_message("���������� ��������� �� ��������� ����������� � �������� ���� Wi-Fi.\n\n���������, ������� �� �� �������� Wi-Fi.\n\n���� �� �������, ��� � Wi-Fi �� � �������, ��...", "�������� ������������� �� ���������� ���������� (netcfg)", netCfgResponse);
			goto get_android_ip;
		}
	} else if (android_model == "SM-T585")
	{
		auto ifconfigResponse = ExecCmd(TEXT("\"C:\\adb\\adb\" shell ifconfig wlan0"));
		if (ifconfigResponse.find("inet addr:") == std::string::npos) {
			try_again_message("���������� ��������� �� ��������� � �������� ���� Wi-Fi.\n\n���������, ������� �� �� �������� Wi-Fi.\n\n���� �� �������, ��� � Wi-Fi �� � �������, ��...", "�������� ���������� �� ���������� ���������� (ifconfig wlan0)", ifconfigResponse);
			goto get_android_ip;
		}
		std::regex rgxIfconfig("inet addr:(.*)  Bcast:");
		std::smatch smIfconfig;

		if (!std::regex_search(ifconfigResponse, smIfconfig, rgxIfconfig)) {
			try_again_message("�� ������� ���������� IP ����� ������ ����������", "�������� ���������� �� ���������� ���������� (ifconfig wlan0)", ifconfigResponse);
			goto adb_init;
		}
		androidIP = smIfconfig[1];
	}

	if (androidIP.find("192.168.0.") == std::string::npos) 
	{
		try_again_message("���������� ��������� ��������� � ���� Wi-Fi, �� ��� �� �������� ����.\n\n���������� ��� � ���� IT_SAMSUNG_�����-��������.\n\n���� �� ��������� ������, �� ��������� ������������� ��� ������.");
		goto get_android_ip;
	}

	system("cls");
	std::cout << "������������ �������� � ����� ������ �� ����..." << std::endl;
	
	adb_to_tcp:
	//Configure ADB to TCP mode
	auto tcpIPResponse = ExecCmd(TEXT("\"C:\\adb\\adb\" tcpip 5555"));
	if (android_model == "SM-P601")
	{
		if (tcpIPResponse.find("restarting in TCP mode port: 5555") == std::string::npos) 
		{
			try_again_message("Android Debug Bridge �� ����� �������� �� ������� � ����� ������ �� ������������ ����.", "Android Debug Bridge �� ���� ����������", tcpIPResponse);
			goto adb_to_tcp;
		}
	} else if (android_model == "SM-T585")
	{
		if (tcpIPResponse.find("error: device '(null)' not found") != std::string::npos) 
		{
			try_again_message("Android Debug Bridge �� ����� �������� �� ������� � ����� ������ �� ������������ ����.", "Android Debug Bridge �� ���� ����������", tcpIPResponse);
			goto adb_to_tcp;
		}
	}

	//Connect android device to server
	androidServerConnect:
	system("cls");

	std::string argSSHHeader = "echo (A) | \"C:\\adb\\sexec\" " + serviceUsernameSSH +"@" + serverIP + "-noRegistry -pw=" + servicePasswordSSH +" -cmd=";

	std::cout << "��������� ���������� � ��������� ��������..." << std::endl;

	std::string arg = argSSHHeader + platformToolsPath + "adb connect " + androidIP + "\"";
	std::wstring wArg = std::wstring(arg.begin(), arg.end());
	auto remoteAdbResponse = ExecCmd(wArg.c_str());
	if (tcpIPResponse.find("failed to authenticate to") != std::string::npos) 
	{
		try_again_message("��������� ������� �� USB \"��������\" �� ANDROID ���������� �� ������ ���.\n\n������ ������� ���������� ��������� ��� �� ��� ����� ����������, � ��� ��������� �������.\n\n���� �� ��� �� �����������, �� ������ ���������� �����������, ��...", "Android Debug Bridge �� �������� �������", tcpIPResponse);
		goto androidServerConnect;
	}

	bool isAndroidReconnect = false;
	androidReconnect:
	if (isAndroidReconnect)
	{
		system("cls");
		std::cout << "�� ������ ��� �� �������� ��������� ������� \"��������\"!\n\n� ���� ��� ��� �������� ��� ��������� ����������� � ������� �� Wi-Fi.\n\n���� ������� �� ����� ������ �� ������� ���� ��� ��������� ������� ������������, �� �������������� USB � ������������� ��� ���������." << std::endl << std::endl
		<< "����� ������� �� ����� ������� ��� ��� ����� ������� �����������..." << std::endl;
		_getch();
	}

	//Check that's all OK
	system("cls");
	std::cout << "����������� �������� ������������ �����������..." << std::endl;
	//very bad
	std::string arg_ok = argSSHHeader + platformToolsPath + "adb devices\"";
	std::wstring wArg_ok = std::wstring(arg_ok.begin(), arg_ok.end());
	auto responseServerDevices = ExecCmd(wArg_ok.c_str());
	auto vector_general = split(responseServerDevices, "List of devices attached\n");
	if (vector_general.size() == 1)
	{
		try_again_message("����������� ����� �� �������!", "Android Debug Bridge �� �������� �������", responseServerDevices);
		goto androidServerConnect;
	}
	auto devicesOnServerStr = vector_general[1];
	auto devicesOnServerArray = split(devicesOnServerStr, "\n");
	if (devicesOnServerArray.size() == 1)
	{
		try_again_message("��� ������� �� ���� ������������ � ANDROID DEBUG BRIDGE �� �������� �������.\n\n���� ������ ����������� ����� ���, ��...", "Android Debug Bridge �� �������� �������", responseServerDevices);
		goto androidServerConnect;
	}
	bool deviceOnServerFound = false;
	system ("cls");
	for (auto &device : devicesOnServerArray)
	{
		auto deviceInfoArray = split(device, ":5555\t");
		if (deviceInfoArray.size() == 1) break; //just skip last new line
		if (deviceInfoArray[0] == androidIP)
		{
			if (deviceInfoArray[1] == "device")
			{
				//All ok
			} else
			{
				if (deviceInfoArray[1] == "unauthorized")
				{
					if (!isAndroidReconnect)
					{
						isAndroidReconnect = true;
						goto androidReconnect;
					}
					userChoice:
					std::cout << "Android Debug Bridge �� ������� ��� � �� ������� ����� �� ������ � ��������." << std::endl << std::endl << "��� ����� ��������� ����� ���������:\n\n1) ����������� �������������� ������� � �������; \n2) �������������� USB � ��������, � ����� ��������� ������ ��������������� ����������.\n\n��� �����: ";
					char userChoose = _getch();
					std::cout << std::endl << std::endl;
					switch(userChoose)
					{
					case '1':
						std::cout << "���������� ������� �������������� ������� � �������..." << std::endl;
						Sleep(1000);
						goto androidServerConnect;
						break;
					case '2':
						std::cout << "�������������� ������� � USB, ����� ������� ����� �������, ����� ����������..." << std::endl;
						_getch();
						goto adb_init;
						break;
					default:
						std::cout << "������������ �����. ���������� ��� ���..." << std::endl << std::endl;
						goto userChoice;
					}
				} else 
				{
					if (deviceInfoArray[1] == "offline")
					{
						try_again_message("ANDROID DEBUG BRIDGE �� ������� ��������, ��� ������� �� ����� � ����� ������ �� ����.\n\n�� �������� ��������� � �������� Wi-Fi.\n\n������� ������ ����� ��������� ������ �����������: ����������� � \"��������\".");
						goto adb_init;
					} else
					{
						try_again_message("��������� ����������� ������ ANDROID DEBUG BRIDGE.\n\n���������� ��������� ������ USB � ���������� ��� ������, ����� ������� ����� ������� ����� ��������� ��������� ����������� ������.\n\n���� ��� �� ��������, ��...", "Android Debug Bridge �� �������� �������", "Android ���������� ��������� � ��������� " + deviceInfoArray[1]);
						goto adb_init;
					}
				}
			}
			deviceOnServerFound = true;
			break;
		}
	}
	if (!deviceOnServerFound)
	{
		try_again_message("�� �����-�� ������� ���� ���������� �� ������������ � ��������� �������.\n\n���������� ��� ���.\n\n��������������, � ���, ��� �� �������� ������������� �����, ��������� Wi-Fi � ��� ������� �� �����������.\n\n����� ���� ���� ��������, ����������� USB ������ �� �������� � ������������ �������.");
		goto adb_init;
	}

	std::cout << "������� ������� ��������ͨ� � ������ ������� �� Wi-FI!" << std::endl << std::endl;
	std::cout << "IP ����� ������ ANDROID ���������� �� �������: " << androidIP << std::endl;
	std::cout << "������ ������ ANDROID ���������� �� �������: SAMSUNG " << android_model << std::endl << std::endl;
	std::cout << "��������� ������ IP ����� (��� ������, ���� IP �� ������������) � ������ ��������� ��� � Android Studio ��� ������� ������ ����������!" << std::endl << std::endl;
	std::cout << "����������, ��������� USB ������ �������� �� ��������.\n\n����������� ������� ������ ��� �������!" << std::endl << std::endl;
	std::cout << "��� ������ �� ��������� ������ � ��������, ����������, ������� �� ����� ������� ��� ��������������� ������������ �������� �� �������. ��� ����� ��� ����������� ����������� �������� � ��������� ���." << std::endl << std::endl;
	std::cout << "�������� ������� ����� �������... ";
	_getch();
	system("cls");
	std::cout << "���������� ����������� ���������� �� �������..." << std::endl;
	std::string argD = argSSHHeader + platformToolsPath + "adb disconnect " + androidIP + "\"";
	std::wstring wArgD = std::wstring(argD.begin(), argD.end());
	auto disconnectResponse = ExecCmd(wArgD.c_str());
	if (disconnectResponse.find("disconnected") == std::string::npos)
	{
		system("cls");

		std::string serverIPForClose = "192.168.0.";

		std::cout << "������� ����� ��������� IP ������ ������ �������: 192.168.0.";
		std::cin >> serverIPEnding;

		system("cls");

		if(serverIPEnding < 1 || serverIPEnding > 254) 
		{
			std::cerr << "������� �������� ��������� ��� IP ������ �������. ���������� ��� ���." << std::endl << std::endl;
			goto serverChoose;
		}

		serverIP += std::to_string(serverIPEnding);

		std::cout << "������ �� ��������� ������������� � ���� ����������." << std::endl << "��������, ������� ����������� � ������� �������." << std::endl << "���� �� ������������� ���������� ������ ������� � ������� �������, �� ������� ��� IP ����� �����, ����� ��������� ��������� ���������� �� ����: 192.168.0.";
		std::string argDL = "\"C:\\adb\\sexec\" teacher@" + servers[i] + " -hostKeyFile=C:\\adb\\Server" + std::to_string(i+1) + ".pub -pw=sudo19 -cmd=\"/home/general/android-sdk/platform-tools/adb disconnect " + androidIP + "\"";
		
		std::wstring wArgDL = std::wstring(argDL.begin(), argDL.end());
		ExecCmd(wArgDL.c_str());
	}
	system("cls");
	std::cout << "������� Android Debug Bridge � ����� ������ �� USB..." << std::endl;
	ExecCmd(TEXT("\"C:\\adb\\adb\" usb"));

	return EXIT_SUCCESS;
}



