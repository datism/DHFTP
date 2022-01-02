#pragma once
#include "stdafx.h"
#include <iostream>
#include <windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <string>
#include <codecvt>
using namespace std;

#define SQL_RESULT_LEN 240
#define SQL_RETURN_CODE_LEN 1000

SQLHANDLE connectSQL() {
	SQLHANDLE sqlConnHandle;
	SQLHANDLE sqlStmtHandle;
	SQLHANDLE sqlEnvHandle;
	SQLWCHAR retconstring[SQL_RETURN_CODE_LEN];

	sqlConnHandle = NULL;
	sqlStmtHandle = NULL;

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

	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle))
		cout << "Alloc handle failed";

	return sqlStmtHandle;
}

void handleREGISTER(SQLHANDLE sqlStmtHandle, string username, string password) {
	string query;
	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	wstring wstr;

	query = "INSERT INTO Account VALUES ('" + username + "','" + password + "',0)";
	wstr = converter.from_bytes(query);

	if (SQL_SUCCESS != SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)wstr.c_str(), SQL_NTS)) {
		//313
		cout << "Username already exists" << endl;
	}
	else {
		//112
		cout << "Sign up successful" << endl;;
	}
}

void handleLOGIN(SQLHANDLE sqlStmtHandle, string username, string password) {
	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	string query = "SELECT * FROM Account where username='" + username + "'";
	wstring wstr = converter.from_bytes(query);

	if (SQL_SUCCESS != SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)wstr.c_str(), SQL_NTS)) {
		cout << "Error querying SQL Server" << endl;
		wcout << wstr;
		return;
	}

	SQLCHAR sqlUsername[50];
	SQLCHAR sqlPassword[50];
	SQLCHAR sqlStatus[50];

	if (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
		SQLGetData(sqlStmtHandle, 1, SQL_CHAR, sqlUsername, sizeof(sqlUsername), NULL);
		SQLGetData(sqlStmtHandle, 2, SQL_CHAR, sqlPassword, sizeof(sqlPassword), NULL);
		SQLGetData(sqlStmtHandle, 3, SQL_CHAR, sqlStatus, sizeof(sqlStatus), NULL);

		string strSqlPassword = reinterpret_cast<char*>(sqlPassword);
		string strSqlStatus = reinterpret_cast<char*>(sqlStatus);

		SQLCloseCursor(sqlStmtHandle);

		if (password == strSqlPassword) {
			if (strSqlStatus == "0") {
				query = "UPDATE Account SET status = 1 WHERE username='" + username + "';";
				wstr = converter.from_bytes(query);
				wcout << wstr << endl;
				if (SQL_SUCCESS != SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)wstr.c_str(), SQL_NTS)) {
					cout << "Error querying SQL Server";
					cout << "\n";
				}
				else {
					//110
					cout << "Log in successful!" << endl;
				}
			}
			else {
				//311
				cout << "Already logged in" << endl;
			}
		}
		else {
			//314
			cout << "Wrong password" << endl;
		}
	}
	else {
		//312
		cout << "Username doesn't exist" << endl;
		SQLCloseCursor(sqlStmtHandle);
	}
}

void handleLOGOUT(SQLHANDLE sqlStmtHandle, string username) {
	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	string query = "SELECT * FROM Account where username='" + username + "'";
	wstring wstr = converter.from_bytes(query);

	if (SQL_SUCCESS != SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)wstr.c_str(), SQL_NTS)) {
		cout << "Error querying SQL Server";
		cout << "\n";
	}

	SQLCHAR sqlStatus[50];

	if (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
		SQLGetData(sqlStmtHandle, 3, SQL_CHAR, sqlStatus, sizeof(sqlStatus), NULL);

		string strSqlStatus = reinterpret_cast<char*>(sqlStatus);

		SQLCloseCursor(sqlStmtHandle);

		if (strSqlStatus == "1") {
			query = "UPDATE Account SET status = 0 WHERE username='" + username + "';";
			wstr = converter.from_bytes(query);
			wcout << wstr << endl;
			if (SQL_SUCCESS != SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)wstr.c_str(), SQL_NTS)) {
				cout << "Error querying SQL Server";
				cout << "\n";
			}
			else {
				//111
				cout << "Log out successful!" << endl;
			}
		}
		else {
			//310
			cout << "Log out failed. Didn't log in" << endl;
		}
	}
	else {
		SQLCloseCursor(sqlStmtHandle);
	}
}
