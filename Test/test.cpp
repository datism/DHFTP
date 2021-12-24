#include <string.h>
#include <stdio.h>
#include <fileapi.h>

#pragma comment(lib, "Ws2_32.lib")

int main() {
	HANDLE file = CreateFile("test.txt", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

}