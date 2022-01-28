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

}