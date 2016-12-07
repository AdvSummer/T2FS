#include <stdio.h>
#include <t2fs.h>


int main () {

	int handle = open2("/arq");
	char buffer[50];
	read2(handle, buffer, 41);
	printf("hi\n");
	printf("%s\n", buffer);

	
	return 0;
}