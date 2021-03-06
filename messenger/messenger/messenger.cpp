#include "pch.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#define _WIN32_WINNT 0x501

#include <WinSock2.h>
#include <WS2tcpip.h>

// Необходимо, чтобы линковка происходила с DLL-библиотекой для работы с сокетам

#pragma comment(lib, "Ws2_32.lib")

using std::cerr;

int main() {
	WSADATA wsaData; // служебная структура для хранение информации о реализации Windows Sockets

	int result = WSAStartup(MAKEWORD(2, 2), &wsaData); // (подгружается Ws2_32.dll)

	// Если произошла ошибка подгрузки библиотеки
	if (result != 0) {
		cerr << "WSAStartup failed: " << result << "\n";
		return result;
	}

	struct addrinfo* addr = NULL; // структура, хранящая информацию об IP-адресе  слущающего сокета

	struct addrinfo hints; // Шаблон для инициализации структуры адреса
	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_INET; // AF_INET определяет, что будет использоваться сеть для работы с сокетом
	hints.ai_socktype = SOCK_STREAM; // Задаем потоковый тип сокета
	hints.ai_protocol = IPPROTO_IP; // Используем протокол TCP (автоматически)
	hints.ai_flags = AI_PASSIVE; // Сокет будет биндиться на адрес, чтобы принимать входящие соединения

	// Инициализируем структуру, хранящую адрес сокета - addr
	// Наш HTTP-сервер будет висеть на 8080-м порту локалхоста
	result = getaddrinfo("192.168.0.107", "8080", &hints, &addr);

	// Если инициализация структуры адреса завершилась с ошибкой, выведем сообщением об этом и завершим выполнение программы
	if (result != 0) {
		cerr << "getaddrinfo failed: " << result << "\n";
		WSACleanup(); // выгрузка библиотеки Ws2_32.dll
		return 1;
	}

	// Создание сокета
	int listen_socket = socket(addr->ai_family, addr->ai_socktype,
		addr->ai_protocol);
	// Если создание сокета завершилось с ошибкой, выводим сообщение, освобождаем память, выделенную под структуру addr, выгружаем dll-библиотеку и закрываем программу
	if (listen_socket == INVALID_SOCKET) {
		cerr << "Error at socket: " << WSAGetLastError() << "\n";
		freeaddrinfo(addr);
		WSACleanup();
		return 1;
	}

	// Привязываем сокет к IP-адресу
	result = bind(listen_socket, addr->ai_addr, (int)addr->ai_addrlen);

	// Если привязать адрес к сокету не удалось, то выводим сообщение об ошибке, освобождаем память, выделенную под структуру addr.
	// Закрываем открытый сокет.
	// Выгружаем DLL-библиотеку из памяти и закрываем программу.
	if (result == SOCKET_ERROR) {
		cerr << "bind failed with error: " << WSAGetLastError() << "\n";
		freeaddrinfo(addr);
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}

	// Инициализируем слушающий сокет
	if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
		cerr << "listen failed with error: " << WSAGetLastError() << "\n";
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}

	const int max_client_buffer_size = 30000;
	char buf[max_client_buffer_size];
	int client_socket = INVALID_SOCKET;

	//Входим в бесконечный цикл
	while (1) {
		// Принимаем входящие соединения
		client_socket = accept(listen_socket, NULL, NULL);
		if (client_socket == INVALID_SOCKET) { //Ловим ошибку, закрываем программу
			cerr << "accept failed: " << WSAGetLastError() << "\n";
			closesocket(listen_socket);
			WSACleanup();
			return 1;
		}

		//Ловим запрос юзера
		result = recv(client_socket, buf, max_client_buffer_size, 0);

		std::stringstream response; // сюда будет записываться ответ клиенту
		std::stringstream response_body; // тело ответа
		std::stringstream response2;
		std::stringstream response_body2;

		if (result == SOCKET_ERROR) {
			// ошибка получения данных
			cerr << "recv failed: " << result << "\n";
			closesocket(client_socket);
		}
		else if (result == 0) {
			// соединение закрыто клиентом
			cerr << "connection closed...\n";
		}
		else if (result > 0) {
			// Мы знаем фактический размер полученных данных, поэтому ставим метку конца строки в буфере запроса.
			buf[result] = '\0';

			// Данные успешно получены
			// формируем тело ответа (HTML)

			std::ifstream html("http.html", std::ios_base::ate); //считываем основной файл
			int size = html.tellg();
			html.seekg(0);
			char *buff = new char[size-4]; // буфер промежуточного хранения считываемого из файла текста
			html.read(buff, size-4);
			//std::ifstream{ pathToFile, std::ios::binary };
			std::ifstream img("img/pipetop.png", std::ios::binary);
			int sizei = img.tellg();
			img.seekg(0);
			char *buffer = new char[sizei];
			img.read(buffer, sizei);

			std::cout << buff << "\n";

			response_body  << buff;
			//response_body2 << buffer;

			// Формируем весь ответ вместе с заголовками
			response << "HTTP/1.1 200 OK\r\n"
				<< "Version: HTTP/1.1\r\n"
				<< "Content-Type: text/html; charset=utf-8\r\n"
				<< "Content-Length: " << response_body.str().length()
				<< "\r\n\r\n"
				<< response_body.str();

			// Отправляем ответ клиенту с помощью функции send
			result = send(client_socket, response.str().c_str(), response.str().length(), 0);
			//result = send(client_socket, response2.str().c_str(), response2.str().length(), 0);	

			//result = recv(client_socket, buf, max_client_buffer_size, 0);

			/*response2 << "HTTP/1.1 200 OK\r\n"
				<< "Content-Type: image/png\r\n"
				<< "Content-Length: " << response_body2.str().length()
				<< "\r\n\r\n"
				<< response_body2.str();

			result = send(client_socket, response2.str().c_str(), response2.str().length(), 0);*/


			if (result == SOCKET_ERROR) {
				// произошла ошибка при отправле данных
				cerr << "send failed: " << WSAGetLastError() << "\n";
			}
			// Закрываем соединение к клиентом
			closesocket(client_socket);
		}
	}

	// Убираем за собой
	closesocket(listen_socket);
	freeaddrinfo(addr);
	WSACleanup();
	return 0;
}