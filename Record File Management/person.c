#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "person.h"
int page_num, record_num, del_page, del_record;	//레코드 파일의 헤더에 해당하는 정보 (전체 페이지수, 전체 레코드수, 최근 삭제된 레코드의 페이지 번호, 최근 삭제된 레코드의 레코드 번호)
int valid_record_count;	//삭제 작업 후 유효한 총 레코드의 수
void readPage(FILE *fp, char *pagebuf, int pagenum)	//페이지 단위로 버퍼에 읽어오는 함수
{
	
	fseek(fp, 16 + (pagenum)*PAGE_SIZE, SEEK_SET);
	fread(pagebuf, PAGE_SIZE, 1, fp);
	

}


void writePage(FILE *fp, const char *pagebuf, int pagenum)	//페이지 단위로 버퍼를 기록하는 함수
{
	
	fseek(fp, 16 + (pagenum)*PAGE_SIZE, SEEK_SET);
	fwrite(pagebuf, PAGE_SIZE, 1, fp);
	
	
}
 
void pack(char *recordbuf, const Person *p)	//Person 구조체를 파일에 저장할 레코드 형식으로 변환
{
	
	strcat(recordbuf, p->id);
	strcat(recordbuf, "#");
	strcat(recordbuf, p->name);
	strcat(recordbuf, "#");
	strcat(recordbuf, p->age);
	strcat(recordbuf, "#");
	strcat(recordbuf, p->addr);
	strcat(recordbuf, "#");
	strcat(recordbuf, p->phone);
	strcat(recordbuf, "#");
	strcat(recordbuf, p->email);
	strcat(recordbuf, "#");
		
}


void unpack(const char *recordbuf, Person *p)	//레코드 형식의 버퍼를 Person 구조체로 변환
{
	char delimter[1] = "#";	//필드 구분 지시자
	char *token;

	token = strtok(recordbuf, delimter);
	strcpy(p->id, token);
	token = strtok(NULL, delimter);
	strcpy(p->name, token);
	token = strtok(NULL, delimter);
	strcpy(p->age, token);
	token = strtok(NULL, delimter);
	strcpy(p->addr, token);
	token = strtok(NULL, delimter);
	strcpy(p->phone, token);
	token = strtok(NULL, delimter);
	strcpy(p->email, token);

	
}


void add(FILE *fp, const Person *p)	//레코드를 파일에 추가하는 함수
{
	char recordbuf[MAX_RECORD_SIZE];	//레코드의 정보를 담는 버퍼
	char pagebuf[PAGE_SIZE];	//기록할 페이지의 정보를 담는 버퍼
	char readbuf[PAGE_SIZE];	//읽어올 페이지의 정보를 담는 버퍼
	int count = 0;	
	_Bool written = 0;	//레코드가 정상적으로 기록되었는지 확인하기 위한 값
	
	memset(recordbuf, 0, MAX_RECORD_SIZE);	//레코드 버퍼 초기화
	pack(recordbuf, p);	//레코드 버퍼 압축
	
	if(del_page==-1 && del_record ==-1){	//삭제된 레코드 연결 리스트에 아직 정보가 없는 경우
		char check_pagebuf[PAGE_SIZE];
		int current_page_records;
		int current_page_last_offset;
		int current_page_last_length;
		
		if(page_num==0 && record_num==0){
			//처음으로 기록할 레코드일 
			int temp_offset = 0;
			int temp_length = strlen(recordbuf);
			current_page_records = 1;
			memset(pagebuf, 0, PAGE_SIZE);
			memcpy(pagebuf, &current_page_records, 4);
			memcpy(pagebuf+4, &temp_offset, 4);
			memcpy(pagebuf+8, &temp_length, 4);
			memcpy(pagebuf+HEADER_AREA_SIZE, recordbuf, sizeof(recordbuf));
			page_num = page_num + 1;
			record_num = record_num + 1;
			writePage(fp, pagebuf, page_num-1);

			fseek(fp, 0, SEEK_SET);
			fwrite((void*)&page_num, sizeof(int), 1, fp);
			fwrite((void*)&record_num, sizeof(int), 1, fp);
			written = 1;
			readPage(fp, check_pagebuf, page_num-1);
			memcpy(&current_page_records, check_pagebuf, 4);
			memcpy(&current_page_last_offset, check_pagebuf + 4 + (8*(current_page_records-1)), 4);
			memcpy(&current_page_last_length, check_pagebuf + 8 + (8*(current_page_records-1)), 4);	
			
		}
		else{
			readPage(fp, check_pagebuf, page_num-1);
			memcpy(&current_page_records, check_pagebuf, 4);
			memcpy(&current_page_last_offset, check_pagebuf + 4 + (8*(current_page_records-1)), 4);
			memcpy(&current_page_last_length, check_pagebuf + 8 + (8*(current_page_records-1)), 4);
			if(((HEADER_AREA_SIZE-4)-(8*current_page_records)>=8) && (strlen(recordbuf)<=(DATA_AREA_SIZE-(current_page_last_offset+current_page_last_length)))){	
				//존재하는 페이지에 기록
				char temp_header[HEADER_AREA_SIZE];
				int prev_offset;
				int prev_length;

				readPage(fp, readbuf, page_num-1);
				memcpy(temp_header, readbuf, HEADER_AREA_SIZE);
				memcpy(&current_page_records, temp_header, 4);
				memcpy(&prev_offset, temp_header + 4 + (8*(current_page_records-1)), 4);
				memcpy(&prev_length, temp_header + 8 + (8*(current_page_records-1)), 4);

				current_page_records = current_page_records + 1;
				int new_offset = prev_offset + prev_length;	
				int new_length = strlen(recordbuf);	
				memset(pagebuf, 0, PAGE_SIZE);
				memcpy(pagebuf, temp_header, HEADER_AREA_SIZE);
				memcpy(pagebuf, &current_page_records, 4); 
				memcpy(pagebuf + 4 + (8*(current_page_records-1)), &new_offset, 4);
				memcpy(pagebuf + 8 + (8*(current_page_records-1)), &new_length, 4);
				memcpy(pagebuf + HEADER_AREA_SIZE, readbuf+ HEADER_AREA_SIZE, DATA_AREA_SIZE);
				memcpy(pagebuf + HEADER_AREA_SIZE + new_offset, recordbuf, strlen(recordbuf)); 
				writePage(fp, pagebuf, page_num-1);
				written = 1;
				record_num = record_num + 1;
				fseek(fp, 0, SEEK_SET);
				fwrite((void*)&page_num, sizeof(int), 1, fp);
				fwrite((void*)&record_num, sizeof(int), 1, fp);
			}
			else{	//새로운 페이지 
				
				int temp_offset = 0;
				int temp_length = strlen(recordbuf);
				current_page_records = 1;
				memset(pagebuf, 0, PAGE_SIZE);
				memcpy(pagebuf, &current_page_records, 4);
				memcpy(pagebuf+4, &temp_offset, 4);
				memcpy(pagebuf+8, &temp_length, 4);
				memcpy(pagebuf+HEADER_AREA_SIZE, recordbuf, strlen(recordbuf));
				page_num = page_num + 1; 
				writePage(fp, pagebuf, page_num-1);
				record_num = record_num + 1;
				written = 1;
				fseek(fp, 0, SEEK_SET);
				fwrite((void*)&page_num, sizeof(int), 1, fp);
				fwrite((void*)&record_num, sizeof(int), 1, fp);
			}
		}
		
	}
	
	else{	//삭제된 레코드가 존재해서 연결 리스트 상에서 들어갈 위치를 찾는 경우
		int next_del_page, next_del_record;	//다음 탐색할 레코드의 페이지 및 레코드 번호 
		int curr_del_page, curr_del_record;	//현재 탐색하고 있는 레코드의 페이지 및 레코드 번호
		char temp_header[HEADER_AREA_SIZE];	//헤더 정보 임시 저장
		int check_del_offset;
		int check_del_length;
		int prev_del_page = -1; 	//이전 탐색한 레코드의 페이지 번호
		int prev_del_record = -1;	//이전 탐색한 레코드의 레코드 번호
		
		curr_del_page = del_page;
		curr_del_record = del_record;
		while(curr_del_page!=-1 && curr_del_record!=-1){	//search first fit	
			readPage(fp, readbuf, curr_del_page);
			memcpy(temp_header, readbuf, HEADER_AREA_SIZE);
			memcpy(&check_del_offset, temp_header + 4 + (curr_del_record*8), 4);	//get offset of del_record
			memcpy(&check_del_length, temp_header + 8 + (curr_del_record*8), 4);
			
			memcpy(&next_del_page, readbuf + HEADER_AREA_SIZE + check_del_offset+1, 4);
			memcpy(&next_del_record, readbuf + HEADER_AREA_SIZE + check_del_offset+5, 4);
			
			if(strlen(recordbuf)<=check_del_length){	//해당 자리에 공간이 남을 경우 레코드 
				memset(pagebuf, 0, PAGE_SIZE);
				memcpy(pagebuf, temp_header, HEADER_AREA_SIZE);
				memcpy(pagebuf + HEADER_AREA_SIZE, readbuf+HEADER_AREA_SIZE, DATA_AREA_SIZE);
				memcpy(pagebuf + HEADER_AREA_SIZE + check_del_offset, recordbuf, strlen(recordbuf));
				written = 1;
				if(prev_del_page == -1 && prev_del_record == -1){
					writePage(fp, pagebuf, curr_del_page);
					del_page = next_del_page;
					del_record = next_del_record;
					fseek(fp, 8, SEEK_SET);
					fwrite(&del_page, 4, 1, fp);
					fseek(fp, 12, SEEK_SET);
					fwrite(&del_record, 4, 1, fp);
				}
				
				else{
					char temp_readbuf[PAGE_SIZE];
					char temp_writebuf[PAGE_SIZE];
					char temp_pagebuf[PAGE_SIZE];
					
					
					
					if(prev_del_page != curr_del_page){
						int prev_del_offset;
						int prev_del_length;
						readPage(fp, temp_readbuf, prev_del_page);
						memcpy(&prev_del_offset, temp_readbuf + 4 + (8*(prev_del_record)), 4);
						memcpy(&prev_del_length, temp_readbuf + 8 + (8*(prev_del_record)), 4);
						memcpy(temp_pagebuf, temp_readbuf, PAGE_SIZE);
						if(next_del_page==-1 && next_del_record==-1){
							int x = -1;
							int y = -1;
							memcpy(temp_pagebuf + HEADER_AREA_SIZE + prev_del_offset + 1, &x, 4);
							memcpy(temp_pagebuf + HEADER_AREA_SIZE + prev_del_offset + 5, &x, 4);
						}
						else{
							memcpy(temp_pagebuf + HEADER_AREA_SIZE + prev_del_offset + 1, &next_del_page, 4);
							memcpy(temp_pagebuf + HEADER_AREA_SIZE + prev_del_offset + 5, &next_del_record, 4);
						}
						writePage(fp, temp_pagebuf, prev_del_page);
						writePage(fp, pagebuf, curr_del_page);
					}
					else if(prev_del_page == curr_del_page){
						int prev_del_offset;
						int prev_del_length;
						readPage(fp, temp_readbuf, curr_del_page);
						memcpy(&prev_del_offset, temp_readbuf + 4 + (8*(prev_del_record)), 4);
						memcpy(&prev_del_length, temp_readbuf + 8 + (8*(prev_del_record)), 4);
						if(next_del_page==-1 && next_del_record==-1){
							int x = -1;
							int y = -1;
							memcpy(pagebuf + HEADER_AREA_SIZE + prev_del_offset + 1, &x, 4);
							memcpy(pagebuf + HEADER_AREA_SIZE + prev_del_offset + 5, &x, 4);
						}
						else{
							memcpy(pagebuf + HEADER_AREA_SIZE + prev_del_offset + 1, &next_del_page, 4);
							memcpy(pagebuf + HEADER_AREA_SIZE + prev_del_offset + 5, &next_del_record, 4);
						}
						writePage(fp, pagebuf, curr_del_page);
						
					}
					
				}
				
				break;
			}
			prev_del_page = curr_del_page;
			prev_del_record = curr_del_record;
			
			curr_del_page = next_del_page;
			curr_del_record = next_del_record;
			
			
		}
		
		if(!written && (strlen(recordbuf)<=check_del_length)){	//if the last record in the linked list is available
			memset(pagebuf, 0, PAGE_SIZE);
			memcpy(pagebuf, temp_header, HEADER_AREA_SIZE);
			memcpy(pagebuf + HEADER_AREA_SIZE, readbuf+HEADER_AREA_SIZE, DATA_AREA_SIZE);
			memcpy(pagebuf + HEADER_AREA_SIZE + check_del_offset, recordbuf, strlen(recordbuf));
			written = 1;
					
			char temp_readbuf[PAGE_SIZE];
			char temp_writebuf[PAGE_SIZE];
			char temp_pagebuf[PAGE_SIZE];
						
			if(prev_del_page != curr_del_page){
				int prev_del_offset;
				int prev_del_length;
				readPage(fp, temp_readbuf, prev_del_page);
				memcpy(&prev_del_offset, temp_readbuf + 4 + (8*(prev_del_record)), 4);
				memcpy(&prev_del_length, temp_readbuf + 8 + (8*(prev_del_record)), 4);
				memcpy(temp_pagebuf, temp_readbuf, PAGE_SIZE);
				if(next_del_page==-1 && next_del_record==-1){
					int x = -1;
					int y = -1;
					memcpy(temp_pagebuf + HEADER_AREA_SIZE + prev_del_offset + 1, &x, 4);
					memcpy(temp_pagebuf + HEADER_AREA_SIZE + prev_del_offset + 5, &x, 4);
				}
				else{
					memcpy(temp_pagebuf + HEADER_AREA_SIZE + prev_del_offset + 1, &next_del_page, 4);
					memcpy(temp_pagebuf + HEADER_AREA_SIZE + prev_del_offset + 5, &next_del_record, 4);
				}
				writePage(fp, temp_pagebuf, prev_del_page);
				writePage(fp, pagebuf, curr_del_page);
			}
			else if(prev_del_page == curr_del_page){
				int prev_del_offset;
				int prev_del_length;
				readPage(fp, temp_readbuf, curr_del_page);
				memcpy(&prev_del_offset, temp_readbuf + 4 + (8*(prev_del_record)), 4);
				memcpy(&prev_del_length, temp_readbuf + 8 + (8*(prev_del_record)), 4);
				if(next_del_page==-1 && next_del_record==-1){
					int x = -1;
					int y = -1;
					memcpy(pagebuf + HEADER_AREA_SIZE + prev_del_offset + 1, &x, 4);
					memcpy(pagebuf + HEADER_AREA_SIZE + prev_del_offset + 5, &x, 4);
				}
				else{
					memcpy(pagebuf + HEADER_AREA_SIZE + prev_del_offset + 1, &next_del_page, 4);
					memcpy(pagebuf + HEADER_AREA_SIZE + prev_del_offset + 5, &next_del_record, 4);
				}
				
				writePage(fp, pagebuf, curr_del_page);
						
			}
					
		}
		
		if(!written){	//삭제된 레코드를 관리하는 연결 리스트에 새로운 레코드를 추가할 공간이 없을 시 
			char check_pagebuf[PAGE_SIZE];
			int current_page_records;
			int current_page_last_offset;
			int current_page_last_length;
			
			readPage(fp, check_pagebuf, page_num-1);
			memcpy(&current_page_records, check_pagebuf, 4);
			memcpy(&current_page_last_offset, check_pagebuf + 4 + (8*(current_page_records-1)), 4);
			memcpy(&current_page_last_length, check_pagebuf + 8 + (8*(current_page_records-1)), 4);
			
			if(((HEADER_AREA_SIZE-4)-(8*current_page_records)>=8) && (strlen(recordbuf)<=(DATA_AREA_SIZE-(current_page_last_offset+current_page_last_length)))){	
				//이미 존재하는 페이지에 기록
				char temp_header[HEADER_AREA_SIZE];
				int prev_offset;
				int prev_length;

				readPage(fp, readbuf, page_num-1);
				memcpy(temp_header, readbuf, HEADER_AREA_SIZE);
				memcpy(&current_page_records, temp_header, 4);
				memcpy(&prev_offset, temp_header + 4 + (8*(current_page_records-1)), 4);
				memcpy(&prev_length, temp_header + 8 + (8*(current_page_records-1)), 4);
				//new pagebuf	
				current_page_records = current_page_records + 1;
				int new_offset = prev_offset + prev_length;	
				int new_length = strlen(recordbuf);	
				memset(pagebuf, 0, PAGE_SIZE);
				memcpy(pagebuf, temp_header, HEADER_AREA_SIZE);
				memcpy(pagebuf, &current_page_records, 4); 
				memcpy(pagebuf + 4 + (8*(current_page_records-1)), &new_offset, 4);
				memcpy(pagebuf + 8 + (8*(current_page_records-1)), &new_length, 4);
				memcpy(pagebuf + HEADER_AREA_SIZE, readbuf+ HEADER_AREA_SIZE, DATA_AREA_SIZE);
				memcpy(pagebuf + HEADER_AREA_SIZE + new_offset, recordbuf, strlen(recordbuf)); 
				writePage(fp, pagebuf, page_num-1);
				written = 1;
				record_num = record_num + 1;
				fseek(fp, 0, SEEK_SET);
				fwrite((void*)&page_num, sizeof(int), 1, fp);
				fwrite((void*)&record_num, sizeof(int), 1, fp);
			}
			else{	//새로운 페이지 생성
				
				int temp_offset = 0;
				int temp_length = strlen(recordbuf);
				current_page_records = 1;
				memset(pagebuf, 0, PAGE_SIZE);
				memcpy(pagebuf, &current_page_records, 4);
				memcpy(pagebuf+4, &temp_offset, 4);
				memcpy(pagebuf+8, &temp_length, 4);
				memcpy(pagebuf+HEADER_AREA_SIZE, recordbuf, strlen(recordbuf));
				page_num = page_num + 1; 
				writePage(fp, pagebuf, page_num-1);
				record_num = record_num + 1;
				written = 1;
				fseek(fp, 0, SEEK_SET);
				fwrite((void*)&page_num, sizeof(int), 1, fp);
				fwrite((void*)&record_num, sizeof(int), 1, fp);
			}
		}
		
	}
}


void delete(FILE *fp, const char *id)	//주민등록번호를 입력 받아 해당 레코드를 삭제하는 함수
{
	char readbuf[PAGE_SIZE];	//읽어들이는 페이지 버퍼
	char sym[1] = "*";	//삭제 레코드 지시자
	for(int i = 0; i<page_num; i++){	//전체 페이지에서 해당 레코드를 탐색
		readPage(fp, readbuf, i);
		int current_page_records;
		memcpy(&current_page_records, readbuf, 4);
		for(int j = 0; j<current_page_records; j++){	//현재 페이지에서 해당 레코드 
			struct _Person person;
			int offset;
			int length;
			char recordbuf[MAX_RECORD_SIZE];
			memcpy(&offset, readbuf + 4 + (8*j), 4);
			memcpy(&length, readbuf + 8 + (8*j), 4);
			memcpy(recordbuf, readbuf + HEADER_AREA_SIZE + offset, length);
			unpack(recordbuf, &person);
			if(strcmp(person.id, id)==0){
				if(del_page==-1 && del_record==-1){	//첫 번째 삭제일 경우
					char writebuf[PAGE_SIZE];
					memcpy(writebuf, readbuf, PAGE_SIZE);
					memcpy(writebuf + HEADER_AREA_SIZE + offset, &sym, 1);
					memcpy(writebuf + HEADER_AREA_SIZE + offset + 1, &del_page, 4);
					memcpy(writebuf + HEADER_AREA_SIZE + offset + 5, &del_record, 4);
					writePage(fp, writebuf, i);
					del_page = i;
					del_record = j;
					fseek(fp, 8, SEEK_SET);
					fwrite(&del_page, 4, 1, fp);
					fwrite(&del_record, 4, 1, fp);
					
					return;
				}
				else{	//첫 번째 삭제가 아닐 경우 (연결 리스트에 추가)
					char writebuf[PAGE_SIZE];
					int temp_del_page = -1;
					int temp_del_record = -1;
					int curr_del_page = del_page;
					int curr_del_record = del_record;
					
					memcpy(writebuf, readbuf, PAGE_SIZE);
					memcpy(writebuf + HEADER_AREA_SIZE + offset, &sym, 1);
					memcpy(writebuf + HEADER_AREA_SIZE + offset + 1, &del_page, 4);
					memcpy(writebuf + HEADER_AREA_SIZE + offset + 5, &del_record, 4);
					writePage(fp, writebuf, i);
					del_page = i;
					del_record = j;
					fseek(fp, 8, SEEK_SET);
					fwrite(&del_page, 4, 1, fp);
					fwrite(&del_record, 4, 1, fp);
					
					
					return;
				}
			}
		}
	}
}

void createIndex(FILE *idxfp, FILE *recordfp){	//레코드 파일에 대한 Simple Index
	int page_count = 0;	
	char id[13];	//주민등록번호 담는 버퍼
	char check_del;	//삭제 레코드인지 확인하기 위한 임시 문자
	long long id_array[record_num][3];	
	char pagebuf[PAGE_SIZE];
	char sym[1] = "*";	//삭제 레코드 지시자
	valid_record_count = 0;	
	for(int i = 0; i<page_num; i++){
		int records_cur_page = 0;
		readPage(recordfp, pagebuf, i);
		memcpy(&records_cur_page, pagebuf, 4);
		int num = records_cur_page;
		for(int j = 0; j<num; j++){
			int offset;
			memcpy(&offset, pagebuf+4+(j*8), 4);
			memcpy(&check_del, pagebuf + HEADER_AREA_SIZE + offset, 1);
			if(check_del!=42){
				memcpy(&id, pagebuf + HEADER_AREA_SIZE + offset, 13);
				id_array[valid_record_count][0] = atoll(id);
				id_array[valid_record_count][1] = i;
				id_array[valid_record_count][2] = j;
				valid_record_count++;
			}
		}
		
	}
	
	long long temp_id;	//오름차순으로 정렬하기 위한 임시 변수
	int temp_page, temp_record;	
	if(valid_record_count>=2){	//인덱스 파일 생성을 위해 오름차순 정렬
		for(int i = 0; i<valid_record_count; i++){
			for(int j = 0; j<valid_record_count-1-i; j++){
				if(id_array[j][0]>id_array[j+1][0]){
					temp_id = id_array[j][0];
					id_array[j][0] = id_array[j+1][0];
					id_array[j+1][0] = temp_id;
					
					temp_page = id_array[j][1];
					id_array[j][1] = id_array[j+1][1];
					id_array[j+1][1] = temp_page;
				
					temp_record = id_array[j][2];
					id_array[j][2] = id_array[j+1][2];
					id_array[j+1][2] = temp_record;
				}
			}
		}
	}
	
	fseek(idxfp, 0, SEEK_SET);
	fwrite((void*)&valid_record_count, sizeof(int), 1, idxfp);
	
	for(int i = 0; i<valid_record_count; i++){	//유효한 레코드 수 만큼 인덱스 파일에 저장
		char id[13];
		sprintf(id, "%ld", id_array[i][0]);
		fwrite(&id, 13, 1, idxfp);
		fwrite(&id_array[i][1], 4, 1, idxfp);
		fwrite(&id_array[i][2], 4, 1, idxfp);
	}
	
}

void binarysearch(FILE *idxfp, const char *id, int *pageNum, int *recordNum){	//이진탐색을 통해 레코드 
	int reads = 0;	//탐색시 레코드를 읽은 횟수를 담는 변수
	int low = 0;	//저점
	int high = valid_record_count-1;	//고점
	long long find_id = atoll(id);	//인자로 받은 주민등록번호를 Long Long으로 변환
	char temp[13];	//
	long long temp_id;
	fseek(idxfp, 0, SEEK_SET);
	while(low<=high){	//이진탐색 진행
		int mid = (low + high)/2;
		fseek(idxfp, 4+(mid*21), SEEK_SET);
		fread(&temp, 13, 1, idxfp);
		temp_id = atoll(temp);
		reads++;
		if(find_id == temp_id){
			int temp_pageNum, temp_recordNum;
			fread(&temp_pageNum, 4, 1, idxfp);
			fread(&temp_recordNum, 4, 1, idxfp);
			*pageNum = temp_pageNum;
			*recordNum = temp_recordNum;
			break;
		}
		else if(find_id < temp_id){
			high = mid -1;
		}
		else if(find_id > temp_id){
			low = mid + 1;
		}
	}
	printf("#reads:%d\n", reads);
	return;
}

int main(int argc, char *argv[])
{
	FILE *fp;  //레코드가 저장될 파일 포인터
	char header_record[4];	//헤더 레코드에 대한 정보를 담는 버퍼
	char pagebuf[PAGE_SIZE];	//임시 페이지 버퍼
	char recordbuf[MAX_RECORD_SIZE];	//임시 레코드 버퍼
	
	if(access(argv[2], F_OK)<0){	//인자로 받은 파일이 존재하지 않을 경우 새로 
		if((fp = fopen(argv[2], "w+"))<0){
			printf("file open error\n");
		}
		page_num = 0;	//새로운 파일 생성시 헤더 초기화
		record_num = 0;
		del_page = -1;
		del_record = -1;
		fwrite((void*)&page_num, sizeof(int), 1, fp);
		fwrite((void*)&record_num, sizeof(int), 1, fp);
		fwrite((void*)&del_page, sizeof(int), 1, fp);
		fwrite((void*)&del_record, sizeof(int), 1, fp);
	}
	else{
		if((fp = fopen(argv[2], "r+"))<0){
			printf("file open error\n");
		}	
		fread((void*)&page_num, sizeof(int), 1, fp);
		fread((void*)&record_num, sizeof(int), 1, fp);
		fread((void*)&del_page, sizeof(int), 1, fp);
		fread((void*)&del_record, sizeof(int), 1, fp);;
		
	} 
	
	
	if(argv[1][0]=='a'){	//인자로 'a'를 받을 시 새로운 레코드 추가
		struct _Person person;
		strcpy(person.id, argv[3]);
		strcpy(person.name, argv[4]);
		strcpy(person.age, argv[5]);
		strcpy(person.addr, argv[6]);
		strcpy(person.phone, argv[7]);
		strcpy(person.email, argv[8]);
		
		add(fp, &person);
		fseek(fp, 0, SEEK_SET);
		fread((void*)&page_num, sizeof(int), 1, fp);
		fread((void*)&record_num, sizeof(int), 1, fp);
		fread((void*)&del_page, sizeof(int), 1, fp);
		fread((void*)&del_record, sizeof(int), 1, fp);;
		fclose(fp);
		
	}
	
	else if(argv[1][0]=='d'){	//인자로 'd'를 받을 시 해당 레코드 삭제
		delete(fp, argv[3]);
		fclose(fp);
	}
	
	else if(argv[1][0]=='i'){	//인자로 'i'를 받을 시 전체 레코드 파일에 대한 인덱스 파일 생성
		FILE *idxfp;
		if((idxfp = fopen(argv[3], "w+"))<0){
			printf("file open error\n");
		}
		createIndex(idxfp,fp);
	}
	
	else if(argv[1][0]=='b'){	//인자로 'b'를 받을 시 주어진 주민등록번호로 이진탐색 진행
		int pageNum = -1;
		int recordNum = -1;
		char pagebuf[PAGE_SIZE];
		FILE *idxfp;
		if((idxfp = fopen(argv[3], "r"))<0){
			printf("file open error\n");
		}
		fseek(idxfp, 0, SEEK_SET);
		fread(&valid_record_count, 4, 1, idxfp);
		binarysearch(idxfp, argv[4], &pageNum, &recordNum);
		if(pageNum==-1 && recordNum==-1){	//주민등록번호에 해당하는 레코드를 찾지 못하였을 경우
			printf("no persons\n");		
		}
		else{
			struct _Person person;
			char recordbuf[MAX_RECORD_SIZE];
			int offset, length;
			readPage(fp, pagebuf, pageNum);
			memcpy(&offset, pagebuf + 4 + (recordNum*8), 4);
			memcpy(&length, pagebuf + 8 + (recordNum*8), 4);
			memcpy(&recordbuf, pagebuf + HEADER_AREA_SIZE + offset, length);
			unpack(recordbuf, &person);
			printf("id=%s\n", person.id);
			printf("name=%s\n", person.name);
			printf("age=%s\n", person.age);
			printf("addr=%s\n", person.addr);
			printf("phone=%s\n", person.phone);
			printf("email=%s\n", person.email);
		}
	}
	
	return 1;
}
