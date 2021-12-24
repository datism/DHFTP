#include <Windows.h>
#include <fileapi.h>
#include <stdio.h>

int main() {
	HANDLE file = CreateFile("test.txt", GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	char buffer[] = "helloWorld";
	DWORD bytesWritten;
	WriteFile(file, buffer, strlen(buffer), &bytesWritten, NULL);

	HANDLE file1;
	if ((file1 = CreateFile("test.txt", GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
		printf("fuck %d\n", GetLastError());
}