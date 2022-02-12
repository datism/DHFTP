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

void handleLOGIN(LPSESSION session, const char *username,const char *password, char *reply) {
	string userName = username;
	string passWord = password;

	if (strlen(session->username) != 0) {
		initParam(reply, ALREADY_LOGIN, "Already log in");
		return;
	}

	if (userName.size() == 0 || passWord.size() == 0) {
		initParam(reply, EMPTY_FIELD, "Empty field");
		return;
	}

	SQLCHAR sqlUsername[50];
	SQLCHAR sqlPassword[50];
	SQLCHAR sqlStatus[50];

	EnterCriticalSection(&gCriticalSection);
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
				initParam(reply, ALREADY_LOGIN, "Login failed. ACcount already logged in another session");
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

	LeaveCriticalSection(&gCriticalSection);
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

	EnterCriticalSection(&gCriticalSection);
	string query = "SELECT * FROM Account where username='" + userName + "'";

	if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
		cout << "Error querying SQL Server" << endl;
		return;
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

	LeaveCriticalSection(&gCriticalSection);
}

void handleLOGOUT(LPSESSION session, char *reply) {
	string username = session->username;

	if (username.length() == 0) {
		initParam(reply, NOT_LOGIN, "Logout failed. Didn't log in");
		return;
	}

	EnterCriticalSection(&gCriticalSection);
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

	LeaveCriticalSection(&gCriticalSection);
}

void handleREGISTER(const char *username, const char *password, char* reply) {
	string userName = username;
	string passWord = password;

	if (userName.size() == 0 || passWord.size() == 0) {
		initParam(reply, EMPTY_FIELD, "Empty field");
		return;
	}

	if (!checkName(username)) {
		initParam(reply, WRONG_SYNTAX, "Invalid username");
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

void handleRETRIVE(LPSESSION session, const char *filename, char *reply) {
	LPFILEOBJ fileobj;
	HANDLE hFile;
	LARGE_INTEGER fileSize;
	char fullPath[MAX_PATH];

	//havent login
	if (strlen(session->username) == 0) {
		/*initParam(reply, NOT_LOGIN, "Didn't log in");
		return;*/
		session->setUsername("test");
		session->setWorkingDir("test");
	}

	//Check param
	if (strlen(filename) == 0) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	//check file name
	if (!checkName(filename)) {
		initParam(reply, WRONG_SYNTAX, "Invalid file name");
		return;
	}

	//Check access and get full path
	if (!checkAccess(session, filename, fullPath)) {
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

	//Open existing file
	hFile = CreateFileA(fullPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		int error = GetLastError();
		printf("CreateFile failed with error %d\n", GetLastError());
		if (error == ERROR_FILE_NOT_FOUND)
			initParam(reply, FILE_NOT_EXIST, "File doesnt exist");
		else
			initParam(reply, SERVER_FAIL, "CreateFile failed");
		return;
	}

	GetFileSizeEx(hFile, &fileSize);

	fileobj = GetFileObj(hFile, fileSize.QuadPart, FILEOBJ::RETR);

	if (fileobj == NULL) {
		initParam(reply, SERVER_FAIL, "Out of memory");
		return;
	}

	session->fileobj = fileobj;

	EnterCriticalSection(&gCriticalSection);
	gFileListen->count++;
	WSASetEvent(gFileListen->acceptEvent);
	LeaveCriticalSection(&gCriticalSection);

	initParam(reply, RETRIEVE_SUCCESS, (ULONG)session, fileobj->size);
}

void handleSTORE(LPSESSION session, const char * filename, const char *fileSize, char *reply) {
	LPFILEOBJ fileobj;
	LONG64 size;
	char fullPath[MAX_PATH];

	//havent login
	if (strlen(session->username) == 0) {
		/*initParam(reply, NOT_LOGIN, "Didn't log in");
		return;*/
		session->setUsername("test");
		session->setWorkingDir("test");
	}

	//check file name
	if (!checkName(filename)) {
		initParam(reply, WRONG_SYNTAX, "Invalid file name");
		return;
	}

	//Check param
	size = _atoi64(fileSize);
	if (strlen(filename) == 0 || strlen(fileSize) == 0 || size == 0) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	//Check access and get full path
	if (!checkAccess(session, filename, fullPath)) {
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
	HANDLE hFile = CreateFileA(fullPath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_NEW, FILE_FLAG_OVERLAPPED, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		int error = GetLastError();
		printf("CreateFile failed with error %d\n", GetLastError());
		if (error == ERROR_FILE_EXISTS)
			initParam(reply, FILE_ALREADY_EXIST, "File already exsit");
		else
			initParam(reply, SERVER_FAIL, "CreateFile failed");
		return;
	}

	//Associates the file hanlde for writing
	if (CreateIoCompletionPort(hFile, gCompletionPort, (ULONG_PTR)session, 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		initParam(reply, SERVER_FAIL, "CreateIoCompletionPort() failed");
		return;
	}

	fileobj = GetFileObj(hFile, size, FILEOBJ::STOR);

	if (fileobj == NULL) {
		initParam(reply, SERVER_FAIL, "Out of memory");
		return;
	}

	session->fileobj = fileobj;

	EnterCriticalSection(&gCriticalSection);
	gFileListen->count++;
	WSASetEvent(gFileListen->acceptEvent);
	LeaveCriticalSection(&gCriticalSection);


	initParam(reply, STORE_SUCCESS, (ULONG)session);
}

void handleRENAME(LPSESSION session, const char *pathname, const char *newname, char *reply) {
	char fullPath[MAX_PATH];
	int error;

	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Didn't log in");
		return;
	}

	if (strlen(pathname) == 0 || strlen(pathname) > MAX_PATH) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	if (!checkAccess(session, pathname, fullPath)) {
		initParam(reply, NO_ACCESS, "Dont have access to this directory");
		return;
	}

	string fullOldName = fullPath;
	string folderName = fullOldName.substr(0, fullOldName.rfind("\\"));

	string newName = newname;
	string fullNewName = folderName + "\\" + newName;

	EnterCriticalSection(&session->cs);
	if (MoveFileA(fullOldName.c_str(), fullNewName.c_str())) {
		initParam(reply, RENAME_SUCCESS, "Rename successful");
	}
	else {
		error = GetLastError();

		if (error == ERROR_ALREADY_EXISTS)
			initParam(reply, FILE_ALREADY_EXIST, "Rename failed. Name already exists");
		else if (error == ERROR_FILE_NOT_FOUND)
			initParam(reply, FILE_ALREADY_EXIST, "Rename failed. File not found");
		else if (error == ERROR_SHARING_VIOLATION)
			initParam(reply, FiLE_BUSY, "File is being used by another process");
		else
			initParam(reply, SERVER_FAIL, "MoveFile faile");
	}
	LeaveCriticalSection(&session->cs);
}

void handleDELETE(LPSESSION session, const char *pathname, char *reply) {
	char fullPath[MAX_PATH];
	int error;

	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Didn't log in");
		return;
	}

	if (strlen(pathname) == 0 || strlen(pathname) > MAX_PATH) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	if (!checkAccess(session, pathname, fullPath)) {
		initParam(reply, NO_ACCESS, "Dont have access to this directory");
		return;
	}
	
	EnterCriticalSection(&session->cs);
	if (DeleteFileA(fullPath)) {
		initParam(reply, DELETE_SUCCESS, "File deleted successfully");
	}
	else {
		error = GetLastError();

		if (error == ERROR_FILE_NOT_FOUND)
			initParam(reply, FILE_NOT_EXIST, "File doesn't exist");
		else if (error == ERROR_SHARING_VIOLATION)
			initParam(reply, FiLE_BUSY, "File is being used by another process");
		else 
			initParam(reply, SERVER_FAIL, "DeleteFile failed");
	}
	LeaveCriticalSection(&session->cs);
}

void handleMAKEDIR(LPSESSION session, const char *pathname, char *reply) {
	char fullPath[MAX_PATH];
	int error;

	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Didn't log in");
		return;
	}

	if (strlen(pathname) == 0 || strlen(pathname) > MAX_PATH) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	if (!checkAccess(session, pathname, fullPath)) {
		initParam(reply, NO_ACCESS, "Dont have access to this directory");
		return;
	}

	if (CreateDirectoryA(fullPath, NULL)) {
		initParam(reply, MAKEDIR_SUCCESS, "Directory created successfully");
	}
	else {
		error = GetLastError();

		if (error == ERROR_PATH_NOT_FOUND)
			initParam(reply, FILE_NOT_EXIST, "Directory failed. Path not found");
		else if (error == ERROR_ALREADY_EXISTS)
			initParam(reply, FILE_ALREADY_EXIST, "Directory failed. Path already exists");
		else
			initParam(reply, SERVER_FAIL, "CreateDirectory failed");
	}
}

void handleREMOVEDIR(LPSESSION session, const char *pathname, char *reply) {
	char fullPath[MAX_PATH];
	int error;

	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Didn't log in");
		return;
	}

	if (strlen(pathname) == 0 || strlen(pathname) > MAX_PATH) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	if (!checkAccess(session, pathname, fullPath)) {
		initParam(reply, NO_ACCESS, "Dont have access to this directory");
		return;
	}

	if (RemoveDirectoryA(fullPath)) {
		initParam(reply, REMOVEDIR_SUCCESS, "Directory removed successfully");
	}
	else {
		error = GetLastError();

		if (error == ERROR_DIR_NOT_EMPTY) {
			initParam(reply, DIR_NOT_EMPTY, "Remove directory failed. Not empty");
		}
		else if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
			initParam(reply, FILE_NOT_EXIST, "Remove directory failed. Path not found");
		}
		else
			initParam(reply, SERVER_FAIL, "RemoveDirectory failed");
	}
}

void handleCHANGEWDIR(LPSESSION session, const char *pathname, char *reply) {
	char fullPath[MAX_PATH];
	char fileData[BUFFSIZE];
	HANDLE find;

	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Didn't log in");
		return;
	}

	if (strlen(pathname) == 0 || strlen(pathname) > MAX_PATH) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}


	if (!checkAccess(session, pathname, fullPath)) {
		initParam(reply, NO_ACCESS, "Dont have access to this directory");
		return;
	}

	find = FindFirstFileExA(
		fullPath,
		FindExInfoBasic,
		fileData,
		FindExSearchLimitToDirectories,
		NULL,
		FIND_FIRST_EX_CASE_SENSITIVE);

	if (find == INVALID_HANDLE_VALUE)
		initParam(reply, FILE_NOT_EXIST, "Change working directory failed. Path not found");
	else {
		session->setWorkingDir(fullPath);
		initParam(reply, CHANGEWDIR_SUCCESS, "Directory changed successfully");
	}

	FindClose(find);
}

void handlePRINTWDIR(LPSESSION session, char *reply) {
	char *workingDir;

	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Didn't log in");
		return;
	}

	workingDir = strstr(session->workingDir, session->username);

	initParam(reply, PRINTWDIR_SUCCESS, workingDir);
}

void handleLISTDIR(LPSESSION session, const char *pathname, char *reply) {
	char fullPath[MAX_PATH];

	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Didn't log in");
		return;
	}

	if (strlen(pathname) == 0 || strlen(pathname) > MAX_PATH) {
		initParam(reply, WRONG_SYNTAX, "Wrong parameter");
		return;
	}

	if (!checkAccess(session, pathname, fullPath)) {
		initParam(reply, NO_ACCESS, "Dont have access to this directory");
		return;
	}

	string pathName = fullPath;
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

void handleMess(LPSESSION session, char *mess, char *reply) {
	std::vector<std::string> para;
	char cmd[BUFFSIZE] = "", res[BUFFSIZE] = "";
	const char *p1, *p2;

	//Parse message
	newParseMess(mess, cmd, para);

	if (!strcmp(cmd, LOGIN)) {
		if (para.size() != 2) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			p2 = para[1].c_str();
			handleLOGIN(session, p1, p2, res);
		}
	}
	else if (!strcmp(cmd, LOGOUT)) {
		if (para.size() != 0) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			handleLOGOUT(session, res);
		}
	}
	else if (!strcmp(cmd, REGISTER)) {
		if (para.size() != 2) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			p2 = para[1].c_str();
			handleREGISTER(p1, p2, res);
		}
	}
	else if (!strcmp(cmd, STORE)) {
		if (para.size() != 2) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			p2 = para[1].c_str();
			handleSTORE(session, p1, p2, res);
		}
	}
	else if (!strcmp(cmd, RETRIEVE)) {
		if (para.size() != 1) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			handleRETRIVE(session, p1, res);
		}
	}
	else if (!strcmp(cmd, RENAME)) {
		if (para.size() != 2) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			p2 = para[1].c_str();
			handleRENAME(session, p1, p2, res);
		}
	}
	else if (!strcmp(cmd, DELETEFILE)) {
		if (para.size() != 1) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			handleDELETE(session, p1, res);
		}
	}
	else if (!strcmp(cmd, MAKEDIR)) {
		if (para.size() != 1) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			handleMAKEDIR(session, p1, res);
		}
	}
	else if (!strcmp(cmd, REMOVEDIR)) {
		if (para.size() != 1) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			handleREMOVEDIR(session, p1, res);
		}
	}
	else if (!strcmp(cmd, CHANGEWDIR)) {
		if (para.size() != 1) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			handleCHANGEWDIR(session, p1, res);
		}
	}
	else if (!strcmp(cmd, PRINTWDIR)) {
		if (para.size() != 0) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			handlePRINTWDIR(session, res);
		}
	}
	else if (!strcmp(cmd, LISTDIR)) {
		if (para.size() != 1) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			handleLISTDIR(session, p1, res);
		}
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

void newParseMess(char *mess, char *cmd, std::vector<std::string> &para) {
	string strMess = mess;
	string strCmd, strP;
	int crPos, spPos, lenStr;

	lenStr = strMess.length();
		
	crPos = strMess.find(HEADER_DELIMITER);

	if (crPos == -1) {
		strcpy_s(cmd, BUFFSIZE, strMess.c_str());
	}
	else {
		strCmd = strMess.substr(0, crPos);
		strcpy_s(cmd, BUFFSIZE, strCmd.c_str());

		strP = strMess.substr(crPos + strlen(HEADER_DELIMITER), lenStr - crPos - strlen(HEADER_DELIMITER));
		spPos = strP.find(PARA_DELIMITER);

		while (1) {
			spPos = strP.find(PARA_DELIMITER);

			if (spPos == -1) {
				para.push_back(strP);
				break;
			}
			else {
				para.push_back(strP.substr(0, spPos));
				strP = strP.substr(spPos + strlen(PARA_DELIMITER), strP.length() - spPos - strlen(PARA_DELIMITER));
			}
		}
	}
}

bool checkAccess(LPSESSION session, const char *path, char *fullPath) {
	char rootPath[MAX_PATH];
	char temp[MAX_PATH];

	sprintf_s(temp, MAX_PATH, "%s%s%s", session->workingDir, "\\", path);

	DWORD rootLength = GetFullPathNameA(session->username, MAX_PATH, rootPath, NULL);
	DWORD pathLength = GetFullPathNameA(temp, MAX_PATH, fullPath, NULL);

	if (rootLength != 0 && pathLength != 0 && strstr(fullPath, rootPath) != NULL) {
		return TRUE;
	}

	strcpy_s(fullPath, MAX_PATH, "");
	return FALSE;
}

bool checkName(const char *name) {
	if (strlen(name) != 0 || NULL == strchr(name, '\\'))
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