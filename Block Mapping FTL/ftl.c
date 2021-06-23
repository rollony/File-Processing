#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "blockmap.h"

int lbn_table[BLOCKS_PER_DEVICE];
int freeblock_num;

void ftl_open()
{
	//
	// address mapping table 초기화 또는 복구
	// free block's pbn 초기화
    	// address mapping table에서 lbn 수는 DATABLKS_PER_DEVICE 동일
    	
	char *sectorbuf;
	int lbn;
	_Bool free = 0;
	
	for(int i = 0; i<BLOCKS_PER_DEVICE; i++){
			lbn_table[i] = -1;
	}
	sectorbuf = (char*)malloc(PAGE_SIZE);	
	for(int i = 0; i<BLOCKS_PER_DEVICE; i++){
		dd_read(i*PAGES_PER_BLOCK, sectorbuf);
		memcpy(&lbn, sectorbuf+SECTOR_SIZE, 4);
		if(lbn!=0xffffffff){
			lbn_table[lbn] = i;
		}
		else if(lbn==0xffffffff && (!free)){
			free = 1;
			lbn_table[DATABLKS_PER_DEVICE] = i;
			freeblock_num = i;
		}
	}
		
	return;
}

void ftl_read(int lsn, char *sectorbuf)
{
	int offset = lsn%PAGES_PER_BLOCK;
	int page = lsn/PAGES_PER_BLOCK;
	char *tempbuf = (char*)malloc(PAGE_SIZE);
	int ppn = lbn_table[page]*PAGES_PER_BLOCK + offset;
	dd_read(ppn, tempbuf);
	memcpy(sectorbuf, tempbuf, SECTOR_SIZE);
	free(tempbuf);
	return;
}

void ftl_write(int lsn, char *sectorbuf)
{
	int tmplbn;
	int tmplsn;
	char tempbuf[PAGE_SIZE];
	char pagebuf[PAGE_SIZE];
	char pagebuf2[PAGE_SIZE];
	int offset = lsn%PAGES_PER_BLOCK;
	int lbn = lsn/PAGES_PER_BLOCK;
	int temp;

	if(lbn_table[lbn]==-1){	//인자로 받은 lsn값에 해당하는 psn이 설정되어 있지 않은 경우
		for(int i = 0; i<BLOCKS_PER_DEVICE; i++){	//블록마다 첫 번째 페이지 spare 영역의 lbn
			dd_read(i*PAGES_PER_BLOCK, tempbuf);
			memcpy(&tmplbn, tempbuf+SECTOR_SIZE, 4);
			if(tmplbn==-1 && (i!=freeblock_num)){	//해당 페이지에 첫 번째 쓰기일 경우
				memset(pagebuf2, -1, PAGE_SIZE);	
				memcpy(pagebuf2+SECTOR_SIZE, &lbn, 4);	//block's first page's spare lbn record
				dd_write(i*PAGES_PER_BLOCK, &pagebuf2);
				
				memcpy(pagebuf, sectorbuf, SECTOR_SIZE);	//기록할 페이지에 정보 저장
				memcpy(pagebuf+SECTOR_SIZE, &lbn, 4);
				memcpy(pagebuf+SECTOR_SIZE+4, &lsn, 4);		
				dd_write((i*PAGES_PER_BLOCK) + offset, &pagebuf);
					
				lbn_table[lbn] = i;
				break;
			}			
		}
	}
	else{	//인자로 받은 lsn값에 해당하는 psn이 존재하는 경우
		dd_read(lbn_table[lbn]*PAGES_PER_BLOCK + offset, tempbuf);
		memcpy(&tmplsn, tempbuf+SECTOR_SIZE+4, 4);
		if(tmplsn!=-1){	//해당 페이지가 최초 쓰기가 아닌 경우
			for(int i = 0; i<PAGES_PER_BLOCK; i++){	//해당 블록에 있는 내용을 freeblock으로 복사
				if(i!=offset){
					dd_read(lbn_table[lbn]*PAGES_PER_BLOCK + i, &tempbuf);
					memcpy(pagebuf, tempbuf,PAGE_SIZE);
					dd_write(freeblock_num*PAGES_PER_BLOCK + i, pagebuf);
				}
			}
			dd_erase(lbn_table[lbn]);

			memcpy(pagebuf2, sectorbuf, SECTOR_SIZE);
		       	memcpy(pagebuf2+SECTOR_SIZE, &lbn, 4);
			memcpy(pagebuf2+SECTOR_SIZE+4, &lsn, 4);	
			dd_write(freeblock_num*PAGES_PER_BLOCK + offset, pagebuf2);
					
			temp = lbn_table[lbn];
			lbn_table[lbn] = freeblock_num;
			freeblock_num = temp;	//원래 정보가 있던 블록을 freeblock으로 새로지정
			lbn_table[DATABLKS_PER_DEVICE] = freeblock_num;
		}
		else{	//해당 페이지가 최초 쓰기인 경우
			memcpy(pagebuf, sectorbuf, SECTOR_SIZE);
			memcpy(pagebuf+SECTOR_SIZE, &lbn, 4);
			memcpy(pagebuf+SECTOR_SIZE+4, &lsn, 4);
			dd_write(lbn_table[lbn]*PAGES_PER_BLOCK + offset, &pagebuf);
		}
	}
	
	return;
}

void ftl_print()
{
	printf("lbn pbn\n");
	for(int i = 0; i<DATABLKS_PER_DEVICE; i++){
		printf("%d %d\n", i, lbn_table[i]);
	}
	printf("free block's pbn=%d\n", freeblock_num);
	
	return;
}
