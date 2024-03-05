#include <windows.h>
#include <fstream>
#include <tchar.h>
#include <windowsX.h>
#include <time.h>
#include "resource.h"
#include <string>

using namespace std;

class CriticalSectionDlg
{
public:
	CriticalSectionDlg(void);
public:
	~CriticalSectionDlg(void);
	static BOOL CALLBACK DlgProc(HWND hWnd, UINT mes, WPARAM wp, LPARAM lp);
	static CriticalSectionDlg* ptr;
	void Cls_OnClose(HWND hwnd);
	BOOL Cls_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
	void Cls_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
};

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPTSTR lpszCmdLine, int nCmdShow)
{
	CriticalSectionDlg dlg;
	return DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, CriticalSectionDlg::DlgProc);
}


CriticalSectionDlg* CriticalSectionDlg::ptr = NULL;

CRITICAL_SECTION cs;

CriticalSectionDlg::CriticalSectionDlg(void)
{
	ptr = this;
}

CriticalSectionDlg::~CriticalSectionDlg(void)
{
	DeleteCriticalSection(&cs);
}

void CriticalSectionDlg::Cls_OnClose(HWND hwnd)
{
	EndDialog(hwnd, 0);
}

BOOL CriticalSectionDlg::Cls_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) 
{
	InitializeCriticalSection(&cs);
	return TRUE;
}

void MessageAboutError(DWORD dwError)
{
	LPVOID lpMsgBuf = NULL;
	TCHAR szBuf[300]; 

	BOOL fOK = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	if(lpMsgBuf != NULL)
	{
		wsprintf(szBuf, TEXT("Error %d: %s"), dwError, lpMsgBuf); 
		MessageBox(0 ,szBuf, TEXT("Message Error"), MB_OK | MB_ICONSTOP); 
		LocalFree(lpMsgBuf); 
	}
}

DWORD WINAPI Write(LPVOID lp)
{
	srand(time(0));
	EnterCriticalSection(&cs);
	ofstream out(TEXT("array.txt"));
	if(!out.is_open())
	{
		MessageAboutError(GetLastError());
		return 1;
	}
	int A[100];
	for(int i = 0; i < 100; i++)
	{
		A[i] = rand()%50;
		out << A[i] << ' ';
	}
	out.close();
	LeaveCriticalSection(&cs);
	MessageBox(0, TEXT("Поток записал информацию в файл"), TEXT("Критическая секция"), MB_OK);
	return 0;
}

DWORD WINAPI Read(LPVOID lp)
{
	EnterCriticalSection(&cs);
	ifstream in(TEXT("array.txt"));
	if(!in.is_open())
	{
		MessageAboutError(GetLastError());
		return 1;
	}
	int B[100];
	int sum = 0;
	for(int i = 0; i < 100; i++)
	{
		in >> B[i];
		sum += B[i];
	}
	in.close();
	LeaveCriticalSection(&cs);
	MessageBox(0, TEXT("Поток прочитал информацию из файла"), TEXT("Критическая секция"), MB_OK);
	TCHAR str[30];
	wsprintf(str, TEXT("Сумма чисел равна %d"), sum);
	MessageBox(0, str, TEXT("Критическая секция"), MB_OK);
	return 0;
}


DWORD WINAPI WriteToFiles(LPVOID lp)
{
	EnterCriticalSection(&cs);

	ifstream sourceFile("array.txt");
	if (!sourceFile.is_open())
	{
		MessageAboutError(GetLastError());
		LeaveCriticalSection(&cs);
		return 1;
	}
	string initialInfo;
	getline(sourceFile, initialInfo);

	const int N = 10; 
	for (int i = 0; i < N; i++)
	{
		string copyFileName = "wr_" + to_string(i) + ".txt";
		ofstream copyFile(copyFileName);
		if (!copyFile.is_open())
		{
			MessageAboutError(GetLastError());
			LeaveCriticalSection(&cs);
			return 1;
		}

		copyFile << initialInfo << endl;
		sourceFile.seekg(0);

		copyFile << sourceFile.rdbuf();

		copyFile.close();
	}

	sourceFile.close();

	LeaveCriticalSection(&cs);

	MessageBox(0, TEXT("Поток WriteToFiles создал копии файлов"), TEXT("Критическая секция"), MB_OK);

	return 0;
}

DWORD WINAPI ReadFromFiles(LPVOID lp)
{
	EnterCriticalSection(&cs);

	const int N = 10; 
	for (int i = 0; i < N; i++)
	{
		string copyFileName = "wr_" + to_string(i) + ".txt";
		ifstream copyFile(copyFileName);
		if (!copyFile.is_open())
		{
			MessageAboutError(GetLastError());
			LeaveCriticalSection(&cs);
			return 1;
		}

		ofstream resultFile("result.txt", ios::app);
		if (!resultFile.is_open())
		{
			MessageAboutError(GetLastError());
			LeaveCriticalSection(&cs);
			return 1;
		}

		resultFile << copyFile.rdbuf();

		resultFile.close();
		copyFile.close();
	}

	LeaveCriticalSection(&cs);

	MessageBox(0, TEXT("Поток ReadFromFiles скопировал содержимое файлов"), TEXT("Критическая секция"), MB_OK);

	return 0;
}

BOOL CALLBACK CriticalSectionDlg::DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		HANDLE_MSG(hwnd, WM_CLOSE, ptr->Cls_OnClose);
		HANDLE_MSG(hwnd, WM_INITDIALOG, ptr->Cls_OnInitDialog);
		HANDLE_MSG(hwnd, WM_COMMAND, ptr->Cls_OnCommand);
	}
	return FALSE;
}

void CriticalSectionDlg::Cls_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	if (id == IDC_BUTTON1)
	{
		HANDLE hThread = CreateThread(NULL, 0, WriteToFiles, 0, 0, NULL);
		CloseHandle(hThread);
		hThread = CreateThread(NULL, 0, ReadFromFiles, 0, 0, NULL);
		CloseHandle(hThread);
	}
}