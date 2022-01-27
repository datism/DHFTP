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
	int a = 1;
	for (; 1; cout << "AA") {
		switch (a)
		{
		case 1: continue;
		case 2: printf("asdasdasd"); break;
		default:
			break;
		}

		cout << "asdasdasd";
	}
}