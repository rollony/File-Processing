#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "blockmap.h"

FILE *flashfp;

/****************  prototypes ****************/
void ftl_open();
void ftl_write(int lsn, char *sectorbuf);
void ftl_read(int lsn, char *sectorbuf);

int main(int argc, char *argv[])
{
	char *blockbuf;
    char sectorbuf[SECTOR_SIZE];
	int lsn, i;

    devicefp = fopen("flashmemory", "w+b");
	if(flashfp == NULL)
	{
		printf("file open error\n");
		exit(1);
	}
	   
    
    // flash memory의 모든 바이트를 '0xff'로 초기화한다.
    blockbuf = (char *)malloc(BLOCK_SIZE);
	memset(blockbuf, 0xFF, BLOCK_SIZE);

	for(i = 0; i < BLOCKS_PER_DEVICE; i++)
	{
		fwrite(blockbuf, BLOCK_SIZE, 1, flashfp);
	}

	free(blockbuf);

	ftl_open();    // ftl_read(), ftl_write() 호출하기 전에 이 함수를 반드시 호출해야 함


	fclose(flashfp);

	return 0;
}
