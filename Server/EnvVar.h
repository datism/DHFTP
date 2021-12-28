// Server.cpp : Defines the entry point for the console application.
//

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

int main() {
	string username, password;
	int mode;

	string query;
	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	wstring wstr;

	SQLHANDLE sqlConnHandle;
	SQLHANDLE sqlStmtHandle;
	SQLHANDLE sqlEnvHandle;
	SQLWCHAR retconstring[SQL_RETURN_CODE_LEN];

	sqlConnHandle = NULL;
	sqlStmtHandle = NULL;

	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle))
		goto COMPLETED;
	if (SQL_SUCCESS != SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0))
		goto COMPLETED;
	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle))
		goto COMPLETED;
	//output
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
		goto COMPLETED;
	case SQL_ERROR:
		cout << "Could not connect to SQL Server";
		cout << "\n";
		goto COMPLETED;
	default:
		break;
	}

	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle))
		goto COMPLETED;

	cout << "Choose mode: 1: Log In   2: Sign Up    3: Log out" << endl;
	cin >> mode;

	getline(cin, username);
	cout << "Enter username: ";
	getline(cin, username);
	cout << "Enter password: ";
	getline(cin, password);

	if (username.length() == 0 || password.length() == 0) {
		//315
		cout << "Empty field" << endl;
	}
	else if (mode == 1) {
		cout << "\n";
		cout << "Logging in...";
		cout << "\n";

		query = "SELECT * FROM Account where username='" + username + "'";
		wstr = converter.from_bytes(query);

		if (SQL_SUCCESS != SQLExecDirect(sqlStmtHandle, (SQLWCHAR*)wstr.c_str(), SQL_NTS)) {
			cout << "Error querying SQL Server";
			cout << "\n";
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
		}
	}
	else if (mode == 2) {
		cout << "\n";
		cout << "Signing up...";
		cout << "\n";

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
	else if (mode == 3) {
		cout << "\n";
		cout << "Logging out...";
		cout << "\n";

		query = "SELECT * FROM Account where username='" + username + "'";
		wstr = converter.from_bytes(query);

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
	}

COMPLETED:
	SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
	SQLDisconnect(sqlConnHandle);
	SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
	SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);

	cout << "\nPress any key to exit...";
	getchar();
}
