#include <WinSock2.h>
#include <MSWSock.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <codecvt>
#include "Service.h"
#include "EnvVar.h"
#include "IoObj.h"
#include "FileObj.h"

using namespace std;

void handleLOGIN(LPSESSION session, char *username, char *password, char *reply) {
	string userName = username;
	string passWord = password;

	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	string query = "SELECT * FROM Account where username='" + userName + "'";
	wstring wstr = converter.from_bytes(query);

	if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLWCHAR*)wstr.c_str(), SQL_NTS)) {
		cout << "Error querying SQL Server" << endl;
		wcout << wstr;
		return;
	}

	SQLCHAR sqlUsername[50];
	SQLCHAR sqlPassword[50];
	SQLCHAR sqlStatus[50];

	if (userName.size() == 0 || passWord.size() == 0) {
		initParam(reply, EMPTY_FIELD, "Empty field");
	}
	else if (SQLFetch(gSqlStmtHandle) == SQL_SUCCESS) {
		SQLGetData(gSqlStmtHandle, 1, SQL_CHAR, sqlUsername, sizeof(sqlUsername), NULL);
		SQLGetData(gSqlStmtHandle, 2, SQL_CHAR, sqlPassword, sizeof(sqlPassword), NULL);
		SQLGetData(gSqlStmtHandle, 3, SQL_CHAR, sqlStatus, sizeof(sqlStatus), NULL);

		string strSqlPassword = reinterpret_cast<char*>(sqlPassword);
		string strSqlStatus = reinterpret_cast<char*>(sqlStatus);

		SQLCloseCursor(gSqlStmtHandle);

		if (passWord == strSqlPassword) {
			if (strSqlStatus == "0") {
				query = "UPDATE Account SET status = 1 WHERE username='" + userName + "';";
				wstr = converter.from_bytes(query);
				wcout << wstr << endl;
				if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLWCHAR*)wstr.c_str(), SQL_NTS)) {
					cout << "Error querying SQL Server";
					cout << "\n";
				}
				else {
					//Set user name
					session->setUsername(userName.c_str());
					//Set working dir
					session->setWorkingDir(userName.c_str());
					initParam(reply, LOGIN_SUCCESS, "Login success");
				}
			}
			else {
				initParam(reply, ALREADY_LOGIN, "Login failed. Already logged in");
			}
		}
		else {
			initParam(reply, WRONG_PASSWORD, "Login failed. Wrong password");
		}
	}
	else {
		initParam(reply, USER_NOT_EXIST, "Login failed. Username doesn't exist");
		SQLCloseCursor(gSqlStmtHandle);
	}
}

void handleLOGOUT(LPSESSION session, char *reply) {
	string username = session->username;

	if (username.length() == 0) {
		initParam(reply, NOT_LOGIN, "Logout failed. Didn't log in");
		return;
	}

	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	string query = "SELECT * FROM Account where username='" + username + "'";
	wstring wstr = converter.from_bytes(query);

	if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLWCHAR*)wstr.c_str(), SQL_NTS)) {
		cout << "Error querying SQL Server";
		cout << "\n";
	}

	SQLCHAR sqlStatus[50];

	if (SQLFetch(gSqlStmtHandle) == SQL_SUCCESS) {
		SQLGetData(gSqlStmtHandle, 3, SQL_CHAR, sqlStatus, sizeof(sqlStatus), NULL);

		string strSqlStatus = reinterpret_cast<char*>(sqlStatus);

		SQLCloseCursor(gSqlStmtHandle);

		if (strSqlStatus == "1") {
			query = "UPDATE Account SET status = 0 WHERE username='" + username + "';";
			wstr = converter.from_bytes(query);
			wcout << wstr << endl;
			if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLWCHAR*)wstr.c_str(), SQL_NTS)) {
				cout << "Error querying SQL Server";
				cout << "\n";
			}
			else {
				//Reset session
				session->setUsername("");
				session->setWorkingDir("");

				initParam(reply, LOGOUT_SUCCESS, "Logout successful");
			}
		}
		else {
			initParam(reply, NOT_LOGIN, "Logout failed. Didn't log in");
		}
	}
	else {
		SQLCloseCursor(gSqlStmtHandle);
	}
}

void handleREGISTER(char *username, char *password, char* reply) {
	string userName = username;
	string passWord = password;

	string query;
	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	wstring wstr;

	query = "INSERT INTO Account VALUES ('" + userName + "','" + passWord + "',0)";
	wstr = converter.from_bytes(query);

	if (userName.size() == 0 || passWord.size() == 0) {
		initParam(reply, EMPTY_FIELD, "Empty field");
	}
	else if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLWCHAR*)wstr.c_str(), SQL_NTS)) {
		initParam(reply, USER_ALREADY_EXIST, "Register failed. Username already exists");
	}
	else {
		//Create new dir
		if (CreateDirectoryA(username, NULL))
			initParam(reply, REGISTER_SUCCESS, "Register success");
		else {
			printf("CreateDirectoryA() failed with error %d\n", GetLastError());
			initParam(reply, SERVER_FAIL, "CreateDirectoryA() failed");
		}
	}
}

void handleRETRIVE(LPSESSION session, char *filename, char *reply) {
	LPFILEOBJ fileobj;
	HANDLE hFile;
	LARGE_INTEGER fileSize;

	//Check param
	if (strlen(filename) == 0) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	//Check access and get full path
	if (!checkAccess(session, filename)) {
		initParam(reply, NO_ACCESS, "Dont have access to this file");
		return;
	}

	//Open existing file
	hFile = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		int error = GetLastError();
		printf("CreateFile failed with error %d\n", GetLastError());
		if (error == ERROR_FILE_NOT_FOUND)
			initParam(reply, FILE_ALREADY_EXIST, "File already exist");
		return;
	}

	GetFileSizeEx(hFile, &fileSize);
	fileobj = GetFileObj(hFile, fileSize.QuadPart);

	if (session->fileobj != NULL)
		session->closeFile();

	session->fileobj = fileobj;

	initParam(reply, RETRIEVE_SUCCESS, fileSize.QuadPart);
}

void handleRECEIVE(LPSESSION session, char * reply) {
	LPIO_OBJ sendFObj = getIoObject(IO_OBJ::SEND_F, NULL, 0);
	if (session->fileobj == NULL) {
		initParam(reply, SERVER_FAIL, "Session fileobj null");
		return;
	}

	//Send file
	if (!TransmitFile(session->cmdSock, session->fileobj->file, session->fileobj->size, BUFFSIZE, &(sendFObj->overlapped), NULL, 0)) {
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
			printf("TransmitFile failed with error %d\n", error);
	}

	initParam(reply, FINISH_SEND, "Send file sucessful");
}

void handleSTORE(LPSESSION session, char * filename, char *fileSize, char *reply) {
	char newfile[BUFFSIZE];
	LPFILEOBJ fileobj;
	LONG64 size;

	//Check param
	size = _atoi64(fileSize);
	if (strlen(filename) == 0 || strlen(fileSize) == 0 || size == 0) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	//Check access and get full path
	if (!checkAccess(session, filename)) {
		initParam(reply, NO_ACCESS, "Dont have access to this file");
		return;
	}

	//Create new file
	sprintf_s(newfile, BUFFSIZE, "%s-%d.txt", filename, session->cmdSock);
	HANDLE hFile = CreateFileA(newfile, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_FLAG_OVERLAPPED, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		int error = GetLastError();
		printf("CreateFile failed with error %d\n", GetLastError());
		if (error == ERROR_FILE_EXISTS)
			initParam(reply, FILE_ALREADY_EXIST, "File already exsit");
		else if (error == ERROR_PATH_NOT_FOUND)
			initParam(reply, FILE_NOT_EXIST, "Path not found");
		return;
	}

	//Associates the file hanlde for writing
	if (CreateIoCompletionPort(hFile, gCompletionPort, (ULONG_PTR)&(session->cmdSock), 0) == NULL) {
		FreeFileObj(fileobj);
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		initParam(reply, SERVER_FAIL, "CreateIoCompletionPort() failed");
		return;
	}

	fileobj = GetFileObj(hFile, size);
	session->fileobj = fileobj;

	//Number of recveive file
	DWORD n = size / BUFFSIZE;
	DWORD remain = size % BUFFSIZE;

	for (int i = 0; i < n; ++i) {
		LPIO_OBJ recvFObj = getIoObject(IO_OBJ::RECV_F, NULL, BUFFSIZE);
		recvFObj->sequence = i;

		if (!PostRecv(session->cmdSock, recvFObj)) {
			session->closeFile();
			initParam(reply, TRANSMIT_FAIL, "Receive file fail");
			return;
		}

		//InterlockedIncrement(&(fileobj->pending));
	}

	if (remain != 0) {
		LPIO_OBJ recvFObj = getIoObject(IO_OBJ::RECV_F, NULL, remain);
		recvFObj->sequence = n;

		if (!PostRecv(session->cmdSock, recvFObj)) {
			session->closeFile();
			initParam(reply, TRANSMIT_FAIL, "Receive file fail");
			return;
		}

		//InterlockedIncrement(&(fileobj->pending));
	}

	initParam(reply, STORE_SUCCESS, "Can start sending file");
}

void handleRENAME(LPSESSION session, char *pathname, char *newname, char *reply) {	
	if (strlen(pathname) == 0 || strlen(newname) == 0) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	if (!checkAccess(session, pathname)) {
		initParam(reply, NO_ACCESS, "Dont have access to this file");
		return;
	}

	string fullOldName = pathname;
	string folderName = fullOldName.substr(0, fullOldName.rfind("\\"));

	string newName = newname;
	string fullNewName = folderName + "\\" + newName;

	if (MoveFileA(fullOldName.c_str(), fullNewName.c_str())) {
		initParam(reply, RENAME_SUCCESS, "Rename successful");
	}
	else {
		if (GetLastError() == ERROR_ALREADY_EXISTS)
			initParam(reply, FILE_ALREADY_EXIST, "Rename failed. Name already exists");
		else
			cout << "Rename failed with error " << GetLastError();
	}
}

void handleDELETE(LPSESSION session, char *pathname, char *reply) {
	if (strlen(pathname) == 0) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	if (!checkAccess(session, pathname)) {
		initParam(reply, NO_ACCESS, "Dont have access to this file");
		return;
	}

	if (DeleteFileA(pathname)) {
		initParam(reply, DELETE_SUCCESS, "File deleted successfully");
	}
	else {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			initParam(reply, FILE_NOT_EXIST, "File doesn't exist");
		}
		else {
			cout << "Delete file failed with error " << GetLastError();
		}
	}
}

void handleMAKEDIR(LPSESSION session, char *pathname, char *reply) {
	if (strlen(pathname) == 0) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	if (!checkAccess(session, pathname)) {
		initParam(reply, NO_ACCESS, "Dont have access to this directory");
		return;
	}

	if (CreateDirectoryA(pathname, NULL)) {
		initParam(reply, MAKEDIR_SUCCESS, "Directory created successfully");
	}
	else {
		if (GetLastError() == ERROR_PATH_NOT_FOUND)
			initParam(reply, FILE_NOT_EXIST, "Directory failed. Path not found");
		else if (GetLastError() == ERROR_ALREADY_EXISTS)
			initParam(reply, FILE_ALREADY_EXIST, "Directory failed. Path already exists");
		else
			cout << "Directory failed with error " << GetLastError();
	}
}

void handleREMOVEDIR(LPSESSION session, char *pathname, char *reply) {
	if (strlen(pathname) == 0) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	if (!checkAccess(session, pathname)) {
		initParam(reply, NO_ACCESS, "Dont have access to this directory");
		return;
	}

	if (RemoveDirectoryA(pathname)) {
		initParam(reply, REMOVEDIR_SUCCESS, "Directory created successfully");
	}
	else {
		if (GetLastError() == ERROR_DIR_NOT_EMPTY) {
			initParam(reply, DIR_NOT_EMPTY, "Remove directory failed. Not empty");
		}
		else if (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND) {
			initParam(reply, FILE_NOT_EXIST, "Remove directory failed. Path not found");
		}
		else
			cout << "Remove directory failed with error: " << GetLastError();
	}
}

void handleCHANGEWDIR(LPSESSION session, char *pathname, char *reply) {
	if (strlen(pathname) == 0) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}


	if (!checkAccess(session, pathname)) {
		initParam(reply, NO_ACCESS, "Dont have access to this directory");
		return;
	}

	session->setWorkingDir(pathname);
	initParam(reply, CHANGEWDIR_SUCCESS, "Directory changed successfully");
}

void handlePRINTWDIR(LPSESSION session, char *reply) {
	initParam(reply, PRINTWDIR_SUCCESS, session->workingDir);
}

void handleLISTDIR(LPSESSION session, char *pathname, char *reply) {
	if (strlen(pathname) == 0) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	if (!checkAccess(session, pathname)) {
		initParam(reply, NO_ACCESS, "Dont have access to this directory");
		return;
	}

	string pathName = pathname;
	pathName += "\\\*";
	string names;

	WIN32_FIND_DATAA data;
	HANDLE hFind = FindFirstFileA(pathName.c_str(), &data);     

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			string name = data.cFileName;
			if (name != "." && name != "..") {
				name += "\n";
				names += name;
			}
		} while (FindNextFileA(hFind, &data));
		
		initParam(reply, LIST_SUCCESS, names.c_str());
	}
	else {
		initParam(reply, FILE_NOT_EXIST, "List directory failed. Invalid path");
	}

	FindClose(hFind);
}

void parseMess(const char *mess, char *cmd, char *p1, char *p2) {
	std::string strMess = mess;
	std::string strCmd, strP1, strP2;
	int lenStr = strMess.length(), crPos = strMess.find(HEADER_DELIMITER), spPos = strMess.find(PARA_DELIMITER);
	if (crPos == -1) {
		std::string strCmd = strMess.substr(0, lenStr);
		strcpy_s(cmd, BUFFSIZE, strCmd.c_str());
	}
	else {
		strCmd = strMess.substr(0, crPos);
		strcpy_s(cmd, BUFFSIZE, strCmd.c_str());
		if (spPos == -1) {
			strP1 = strMess.substr(crPos + 1, lenStr - crPos - 1);
			strcpy_s(p1, BUFFSIZE, strP1.c_str());
		}
		else {
			strP1 = strMess.substr(crPos + 1, spPos - crPos - 1);
			strcpy_s(p1, BUFFSIZE, strP1.c_str());
			strP2 = strMess.substr(spPos + 1, lenStr - spPos - 1);
			strcpy_s(p2, BUFFSIZE, strP2.c_str());
		}
	}
}

void handleMess(LPSESSION session, char *mess, char *reply) {
	char cmd[BUFFSIZE], p1[BUFFSIZE], p2[BUFFSIZE];
	char res[BUFFSIZE];

	//Parse message
	parseMess(mess, cmd, p1, p2);

	if (!strcmp(cmd, LOGIN)) {
		handleLOGIN(session, p1, p2, res);
	}
	else if (!strcmp(cmd, LOGOUT)) {
		handleLOGOUT(session, res);
	}
	else if (!strcmp(cmd, REGISTER)) {
		handleREGISTER(p1, p2, res);
	}
	else if (!strcmp(cmd, STORE)) {
		handleSTORE(session, p1, p2, res);
	}
	else if (!strcmp(cmd, RETRIEVE)) {
		handleRETRIVE(session, p1, res);
	}
	else if (!strcmp(cmd, RECEIVE)) {
		handleRECEIVE(session, res);
	}
	else if (!strcmp(cmd, RENAME)) {
		handleRENAME(session, p1, p2, res);
	}
	else if (!strcmp(cmd, DELETEFILE)) {
		handleDELETE(session, p1, res);
	}
	else if (!strcmp(cmd, MAKEDIR)) {
		handleMAKEDIR(session, p1, res);
	}
	else if (!strcmp(cmd, REMOVEDIR)) {
		handleREMOVEDIR(session, p1, res);
	}
	else if (!strcmp(cmd, CHANGEWDIR)) {
		handleCHANGEWDIR(session, p1, res);
	}
	else if (!strcmp(cmd, PRINTWDIR)) {
		handlePRINTWDIR(session, res);
	}
	else if (!strcmp(cmd, LISTDIR)) {
		handleLISTDIR(session, p1, res);
	}
	else
		initParam(res, WRONG_SYNTAX, "Wrong header");

	initMessage(reply, RESPONE, res, NULL);
}

bool connectSQL() {
	SQLHANDLE sqlConnHandle;
	SQLHANDLE sqlEnvHandle;
	SQLWCHAR retconstring[SQL_RETURN_CODE_LEN];

	sqlConnHandle = NULL;
	gSqlStmtHandle = NULL;

	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle))
		cout << "Alloc failed" << endl;
	if (SQL_SUCCESS != SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0))
		cout << "Set env failed" << endl;
	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle))
		cout << "Alloc failed" << endl;

	cout << "Attempting connection to SQL Server...";
	cout << "\n";

	switch (SQLDriverConnect(sqlConnHandle,
		NULL,
		(SQLWCHAR*)L"DRIVER={SQL Server};SERVER=localhost, 1433;DATABASE=FileSystem;UID=sa;PWD=minh1234;",
		SQL_NTS,
		retconstring,
		1024,
		NULL,
		SQL_DRIVER_NOPROMPT))
	{
	case SQL_SUCCESS:
		cout << "Successfully connected to SQL Server";
		cout << "\n";
		break;
	case SQL_SUCCESS_WITH_INFO:
		cout << "Successfully connected to SQL Server";
		cout << "\n";
		break;
	case SQL_INVALID_HANDLE:
		cout << "Could not connect to SQL Server";
		cout << "\n";
	case SQL_ERROR:
		cout << "Could not connect to SQL Server";
		cout << "\n";
	default:
		break;
	}

	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &gSqlStmtHandle)) {
		cout << "Alloc handle failed";
		return FALSE;
	}

	return TRUE;
}

bool checkName(char *name) {
	if (NULL == strchr(name, '\\'))
		return TRUE;
	return FALSE;
}

bool checkAccess(LPSESSION session, char *path) {
	char rootPath[MAX_PATH];
	char fullPath[MAX_PATH];
	char temp[MAX_PATH];

	sprintf_s(temp, MAX_PATH, "%s%s%s", session->workingDir, "\\", path);

	DWORD rootLength = GetFullPathNameA(session->username, MAX_PATH, rootPath, NULL);
	DWORD pathLength = GetFullPathNameA(temp, MAX_PATH, fullPath, NULL);

	if (rootLength != 0 && pathLength != 0 && strstr(fullPath, rootPath) != NULL) {
		strcpy_s(path, MAX_PATH, fullPath);
		return TRUE;
	}

	strcpy_s(path, MAX_PATH, "");
	return FALSE;
}

template <typename T, typename X>
void initMessage(char *mess, const char *header, const T p1, const X p2) {
	char param[BUFFSIZE];

	initParam(param, p1, p2);

	if (strlen(param) == 0)
		sprintf_s(mess, BUFFSIZE, "%s%s", header, ENDING_DELIMITER);
	else
		sprintf_s(mess, BUFFSIZE, "%s%s%s%s", header, HEADER_DELIMITER, param, ENDING_DELIMITER);
}

template <typename T, typename X>
void initParam(char *param, const T p1, const X p2) {
	strcpy_s(param, BUFFSIZE, "");
	std::ostringstream sstr;

	if (p1 == NULL)
		return;

	if (p2 == NULL)
		sstr << p1;
	else
		sstr << p1 << PARA_DELIMITER << p2;

	string str = sstr.str();
	const char *cstr = str.c_str();
	strcpy_s(param, BUFFSIZE, cstr);
}
