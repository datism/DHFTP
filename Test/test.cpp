#include <iostream>
#include <Windows.h>
#include <fileapi.h>


using namespace std;

void closeFile(HANDLE file, BOOLEAN deleteFile) {
	FILE_DISPOSITION_INFO fdi;
	fdi.DeleteFile = deleteFile;
	if (!SetFileInformationByHandle(file,
		FileDispositionInfo, &fdi, sizeof(FILE_DISPOSITION_INFO))) {
		printf("SetFileInformationByHandle failed with error %d\n", GetLastError());
	}

	CloseHandle(file);
}

int main() {
	char *filename = "testbig.rar";
	char *newname = "testtest.txt";
	HANDLE hFile = CreateFileA(filename, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("CreateFile failed with error %d\n", GetLastError());
		return 1;
	}

	closeFile(hFile, FALSE);

	return 0;
}