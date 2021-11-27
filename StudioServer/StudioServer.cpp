// StudioServer.cpp: определяет точку входа для консольного приложения.
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
		<< "Нажмите любую клавишу, чтобы повторить попытку подключения..." << std::endl << std::endl;
	_getch();
	system("cls");
}

void try_again_message(const std::string message, const std::string processName, const std::string systemMessage)
{
	system("cls");
	std::cerr << message << std::endl << std::endl
		<< "Покажите данное сообщение от процесса " << processName << " преподавателю:" << std::endl
		<< std::endl << systemMessage << std::endl << std::endl
		<< "Нажмите любую клавишу, чтобы повторить попытку подключения..." << std::endl << std::endl;
	_getch();
	system("cls");
}

int _tmain(int argc, _TCHAR* argv[])
{
	//Prepare steps
	setlocale(LC_ALL, "Russian");

	std::cout << "НАПОМИНАНИЕ:" << std::endl <<
		"Во время процесса подключения экран на планшете должен быть разблокирован!" << std::endl << std::endl;

	//Asking for server
	//Server choose by user
serverChoose:
	std::string serverIP = "192.168.0.";
	int serverIPEnding;

	std::cout << "Введите здесь окончание IP адреса своего сервера: 192.168.0.";
	std::cin >> serverIPEnding;

	system("cls");

	if(serverIPEnding < 1 || serverIPEnding > 254) 
	{
		std::cerr << "Введено неверное окончание для IP адреса сервера. Попробуйте ещё раз." << std::endl << std::endl;
		goto serverChoose;
	}

	serverIP += std::to_string(serverIPEnding);

	//Begin of preparing
	adb_init:
	system("cls");
	std::cout << "Поиск и завершение старых экземпляров Android Debug Bridge..." << std::endl;

	//Kill last launched adb server
	if (!ExecCmd(TEXT("\"C:\\adb\\adb\" kill-server")).empty()) {
		try_again_message("НА ЭТОМ КОМПЬЮТЕРЕ ОТСУТСТВУЕТ ANDROID DEBUG BRIDGE.\n\nПроверьте папку C:\\adb и если по какой-то причине там ничего нет, то скопируйте её содержимое с ноутбука соседа или школьного файлового сервера.");
		goto adb_init;
	}

	//Finding android device
	find_android_dev:
	system("cls");
	std::cout << "Запуск Android Debug Bridge и поиск устройств, подключенных к ноутбуку..." << std::endl;
	auto devicesResponse = ExecCmd(TEXT("\"C:\\adb\\adb\" devices"));
	devicesResponse = devicesResponse.erase(0, 24);
	if (devicesResponse.find("device") == std::string::npos) {
		if (devicesResponse.find("unauthorized") != std::string::npos) {
			try_again_message("Разрешите \"навсегда\" отладку на вашем планшете для этого компьютера, а затем попробуйте ещё раз.");
			goto adb_init;
		} else {
			try_again_message("ПЛАНШЕТНЫЙ КОМПЬЮТЕР НЕ НАЙДЕН.\n\nЕсли он уже подсоединен по кабелю USB, то проверьте включена ли в параметрах разработчика отладка по USB. Также, посмотрите на экране планшета: видит ли он подключение к компьютеру?\n\nВозможно, повреждён USB разъём или кабель. Если считаете, что всё в порядке, то...", "Android Debug Bridge на этом ноутбуке", devicesResponse);
			goto find_android_dev;
		}
	}

	//Код, позволяющий вытащить ID устройства. Сейчас не используется.
	/*std::regex rgx("\n(.*)\t");
	std::smatch sm;

	if (!std::regex_search(devicesResponse, sm, rgx)) {
		std::cerr << "Не удалось обнаружить ID вашего устройства" << std::endl;
		std::cerr << "Покажите это сообщение преподавателю:" << std::endl << std::endl;
		std::cerr << "adb devices returns:" << std::endl << devicesResponse << std::endl << std::endl;
		std::cerr << "Нажмите любую клавишу для завершения работы программы..." << std::endl;
		_getch();
		return EXIT_FAILURE;
	}

	std::string deviceID = sm[1];*/

	//Get device model
	//get_device_model:
	system("cls");
	std::cout << "Получение модели планшета..." << std::endl;
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
		std::cout << "Вы используете неподдерживаемое данным ПО устройство.\nКорректная работа не гарантирована!\n\nНа выбор доступно два метода работы с Android устройством:\n1. Метод для Android 4.4.2;\n2. Метод для Android 8.1\n\nКаким методом вы хотите подключиться?\nВведите ваш выбор здесь: ";
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
			try_again_message("Неверный выбор! Попробуйте ещё раз...");
			goto method_choise;
		}

	}

	//Get IP address
	get_android_ip:
	system("cls");
	std::cout << "Получение IP адреса с планшета..." << std::endl;

	std::string androidIP;

	if (android_model == "SM-P601")
	{
		auto netCfgResponse = ExecCmd(TEXT("\"C:\\adb\\adb\" shell netcfg"));
		if (netCfgResponse.find("wlan0    UP") == std::string::npos) {
			try_again_message("ПЛАНШЕТНЫЙ КОМПЬЮТЕР НЕ РАСПОЗНАЛ ПОДКЛЮЧЕНИЯ К ШКОЛЬНОЙ СЕТИ Wi-Fi.\n\nПроверьте, включен ли на планшете Wi-Fi.\n\nЕсли вы уверены, что с Wi-Fi всё в порядке, то...", "сетевого конфигуратора на планшетном компьютере (netcfg)", netCfgResponse);
			goto get_android_ip;
		}

		std::regex rgxNetCfg("wlan0    UP                               (.*)  0x0");
		std::smatch smNetCfg;

		if (!std::regex_search(netCfgResponse, smNetCfg, rgxNetCfg)) {
			try_again_message("НЕ УДАЛОСЬ ОБНАРУЖИТЬ IP АДРЕС ВАШЕГО УСТРОЙСТВА", "сетевого конфигуратора на планшетном компьютере (netcfg)", netCfgResponse);
			goto adb_init;
		}

		//remove mask from ip
		std::string androidIPMask = smNetCfg[1];

		std::regex rgxIP("(.*)/");
		std::smatch smIP;
		if (!std::regex_search(androidIPMask, smIP, rgxIP)) {
			try_again_message("НЕ УДАЛОСЬ ИЗВЛЕЧЬ IP АДРЕС УСТРОЙСТВА ИЗ МАСКИ ПОДСЕТИ.\n\nВозможно, подключено неподдерживаемое данной программой Android устройство.\n\nОжидалось, что система Android вернёт IP адрес, а вернула это:\n" + androidIPMask + "\nВ любом случае...");
			goto adb_init;
		}

		androidIP = smIP[1];
		//Removing spaces in IP
		std::string::iterator end_pos = std::remove(androidIP.begin(), androidIP.end(), ' ');
		androidIP.erase(end_pos, androidIP.end());
		if (androidIP.find("0.0.0.0") != std::string::npos) 
		{
			try_again_message("ПЛАНШЕТНЫЙ КОМПЬЮТЕР НЕ РАСПОЗНАЛ ПОДКЛЮЧЕНИЯ К ШКОЛЬНОЙ СЕТИ Wi-Fi.\n\nПроверьте, включен ли на планшете Wi-Fi.\n\nЕсли вы уверены, что с Wi-Fi всё в порядке, то...", "сетевого конфигуратора на планшетном компьютере (netcfg)", netCfgResponse);
			goto get_android_ip;
		}
	} else if (android_model == "SM-T585")
	{
		auto ifconfigResponse = ExecCmd(TEXT("\"C:\\adb\\adb\" shell ifconfig wlan0"));
		if (ifconfigResponse.find("inet addr:") == std::string::npos) {
			try_again_message("ПЛАНШЕТНЫЙ КОМПЬЮТЕР НЕ ПОДКЛЮЧЕН К ШКОЛЬНОЙ СЕТИ Wi-Fi.\n\nПроверьте, включен ли на планшете Wi-Fi.\n\nЕсли вы уверены, что с Wi-Fi всё в порядке, то...", "сетевого диспетчера на планшетном компьютере (ifconfig wlan0)", ifconfigResponse);
			goto get_android_ip;
		}
		std::regex rgxIfconfig("inet addr:(.*)  Bcast:");
		std::smatch smIfconfig;

		if (!std::regex_search(ifconfigResponse, smIfconfig, rgxIfconfig)) {
			try_again_message("НЕ УДАЛОСЬ ОБНАРУЖИТЬ IP АДРЕС ВАШЕГО УСТРОЙСТВА", "сетевого диспетчера на планшетном компьютере (ifconfig wlan0)", ifconfigResponse);
			goto adb_init;
		}
		androidIP = smIfconfig[1];
	}

	if (androidIP.find("192.168.0.") == std::string::npos) 
	{
		try_again_message("ПЛАНШЕТНЫЙ КОМПЬЮТЕР ПОДКЛЮЧЕН К СЕТИ Wi-Fi, НО ЭТО НЕ ШКОЛЬНАЯ СЕТЬ.\n\nПодключите его к сети IT_SAMSUNG_номер-кабинета.\n\nЕсли он потребует пароль, то попросите преподавателя его ввести.");
		goto get_android_ip;
	}

	system("cls");
	std::cout << "Переключение планшета в режим работы по сети..." << std::endl;
	
	adb_to_tcp:
	//Configure ADB to TCP mode
	auto tcpIPResponse = ExecCmd(TEXT("\"C:\\adb\\adb\" tcpip 5555"));
	if (android_model == "SM-P601")
	{
		if (tcpIPResponse.find("restarting in TCP mode port: 5555") == std::string::npos) 
		{
			try_again_message("Android Debug Bridge на вашем планшете не перешёл в режим работы по беспроводной сети.", "Android Debug Bridge на этом компьютере", tcpIPResponse);
			goto adb_to_tcp;
		}
	} else if (android_model == "SM-T585")
	{
		if (tcpIPResponse.find("error: device '(null)' not found") != std::string::npos) 
		{
			try_again_message("Android Debug Bridge на вашем планшете не перешёл в режим работы по беспроводной сети.", "Android Debug Bridge на этом компьютере", tcpIPResponse);
			goto adb_to_tcp;
		}
	}

	//Connect android device to server
	androidServerConnect:
	system("cls");

	std::string argSSHHeader = "echo (A) | \"C:\\adb\\sexec\" " + serviceUsernameSSH +"@" + serverIP + "-noRegistry -pw=" + servicePasswordSSH +" -cmd=";

	std::cout << "Установка соединения с выбранным сервером..." << std::endl;

	std::string arg = argSSHHeader + platformToolsPath + "adb connect " + androidIP + "\"";
	std::wstring wArg = std::wstring(arg.begin(), arg.end());
	auto remoteAdbResponse = ExecCmd(wArg.c_str());
	if (tcpIPResponse.find("failed to authenticate to") != std::string::npos) 
	{
		try_again_message("РАЗРЕШИТЕ ОТЛАДКУ ПО USB \"НАВСЕГДА\" НА ANDROID УСТРОЙСТВЕ ВО ВТОРОЙ РАЗ.\n\nДанную отладку необходимо разрешить уже не для этого компьютера, а для школьного сервера.\n\nЕсли вы уже всё подтвердили, но ошибка продолжает повторяться, то...", "Android Debug Bridge на школьном сервере", tcpIPResponse);
		goto androidServerConnect;
	}

	bool isAndroidReconnect = false;
	androidReconnect:
	if (isAndroidReconnect)
	{
		system("cls");
		std::cout << "ВО ВТОРОЙ РАЗ НА ПЛАНШЕТЕ РАЗРЕШИТЕ ОТЛАДКУ \"НАВСЕГДА\"!\n\nВ этот раз это делается для удалённого подключения к серверу по Wi-Fi.\n\nЕсли планшет не выдаёт запрос на отладку даже при следующей попытке подключиться, то переподключите USB и перезапустите эту программу." << std::endl << std::endl
		<< "Затем нажмите на любую клавишу для ещё одной попытки подключения..." << std::endl;
		_getch();
	}

	//Check that's all OK
	system("cls");
	std::cout << "Выполняется проверка правильности подключения..." << std::endl;
	//very bad
	std::string arg_ok = argSSHHeader + platformToolsPath + "adb devices\"";
	std::wstring wArg_ok = std::wstring(arg_ok.begin(), arg_ok.end());
	auto responseServerDevices = ExecCmd(wArg_ok.c_str());
	auto vector_general = split(responseServerDevices, "List of devices attached\n");
	if (vector_general.size() == 1)
	{
		try_again_message("НЕОЖИДАННЫЙ ОТВЕТ ОТ СЕРВЕРА!", "Android Debug Bridge на школьном сервере", responseServerDevices);
		goto androidServerConnect;
	}
	auto devicesOnServerStr = vector_general[1];
	auto devicesOnServerArray = split(devicesOnServerStr, "\n");
	if (devicesOnServerArray.size() == 1)
	{
		try_again_message("ВАШ ПЛАНШЕТ НЕ СМОГ ПОДКЛЮЧИТЬСЯ К ANDROID DEBUG BRIDGE НА ШКОЛЬНОМ СЕРВЕРЕ.\n\nЕсли ошибка повторяется много раз, то...", "Android Debug Bridge на школьном сервере", responseServerDevices);
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
					std::cout << "Android Debug Bridge на сервере так и не получил права на доступ к планшету." << std::endl << std::endl << "Это можно исправить двумя способами:\n\n1) Попробовать переподключить планшет к серверу; \n2) Переподключить USB к ноутбуку, а затем выполнить полное переподключение устройства.\n\nВаш выбор: ";
					char userChoose = _getch();
					std::cout << std::endl << std::endl;
					switch(userChoose)
					{
					case '1':
						std::cout << "Производим попытку переподключить планшет к серверу..." << std::endl;
						Sleep(1000);
						goto androidServerConnect;
						break;
					case '2':
						std::cout << "Переподключите планшет к USB, затем нажмите любую клавишу, чтобы продолжить..." << std::endl;
						_getch();
						goto adb_init;
						break;
					default:
						std::cout << "Неправильный выбор. Попробуйте ещё раз..." << std::endl << std::endl;
						goto userChoice;
					}
				} else 
				{
					if (deviceInfoArray[1] == "offline")
					{
						try_again_message("ANDROID DEBUG BRIDGE НА СЕРВЕРЕ СООБЩАЕТ, ЧТО ПЛАНШЕТ НЕ ВЫШЕЛ В РЕЖИМ РАБОТЫ ПО СЕТИ.\n\nНа планшете выключите и включите Wi-Fi.\n\nПланшет должен будет попросить вторую авторизацию: подтвердите её \"навсегда\".");
						goto adb_init;
					} else
					{
						try_again_message("ПРОИЗОШЛА НЕИЗВЕСТНАЯ ОШИБКА ANDROID DEBUG BRIDGE.\n\nПопробуйте отключить кабель USB и подключить его заново, затем нажмите любую клавишу чтобы повторить процедуру подключения заново.\n\nЕсли это не помогает, то...", "Android Debug Bridge на школьном сервере", "Android устройство находится в состоянии " + deviceInfoArray[1]);
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
		try_again_message("По какой-то причине ваше устройство НЕ ПОДКЛЮЧИЛОСЬ к школьному серверу.\n\nПопробуйте ещё раз.\n\nУдостоверьтесь, в том, что на планшете разблокирован экран, подключен Wi-Fi и нет запроса на авторизацию.\n\nПосле всех этих проверок, отсоедините USB кабель от ноутбука и присоедините обратно.");
		goto adb_init;
	}

	std::cout << "ПЛАНШЕТ УСПЕШНО ПОДСОЕДИНЁН К ВАШЕМУ СЕРВЕРУ ПО Wi-FI!" << std::endl << std::endl;
	std::cout << "IP АДРЕС ВАШЕГО ANDROID УСТРОЙСТВА НА СЕРВЕРЕ: " << androidIP << std::endl;
	std::cout << "МОДЕЛЬ ВАШЕГО ANDROID УСТРОЙСТВА НА СЕРВЕРЕ: SAMSUNG " << android_model << std::endl << std::endl;
	std::cout << "Запомните данный IP адрес (или модель, если IP не отображается) и всегда выбирайте его в Android Studio при запуске своего приложения!" << std::endl << std::endl;
	std::cout << "Пожалуйста, отключите USB вашего планшета от ноутбука.\n\nНепрерывная зарядка ВРЕДИТ его батарее!" << std::endl << std::endl;
	std::cout << "Как только вы закончите работу с сервером, пожалуйста, НАЖМИТЕ НА ЛЮБУЮ КЛАВИШУ для принудительного отсоединения планшета от сервера. Это нужно для корректного подключения планшета в следующий раз." << std::endl << std::endl;
	std::cout << "Ожидание нажатия любой клавиши... ";
	_getch();
	system("cls");
	std::cout << "Отключение планшетного компьютера от сервера..." << std::endl;
	std::string argD = argSSHHeader + platformToolsPath + "adb disconnect " + androidIP + "\"";
	std::wstring wArgD = std::wstring(argD.begin(), argD.end());
	auto disconnectResponse = ExecCmd(wArgD.c_str());
	if (disconnectResponse.find("disconnected") == std::string::npos)
	{
		system("cls");

		std::string serverIPForClose = "192.168.0.";

		std::cout << "Введите здесь окончание IP адреса своего сервера: 192.168.0.";
		std::cin >> serverIPEnding;

		system("cls");

		if(serverIPEnding < 1 || serverIPEnding > 254) 
		{
			std::cerr << "Введено неверное окончание для IP адреса сервера. Попробуйте ещё раз." << std::endl << std::endl;
			goto serverChoose;
		}

		serverIP += std::to_string(serverIPEnding);

		std::cout << "Сервер не обнаружил подключенного к нему устройства." << std::endl << "Возможно, планшет подключился к другому серверу." << std::endl << "Если вы действительно подключали данный планшет к другому серверу, то введите его IP адрес здесь, чтобы программа отключила устройство от него: 192.168.0.";
		std::string argDL = "\"C:\\adb\\sexec\" teacher@" + servers[i] + " -hostKeyFile=C:\\adb\\Server" + std::to_string(i+1) + ".pub -pw=sudo19 -cmd=\"/home/general/android-sdk/platform-tools/adb disconnect " + androidIP + "\"";
		
		std::wstring wArgDL = std::wstring(argDL.begin(), argDL.end());
		ExecCmd(wArgDL.c_str());
	}
	system("cls");
	std::cout << "Перевод Android Debug Bridge в режим работы по USB..." << std::endl;
	ExecCmd(TEXT("\"C:\\adb\\adb\" usb"));

	return EXIT_SUCCESS;
}



