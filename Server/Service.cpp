#include "Service.h"
#include <WinSock2.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <codecvt>
#include "EnvVar.h"
#include "IoObj.h"
#include "FileObj.h"

using namespace std;

void handleLOGIN(LPSESSION session, char *username, char *password, char *reply) {
	string userName = username;
	string passWord = password;

	if (userName.size() == 0 || passWord.size() == 0) {
		initParam(reply, EMPTY_FIELD, "Empty field");
		return;
	}

	SQLCHAR sqlUsername[50];
	SQLCHAR sqlPassword[50];
	SQLCHAR sqlStatus[50];

	string query = "SELECT * FROM Account where username='" + userName + "'";

	if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
		cout << "Error querying SQL Server" << endl;
		cout << query;
		return;
	}

	if (SQLFetch(gSqlStmtHandle) == SQL_SUCCESS) {
		SQLGetData(gSqlStmtHandle, 1, SQL_CHAR, sqlUsername, sizeof(sqlUsername), NULL);
		SQLGetData(gSqlStmtHandle, 2, SQL_CHAR, sqlPassword, sizeof(sqlPassword), NULL);
		SQLGetData(gSqlStmtHandle, 3, SQL_CHAR, sqlStatus, sizeof(sqlStatus), NULL);

		string strSqlPassword = reinterpret_cast<char*>(sqlPassword);
		string strSqlStatus = reinterpret_cast<char*>(sqlStatus);

		SQLCloseCursor(gSqlStmtHandle);

		if (passWord == strSqlPassword) {
			if (strSqlStatus == "0") {
				query = "UPDATE Account SET status = 1 WHERE username='" + userName + "';";
				if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
					cout << "Error querying SQL Server";
					cout << "\n";
				}
				else {
					session->setUsername(username);
					session->setWorkingDir(username);
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

void changePass(LPSESSION session, char *reply) {
	char oldpass[BUFFSIZE], newpass[BUFFSIZE];
	string userName = session->username;

	if (userName.length() == 0) {
		initParam(reply, NOT_LOGIN, "Logout failed. Didn't log in");
		return;
	}

	cout << "Enter old password: ";
	gets_s(oldpass, BUFFSIZE);
	cout << "Enter new password: ";
	gets_s(oldpass, BUFFSIZE);

	string oldPass = oldpass, newPass = newpass;

	if (oldPass.length() == 0 || newPass.length() == 0) {
		initParam(reply, EMPTY_FIELD, "Empty field");
		return;
	}

	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	string query = "SELECT * FROM Account where username='" + userName + "'";

	if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
		cout << "Error querying SQL Server" << endl;
		return ;
	}

	SQLWCHAR sqlPassword[50];

	if (SQLFetch(gSqlStmtHandle) == SQL_SUCCESS) {
		SQLGetData(gSqlStmtHandle, 2, SQL_CHAR, sqlPassword, sizeof(sqlPassword), NULL);
		string strSqlPassword = reinterpret_cast<char*>(sqlPassword);
		SQLCloseCursor(gSqlStmtHandle);

		if (oldPass == strSqlPassword) {
			query = "UPDATE Account SET password = " + newPass + " WHERE username='" + userName + "';";
			if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
				cout << "Error querying SQL Server";
				cout << "\n";
			}
			else {
				//initParam(reply, CHANGE_PASS_SUCCESS, "Password changed successfully");
			}
		}
		else {
			//initParam(reply, CHANGE_PASS_SUCCESS, "Wrong old password");
		}
	}
	else {
		SQLCloseCursor(gSqlStmtHandle);
	}
}

void handleLOGOUT(LPSESSION session, char *reply) {
	string username = session->username;

	if (username.length() == 0) {
		initParam(reply, NOT_LOGIN, "Logout failed. Didn't log in");
		return;
	}


	string query = "SELECT * FROM Account where username='" + username + "'";

	if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
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
			if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
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

	if (userName.size() == 0 || passWord.size() == 0) {
		initParam(reply, EMPTY_FIELD, "Empty field");
		return;
	}

	string query;
	query = "INSERT INTO Account VALUES ('" + userName + "','" + passWord + "',0)";

	if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
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

void handleRETRIVE(LPSESSION session, char *clientPort, char *filename, char *reply) {
	LPIO_OBJ connectobj;
	LPFILEOBJ fileobj;
	SOCKADDR_IN addr;
	HANDLE hFile;
	LARGE_INTEGER fileSize;
	DWORD port;
	int namelen = sizeof(addr);


	//havent login
	if (strlen(session->username) == 0) {
		/*initParam(reply, NOT_LOGIN, "Didn't log in");
		return;*/
		session->setUsername("test");
		session->setWorkingDir("test");
	}

	//Check param
	port = atoi(clientPort);
	if (port <= 0) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	if (!checkName(filename)) {
		initParam(reply, WRONG_SYNTAX, "Invalid file name");
		return;
	}

	//previous file wasnt closed
	EnterCriticalSection(&session->cs);
	if (session->fileobj != NULL) {
		initParam(reply, SERVER_FAIL, "1 file at a time");
		LeaveCriticalSection(&session->cs);
		return;
	}
	LeaveCriticalSection(&session->cs);

	//Open existing file
	hFile = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		int error = GetLastError();
		printf("CreateFile failed with error %d\n", GetLastError());
		if (error == ERROR_FILE_NOT_FOUND)
			initParam(reply, FILE_NOT_EXIST, "File doesnt exist");
		return;
	}

	GetFileSizeEx(hFile, &fileSize);

	if (getpeername(session->cmdSock, (SOCKADDR *)&addr, &namelen) == SOCKET_ERROR) {
		printf("getpeername failed with error %d\n", WSAGetLastError());
		initParam(reply, SERVER_FAIL, "getpeername failed");
		return;
	}

	addr.sin_port = htons(port);

	fileobj = GetFileObj(hFile, &addr, fileSize.QuadPart, FILEOBJ::RETR);
	connectobj = getIoObject(IO_OBJ::CONECT, NULL, 0);

	if (fileobj == NULL || connectobj == NULL) {
		initParam(reply, SERVER_FAIL, "Out of memory");
		return;
	}

	if (CreateIoCompletionPort((HANDLE)session->fileobj->fileSock, gCompletionPort, (ULONG_PTR)session, 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		initParam(reply, SERVER_FAIL, "CreateIoCompletionPort() failed");
		return;
	}

	session->fileobj = fileobj;
	session->EnListPendingOperation(connectobj);

	initParam(reply, RETRIEVE_SUCCESS, fileobj->size);
}

void handleSTORE(LPSESSION session, char *clientPort, char * filename, char *fileSize, char *reply) {
	LPFILEOBJ fileobj;
	SOCKADDR_IN addr;
	LPIO_OBJ connectobj;
	LONG64 size;
	DWORD port;
	int namelen = sizeof(addr);

	//havent login
	if (strlen(session->username) == 0) {
		/*initParam(reply, NOT_LOGIN, "Didn't log in");
		return;*/
		session->setUsername("test");
		session->setWorkingDir("test");
	}

	//Check param
	size = _atoi64(fileSize);
	port = atoi(clientPort);
	if (port <= 0 || size <= 0) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	if (!checkName(filename)) {
		initParam(reply, WRONG_SYNTAX, "Invalid file name");
		return;
	}


	//Check access and get full path
	if (!checkAccess(session, filename)) {
		initParam(reply, NO_ACCESS, "Dont have access to this file");
		return;
	}

	//previous file wasnt closed
	EnterCriticalSection(&session->cs);
	if (session->fileobj != NULL) {
		initParam(reply, SERVER_FAIL, "1 file at a time");
		LeaveCriticalSection(&session->cs);
		return;
	}
	LeaveCriticalSection(&session->cs);

	//creat new file
	HANDLE hFile = CreateFileA(filename, GENERIC_WRITE | DELETE, 0, NULL, CREATE_NEW, FILE_FLAG_OVERLAPPED, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		int error = GetLastError();
		printf("CreateFile failed with error %d\n", GetLastError());
		if (error == ERROR_FILE_EXISTS)
			initParam(reply, FILE_ALREADY_EXIST, "File already exsit");
		return;
	}
	
	if (getpeername(session->cmdSock, (SOCKADDR *)&addr, &namelen) == SOCKET_ERROR) {
		printf("getpeername failed with error %d\n", WSAGetLastError());
		initParam(reply, SERVER_FAIL, "getpeername failed");
		return;
	}

	addr.sin_port = htons(port);

	fileobj = GetFileObj(hFile, &addr, size, FILEOBJ::STOR);
	connectobj = getIoObject(IO_OBJ::CONECT, NULL, 0);

	if (fileobj == NULL || connectobj == NULL) {
		initParam(reply, SERVER_FAIL, "Out of memory");
		return;
	}
	session->fileobj = fileobj;

	//Associates the file hanlde for writing
	if (CreateIoCompletionPort(hFile, gCompletionPort, (ULONG_PTR)session, 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		initParam(reply, SERVER_FAIL, "CreateIoCompletionPort() failed");
		return;
	}if (CreateIoCompletionPort((HANDLE) session->fileobj->fileSock, gCompletionPort, (ULONG_PTR)session, 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		initParam(reply, SERVER_FAIL, "CreateIoCompletionPort() failed");
		return;
	}

	

	session->EnListPendingOperation(connectobj);

	initParam(reply, STORE_SUCCESS, "CONNECT");
}

void handleRENAME(LPSESSION session, char *pathname, char *newname, char *reply) {
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Didn't log in");
		return;
	}

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
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Didn't log in");
		return;
	}

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
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Didn't log in");
		return;
	}

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
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Didn't log in");
		return;
	}

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
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Didn't log in");
		return;
	}

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
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Didn't log in");
		return;
	}

	initParam(reply, PRINTWDIR_SUCCESS, session->workingDir);
}

void handleLISTDIR(LPSESSION session, char *pathname, char *reply) {
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Didn't log in");
		return;
	}

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

void newParseMess(char *mess, char *cmd, std::vector<std::string> &para) {
	string strMess = mess;
	string strCmd;
	int lenStr = strMess.length(), crPos = strMess.find(HEADER_DELIMITER);

	if (crPos == -1) {
		string strCmd = strMess.substr(0, lenStr);
		strcpy_s(cmd, BUFFSIZE, strCmd.c_str());
	}
	else {
		strCmd = strMess.substr(0, crPos);
		strcpy_s(cmd, BUFFSIZE, strCmd.c_str());

		string strP = strMess.substr(crPos + 1, lenStr - crPos - 1);
		int spPos = strP.find(PARA_DELIMITER);

		while (spPos != -1) {
			string p = strP.substr(0, spPos);
			para.push_back(p);
			int len = strP.length();
			strP = strP.substr(spPos + 1, len - spPos - 1);
			spPos = strP.find(PARA_DELIMITER);
		}
	}
}

void parseMess(char *mess, char *cmd, char *p1, char *p2) {
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
	char cmd[BUFFSIZE] = "", res[BUFFSIZE] = "";
	std::vector<std::string> para;
  
  //Parse message
	newParseMess(mess, cmd, para);

  if (!strcmp(cmd, LOGIN)) {
		handleLOGIN(session, (char *)para[0].c_str(), (char *)para[1].c_str(), res);
	}
	else if (!strcmp(cmd, LOGOUT)) {
		handleLOGOUT(session, res);
  }
	else if (!strcmp(cmd, REGISTER)) {
		handleREGISTER((char *)para[0].c_str(), (char *)para[1].c_str(), res);
	}
	else if (!strcmp(cmd, STORE)) {
		handleSTORE(session, (char *)para[0].c_str(), (char *)para[1].c_str(), (char *)para[2].c_str(), res);
	}
	else if (!strcmp(cmd, RETRIEVE)) {
		handleRETRIVE(session, (char *)para[0].c_str(), (char *)para[1].c_str(), res);
	}
	else if (!strcmp(cmd, RENAME)) {
		handleRENAME(session, (char *)para[0].c_str(), (char *)para[1].c_str(), res);
	}
	else if (!strcmp(cmd, DELETEFILE)) {
		handleDELETE(session, (char *)para[0].c_str(), res);
	}
	else if (!strcmp(cmd, MAKEDIR)) {
		handleMAKEDIR(session, (char *)para[0].c_str(), res);
	}
	else if (!strcmp(cmd, REMOVEDIR)) {
		handleREMOVEDIR(session, (char *)para[0].c_str(), res);
	}
	else if (!strcmp(cmd, CHANGEWDIR)) {
		handleCHANGEWDIR(session, (char *)para[0].c_str(), res);
	}
	else if (!strcmp(cmd, PRINTWDIR)) {
		handlePRINTWDIR(session, res);
	}
	else if (!strcmp(cmd, LISTDIR)) {
		handleLISTDIR(session, (char *)para[0].c_str(), res);
	}
	else
		initParam(res, WRONG_SYNTAX, "Wrong header");

	initMessage(reply, RESPONE, res);
}

bool connectSQL() {
	SQLHANDLE sqlConnHandle;
	SQLHANDLE sqlEnvHandle;
	SQLCHAR retconstring[SQL_RETURN_CODE_LEN];

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
		(SQLCHAR*)"DRIVER={SQL Server};SERVER=localhost, 1433;DATABASE=FileSystem;UID=sa;PWD=minh1234;",
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

bool checkName(char *name) {
	if (strlen(name) == 0 || NULL == strchr(name, '\\'))
		return TRUE;
	return FALSE;
}

void initParam(char *param) {}

template <typename P>
void initParam(char *param, P p) {
	std::ostringstream sstr;
	sstr << p;
	strcat_s(param, BUFFSIZE, sstr.str().c_str());
}

template <typename P, typename... Args>
void initParam(char *param, P p, Args... paras) {
	std::ostringstream sstr;

	sstr << p << PARA_DELIMITER;
	strcat_s(param, BUFFSIZE, sstr.str().c_str());

	initParam(param, paras...);
}