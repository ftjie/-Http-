#include <iostream>
#include <cstring>
#include <WinSock2.h> // window下的socket套接字头文件
#include <pthread.h>  // 添加包含目录与库目录时使用相对路径，使用绝对路径会导致在别的机器上无法跑通
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
	// 开启网络权限 
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET listen_sock = createSocket(9090);
	
	return WaitConnect(listen_sock);
}

SOCKET createSocket(int _Port)
{

	// 网络(TCP)(监听socket)
	// AF_INET: ipv4  
	// AF_INET6: ipv6  协议地址族
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in local = { 0 };
	local.sin_family = AF_INET;
	local.sin_port = htons(_Port); // 主机字节序转网络字节序
	local.sin_addr.S_un.S_addr = INADDR_ANY;

	//绑定
	if (bind(listen_sock, (struct sockaddr*)&local, sizeof(struct sockaddr)) == -1)
	{
		printf("绑定失败");
		exit(-1);
	}

	if (listen(listen_sock, 10) == -1)
	{
		printf("监听失败");
		exit(-1);
	}
	printf("绑定并且监听成功, 等待客户端连接\n");

	return listen_sock;
}

int WaitConnect(SOCKET _ListenSocket)
{
	struct sockaddr_in ClientAddr;
	int socklen = sizeof(struct sockaddr_in); 

	//线程分离 
	pthread_t thread; // 线程标识
	pthread_attr_t thread_attr; // 线程属性
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED); // 设置线程属性：线程分离
	//线程执行完毕后自动释放资源

	//浏览器自动发出请求图标（favicon.ico）的http请求，失败
	ThreadPool &pool = ThreadPool::GetInstance(4);
	while (1)
	{
		// 通过client_sock与客户端进行通讯, 当有客户端connect时，
		// accept函数有返回值，服务器通过select调用同时监听多个socket
		SOCKET client_sock = accept(_ListenSocket, (struct sockaddr*)&ClientAddr, &socklen);

		std::cout << "客户端ip与端口等相关信息：" << ClientAddr.sin_family << "  " << ClientAddr.sin_port
			<< std::endl;

		if (client_sock == -1)
		{
			continue;

		}

		// 连接成功,inet_ntoa 网络字节序ip地址转点分十进制ip地址
		//printf("%s %d\n", inet_ntoa(ClientAddr.sin_addr), ntohs(ClientAddr.sin_port));


		// 开线程
		SOCKET* temp = new SOCKET; // 防止线程未执行完毕，又新建一个socket，使该线程的socket失效
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

	// 接收数据
	char buff[BUFF_SIZE] = { 0 };
	int ret = recv(client_sock, buff, BUFF_SIZE, 0); // 阻塞的
	if (ret > 0)
	{

		// 分析报文
		std::string url = FromHttpPackageGetUrl(buff);
		std::string filecontent = "";
		std::string filetype = "";

		// 读取文件
		if (url == "/")
		{
			// 读取文件index.html文件内容
			std::ifstream file("index.html", std::ios::binary);
			if (file.is_open())
			{
				std::cout << "index.html文件打开成功" << std::endl;
				file.seekg(0, std::ios::end);
				filecontent.resize(file.tellg());
				file.seekg(0, std::ios::beg);

				// 读取文件
				file.read(&filecontent[0], filecontent.size());
				file.close();

				filetype = GetFileType("index.html");
			}
			else
			{
				std::cout << "index.html文件打开失败" << std::endl;
			}
		}
		else
		{
			url = url.substr(1);
			// 读取文件url文件内容
			std::ifstream file(url, std::ios::binary);
			if (file.is_open())
			{
				std::cout << "url文件打开成功" << std::endl;
				file.seekg(0, std::ios::end);
				filecontent.resize(file.tellg());
				file.seekg(0, std::ios::beg);

				// 读取文件
				file.read(&filecontent[0], filecontent.size());
				file.close();

				filetype = GetFileType(url);
			}
			else
			{
				std::cout << "url文件打开失败" << std::endl;
			}

		}


		// 组建HTTP包
		memset(buff, 0, BUFF_SIZE);
		sprintf_s(buff, "HTTP/1.0 200 OK\r\n\
				Content-Type: %s\r\n\
				Transfer-Encoding: chunked\r\n\
				Connection: Keep-Alive\r\n\
				Accept-Ranges:bytes\r\n\
				Content-Length:%d\r\n\r\n", filetype.c_str(), (int)filecontent.size());

		// 发送
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

	std::cout << "请求URL:   "<<_HttpPackage << std::endl;
	
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

	return "text/html"; // 都没有匹配上返回这个
}