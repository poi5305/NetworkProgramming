#include <iostream>

#include <windows.h>
#include <list>
#include <map>



using namespace std;
#include "resource.h"
#define SERVER_PORT 7799
#define WM_SOCKET_NOTIFY (WM_USER + 1)









BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
int EditPrintf (HWND, TCHAR *, ...);
//=================================================================
//	Global Variables
//=================================================================
list<SOCKET> Socks;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	//theApp.Run();
	//AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0);
	return DialogBox(hInstance, MAKEINTRESOURCE(ID_MAIN), NULL, MainDlgProc);
}

struct g_para
{
	HWND *hwnd;
	UINT *Message;
	WPARAM *wParam;
	LPARAM *lParam;
};
g_para gp;

#include "httpd.hpp"

std::map<int, HTTPD> httpds;
BOOL CALLBACK MainDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	WSADATA wsaData;

	static HWND hwndEdit;
	static SOCKET msock, ssock;
	static struct sockaddr_in sa;
	int sfd;
	int err;
	
	std::vector<int> close_httpds;

	gp.hwnd = &hwnd;
	gp.Message = &Message;
	gp.wParam = &wParam;
	gp.lParam = &lParam;

	switch(Message) 
	{
		case WM_INITDIALOG:
			hwndEdit = GetDlgItem(hwnd, IDC_RESULT);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_LISTEN:

					WSAStartup(MAKEWORD(2, 0), &wsaData);

					//create master socket
					msock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

					if( msock == INVALID_SOCKET ) {
						EditPrintf(hwndEdit, TEXT("=== Error: create socket error ===\r\n"));
						WSACleanup();
						return TRUE;
					}

					err = WSAAsyncSelect(msock, hwnd, WM_SOCKET_NOTIFY, FD_ACCEPT | FD_CLOSE | FD_READ | FD_WRITE);

					if ( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
						closesocket(msock);
						WSACleanup();
						return TRUE;
					}

					//fill the address info about server
					sa.sin_family		= AF_INET;
					sa.sin_port			= htons(SERVER_PORT);
					sa.sin_addr.s_addr	= INADDR_ANY;

					//bind socket
					err = bind(msock, (LPSOCKADDR)&sa, sizeof(struct sockaddr));

					if( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: binding error ===\r\n"));
						WSACleanup();
						return FALSE;
					}

					err = listen(msock, 2);
		
					if( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: listen error ===\r\n"));
						WSACleanup();
						return FALSE;
					}
					else {
						EditPrintf(hwndEdit, TEXT("=== Server START ===\r\n"));
					}

					break;
				case ID_EXIT:
					EndDialog(hwnd, 0);
					break;
			};
			break;

		case WM_CLOSE:
			EndDialog(hwnd, 0);
			break;

		case WM_SOCKET_NOTIFY:
			switch( WSAGETSELECTEVENT(lParam) )
			{
				case FD_ACCEPT:
					ssock = accept(msock, NULL, NULL);
					Socks.push_back(ssock);
					EditPrintf(hwndEdit, TEXT("=== Accept one new client(%d - %d), List size:%d ===\r\n"), ssock, wParam, Socks.size());
					break;
				case FD_READ:
					
					if (httpds.find(ssock) == httpds.end())
					{	// new browser fd
						httpds[ssock].init(ssock);
					}
					else
					{  //not new browse fd, active client, exam httpd is close
						for (auto &httpds_pair : httpds)
						{
							//auto &sockfd = httpds_pair.first;
							if (httpds_pair.second.is_close)
								close_httpds.push_back(httpds_pair.first);
						}
					}
					EditPrintf(hwndEdit, TEXT("=== READ one new client(%d - %d), List size:%d ===\r\n"), ssock, wParam, Socks.size());
					break;
				case FD_WRITE:
					EditPrintf(hwndEdit, TEXT("=== WRITE one new client(%d - %d), List size:%d ===\r\n"), ssock, wParam, Socks.size());
					break;
				case FD_CLOSE:
					closesocket(wParam);
					if (httpds.find(wParam) != httpds.end())
					{
						httpds.erase(wParam);
					}
					EditPrintf(hwndEdit, TEXT("=== close one new client(%d - %d), List size:%d ===\r\n"), ssock, wParam, Socks.size());
					break;
				
			};
			for (auto &httpds_pair : httpds)
			{
				if (httpds_pair.second.is_cgi)
				{
					httpds_pair.second.change_fd();
					int state = httpds_pair.second.hw3.winrun(wParam, WSAGETSELECTEVENT(lParam));
					if (state == 1)
					{ // all finish
						close_httpds.push_back(httpds_pair.first);
						//httpds.erase(sockfd);
					}
				}
				if (httpds_pair.second.is_close)
					close_httpds.push_back(httpds_pair.first);
			}
			for (auto &sockfd : close_httpds)
			{
				closesocket(sockfd);
				httpds.erase(sockfd);
			}
			close_httpds.clear();
			
			break;
		
		default:
			return FALSE;

	};

	return TRUE;
}




int EditPrintf (HWND hwndEdit, TCHAR * szFormat, ...)
{
     TCHAR   szBuffer [1024] ;
     va_list pArgList ;

     va_start (pArgList, szFormat) ;
     wvsprintf (szBuffer, szFormat, pArgList) ;
     va_end (pArgList) ;

     SendMessage (hwndEdit, EM_SETSEL, (WPARAM) -1, (LPARAM) -1) ;
     SendMessage (hwndEdit, EM_REPLACESEL, FALSE, (LPARAM) szBuffer) ;
     SendMessage (hwndEdit, EM_SCROLLCARET, 0, 0) ;
	 return SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0); 
}