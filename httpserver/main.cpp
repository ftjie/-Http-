#include <iostream>
#include <cstring>
#include <WinSock2.h> // window�µ�socket�׽���ͷ�ļ�
#include <pthread.h>  // ��Ӱ���Ŀ¼���Ŀ¼ʱʹ�����·����ʹ�þ���·���ᵼ���ڱ�Ļ������޷���ͨ
#include <fstream>
#include <mutex>
#include <Condition_variable>
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include "ThreadPool.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "x64/pthreadVC2.lib")

#define BUFF_SIZE 1024 

//using namespace std;

SOCKET createSocket(int _Port);
int WaitConnect(SOCKET _ListenSocket); 
void* HttpThread(void* arg);
std::string FromHttpPackageGetUrl(std::string _HttpPackage);
std::string GetFileType(std::string _filename);

int main()
{
	// ��������Ȩ�� 
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET listen_sock = createSocket(9090);
	
	return WaitConnect(listen_sock);
}

SOCKET createSocket(int _Port)
{

	// ����(TCP)(����socket)
	// AF_INET: ipv4  
	// AF_INET6: ipv6  Э���ַ��
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in local = { 0 };
	local.sin_family = AF_INET;
	local.sin_port = htons(_Port); // �����ֽ���ת�����ֽ���
	local.sin_addr.S_un.S_addr = INADDR_ANY;

	//��
	if (bind(listen_sock, (struct sockaddr*)&local, sizeof(struct sockaddr)) == -1)
	{
		printf("��ʧ��");
		exit(-1);
	}

	if (listen(listen_sock, 10) == -1)
	{
		printf("����ʧ��");
		exit(-1);
	}
	printf("�󶨲��Ҽ����ɹ�, �ȴ��ͻ�������\n");

	return listen_sock;
}

int WaitConnect(SOCKET _ListenSocket)
{
	struct sockaddr_in ClientAddr;
	int socklen = sizeof(struct sockaddr_in); 

	//�̷߳��� 
	pthread_t thread; // �̱߳�ʶ
	pthread_attr_t thread_attr; // �߳�����
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED); // �����߳����ԣ��̷߳���
	//�߳�ִ����Ϻ��Զ��ͷ���Դ

	//������Զ���������ͼ�꣨favicon.ico����http����ʧ��
	ThreadPool &pool = ThreadPool::GetInstance(4);
	while (1)
	{
		// ͨ��client_sock��ͻ��˽���ͨѶ, ���пͻ���connectʱ��
		// accept�����з���ֵ��������ͨ��select����ͬʱ�������socket
		SOCKET client_sock = accept(_ListenSocket, (struct sockaddr*)&ClientAddr, &socklen);

		std::cout << "�ͻ���ip��˿ڵ������Ϣ��" << ClientAddr.sin_family << "  " << ClientAddr.sin_port
			<< std::endl;

		if (client_sock == -1)
		{
			continue;

		}

		// ���ӳɹ�,inet_ntoa �����ֽ���ip��ַת���ʮ����ip��ַ
		//printf("%s %d\n", inet_ntoa(ClientAddr.sin_addr), ntohs(ClientAddr.sin_port));


		// ���߳�
		SOCKET* temp = new SOCKET; // ��ֹ�߳�δִ����ϣ����½�һ��socket��ʹ���̵߳�socketʧЧ
		*temp = client_sock;
		pool.enqueue(HttpThread, temp); // Reactor
 		//pthread_create(&thread, &thread_attr, HttpThread, temp);
	}

}

void* HttpThread(void* arg)
{
	if (arg == nullptr)
		return nullptr;

	SOCKET client_sock = *((SOCKET*)arg);
	delete (SOCKET*)arg;

	// ��������
	char buff[BUFF_SIZE] = { 0 };
	int ret = recv(client_sock, buff, BUFF_SIZE, 0); // ������
	if (ret > 0)
	{

		// ��������
		std::string url = FromHttpPackageGetUrl(buff);
		std::string filecontent = "";
		std::string filetype = "";

		// ��ȡ�ļ�
		if (url == "/")
		{
			// ��ȡ�ļ�index.html�ļ�����
			std::ifstream file("index.html", std::ios::binary);
			if (file.is_open())
			{
				std::cout << "index.html�ļ��򿪳ɹ�" << std::endl;
				file.seekg(0, std::ios::end);
				filecontent.resize(file.tellg());
				file.seekg(0, std::ios::beg);

				// ��ȡ�ļ�
				file.read(&filecontent[0], filecontent.size());
				file.close();

				filetype = GetFileType("index.html");
			}
			else
			{
				std::cout << "index.html�ļ���ʧ��" << std::endl;
			}
		}
		else
		{
			url = url.substr(1);
			// ��ȡ�ļ�url�ļ�����
			std::ifstream file(url, std::ios::binary);
			if (file.is_open())
			{
				std::cout << "url�ļ��򿪳ɹ�" << std::endl;
				file.seekg(0, std::ios::end);
				filecontent.resize(file.tellg());
				file.seekg(0, std::ios::beg);

				// ��ȡ�ļ�
				file.read(&filecontent[0], filecontent.size());
				file.close();

				filetype = GetFileType(url);
			}
			else
			{
				std::cout << "url�ļ���ʧ��" << std::endl;
			}

		}


		// �齨HTTP��
		memset(buff, 0, BUFF_SIZE);
		sprintf_s(buff, "HTTP/1.0 200 OK\r\n\
				Content-Type: %s\r\n\
				Transfer-Encoding: chunked\r\n\
				Connection: Keep-Alive\r\n\
				Accept-Ranges:bytes\r\n\
				Content-Length:%d\r\n\r\n", filetype.c_str(), (int)filecontent.size());

		// ����
		send(client_sock, buff, strlen(buff), 0);
		send(client_sock, filecontent.c_str(), (int)filecontent.size(), 0);
	}
	closesocket(client_sock);

	return NULL;
}

std::string FromHttpPackageGetUrl(std::string _HttpPackage)
{  
	std::string url = "";
	int start = _HttpPackage.find('/');
	int end = _HttpPackage.find(' ', start);

	url = _HttpPackage.substr(start, end - start);

	std::cout << "����URL:   "<<_HttpPackage << std::endl;
	
	return url;
}

std::string GetFileType(std::string _filename)
{
	size_t start = _filename.find('.');
	std::string Suffix = _filename.substr(start + 1);

	if (Suffix == "bmp")
		return "image/bmp";
	else if (Suffix == "gif")
		return "image/gif";
	else if (Suffix == "ico")
		return "image/x-icon";
	else if (Suffix == "jpg")
		return "image/jpeg";
	else if (Suffix == "avi")
		return "video/avi";
	else if (Suffix == "css")
		return "text/css";
	else if (Suffix == "dll" || Suffix == "exe")
		return "application/x-msdownload";
	else if (Suffix == "dtd")
		return "text/xml";
	else if (Suffix == "mp3")
		return "audio/mp3";
	else if (Suffix == "mpg")
		return "video/mpg";
	else if (Suffix == "png")
		return "image/png";
	else if (Suffix == "xls")
		return "application/vnd.ms-excel";
	else if (Suffix == "doc")
		return "application/msword";
	else if (Suffix == "mp4")
		return "video/mpeg4";
	else if (Suffix == "ppt")
		return "application/x-ppt";
	else if (Suffix == "wma")
		return "audio/x-ms-wma";
	else if (Suffix == "wmv")
		return "video/x-ms-wmv";

	return "text/html"; // ��û��ƥ���Ϸ������
}