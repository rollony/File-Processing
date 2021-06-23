#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "person.h"
int page_num, record_num, del_page, del_record;
int valid_record_count;
void readPage(FILE *fp, char *pagebuf, int pagenum)
{
	
	fseek(fp, 16 + (pagenum)*PAGE_SIZE, SEEK_SET);
	fread(pagebuf, PAGE_SIZE, 1, fp);
	

}


void writePage(FILE *fp, const char *pagebuf, int pagenum)
{
	
	fseek(fp, 16 + (pagenum)*PAGE_SIZE, SEEK_SET);
	fwrite(pagebuf, PAGE_SIZE, 1, fp);
	
	
}
 
void pack(char *recordbuf, const Person *p)
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


void unpack(const char *recordbuf, Person *p)
{
	char delimter[1] = "#";
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


void add(FILE *fp, const Person *p)
{
	char recordbuf[MAX_RECORD_SIZE];
	char pagebuf[PAGE_SIZE];
	char readbuf[PAGE_SIZE];
	int count = 0;
	_Bool written = 0;
	
	//int current_page_records;
	memset(recordbuf, 0, MAX_RECORD_SIZE);
	pack(recordbuf, p);
	
	
	if(del_page==-1 && del_record ==-1){
		char check_pagebuf[PAGE_SIZE];
		int current_page_records;
		int current_page_last_offset;
		int current_page_last_length;
		
		if(page_num==0 && record_num==0){
			
			int temp_offset = 0;
			int temp_length = strlen(recordbuf);
			//printf("length : %d\n", temp_length);
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
			//printf("%d %d %d", current_page_records, current_page_last_offset, current_page_last_length);
			
			
		}
		else{
			readPage(fp, check_pagebuf, page_num-1);
			memcpy(&current_page_records, check_pagebuf, 4);
			memcpy(&current_page_last_offset, check_pagebuf + 4 + (8*(current_page_records-1)), 4);
			memcpy(&current_page_last_length, check_pagebuf + 8 + (8*(current_page_records-1)), 4);
			//printf("current page info : %d %d %d", current_page_records, current_page_last_offset, current_page_last_length);
			if(((HEADER_AREA_SIZE-4)-(8*current_page_records)>=8) && (strlen(recordbuf)<=(DATA_AREA_SIZE-(current_page_last_offset+current_page_last_length)))){	
				//write on existing page
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
			else{	//create new page
				
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
	
	else{
		int next_del_page, next_del_record;
		int curr_del_page, curr_del_record;
		char temp_header[HEADER_AREA_SIZE];
		int check_del_offset;
		int check_del_length;
		int prev_del_page = -1; 
		int prev_del_record = -1;
		
		curr_del_page = del_page;
		curr_del_record = del_record;
		while(curr_del_page!=-1 && curr_del_record!=-1){	//search first fit	
			readPage(fp, readbuf, curr_del_page);
			memcpy(temp_header, readbuf, HEADER_AREA_SIZE);
			memcpy(&check_del_offset, temp_header + 4 + (curr_del_record*8), 4);	//get offset of del_record
			memcpy(&check_del_length, temp_header + 8 + (curr_del_record*8), 4);
			
			memcpy(&next_del_page, readbuf + HEADER_AREA_SIZE + check_del_offset+1, 4);
			memcpy(&next_del_record, readbuf + HEADER_AREA_SIZE + check_del_offset+5, 4);
			
			if(strlen(recordbuf)<=check_del_length){	//add record in place if its available
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
		
		if(!written){	//no records to overwrite in linked list
			char check_pagebuf[PAGE_SIZE];
			int current_page_records;
			int current_page_last_offset;
			int current_page_last_length;
			
			readPage(fp, check_pagebuf, page_num-1);
			memcpy(&current_page_records, check_pagebuf, 4);
			memcpy(&current_page_last_offset, check_pagebuf + 4 + (8*(current_page_records-1)), 4);
			memcpy(&current_page_last_length, check_pagebuf + 8 + (8*(current_page_records-1)), 4);
			//printf("current page info : %d %d %d", current_page_records, current_page_last_offset, current_page_last_length);
			if(((HEADER_AREA_SIZE-4)-(8*current_page_records)>=8) && (strlen(recordbuf)<=(DATA_AREA_SIZE-(current_page_last_offset+current_page_last_length)))){	
				//write on existing page
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
			else{	//create new page
				
				//printf("need new page");
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
				//printf("write success, records: %d, pages: %d\n", record_num, page_num);
				written = 1;
				fseek(fp, 0, SEEK_SET);
				fwrite((void*)&page_num, sizeof(int), 1, fp);
				fwrite((void*)&record_num, sizeof(int), 1, fp);
			}
		}
		
	}
}


void delete(FILE *fp, const char *id)
{
	char readbuf[PAGE_SIZE];
	char sym[1] = "*";
	for(int i = 0; i<page_num; i++){
		readPage(fp, readbuf, i);
		int current_page_records;
		memcpy(&current_page_records, readbuf, 4);
		for(int j = 0; j<current_page_records; j++){
			struct _Person person;
			int offset;
			int length;
			char recordbuf[MAX_RECORD_SIZE];
			memcpy(&offset, readbuf + 4 + (8*j), 4);
			memcpy(&length, readbuf + 8 + (8*j), 4);
			memcpy(recordbuf, readbuf + HEADER_AREA_SIZE + offset, length);
			unpack(recordbuf, &person);
			//printf("person.id : %s\n", person.id);
			if(strcmp(person.id, id)==0){
				if(del_page==-1 && del_record==-1){	//if its the first time deleting
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
				else{	//not first time deleting (add to linked list)
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

void createIndex(FILE *idxfp, FILE *recordfp){
	int page_count = 0;
	
	char id[13];
	char check_del;
	long long id_array[record_num][3];
	char pagebuf[PAGE_SIZE];
	char sym[1] = "*";
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
	
	long long temp_id;
	int temp_page, temp_record;
	if(valid_record_count>=2){
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
	for(int i = 0; i<valid_record_count; i++){
		char id[13];
		
		sprintf(id, "%ld", id_array[i][0]);
		fwrite(&id, 13, 1, idxfp);
		fwrite(&id_array[i][1], 4, 1, idxfp);
		fwrite(&id_array[i][2], 4, 1, idxfp);
	}
	
}

void binarysearch(FILE *idxfp, const char *id, int *pageNum, int *recordNum){	
	int reads = 0;
	int low = 0;
	int high = valid_record_count-1;
	long long find_id = atoll(id);
	char temp[13];
	long long temp_id;
	fseek(idxfp, 0, SEEK_SET);
	while(low<=high){
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
	FILE *fp;  
	char header_record[4];
	char pagebuf[PAGE_SIZE];
	char recordbuf[MAX_RECORD_SIZE];
	
	if(access(argv[2], F_OK)<0){	//if file doesn't exist yet
		if((fp = fopen(argv[2], "w+"))<0){
			printf("file open error\n");
		}
		page_num = 0;
		record_num = 0;
		del_page = -1;
		del_record = -1;
		fwrite((void*)&page_num, sizeof(int), 1, fp);
		fwrite((void*)&record_num, sizeof(int), 1, fp);
		fwrite((void*)&del_page, sizeof(int), 1, fp);
		fwrite((void*)&del_record, sizeof(int), 1, fp);
		//printf("record header info : %d %d %d %d\n", page_num, record_num, del_page, del_record);
	}
	else{
		if((fp = fopen(argv[2], "r+"))<0){
			printf("file open error\n");
		}	
		fread((void*)&page_num, sizeof(int), 1, fp);
		fread((void*)&record_num, sizeof(int), 1, fp);
		fread((void*)&del_page, sizeof(int), 1, fp);
		fread((void*)&del_record, sizeof(int), 1, fp);;
		//printf("record header info : %d %d %d %d\n", page_num, record_num, del_page, del_record);
		
	
	} 
	
	
	if(argv[1][0]=='a'){
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
		//printf("record header info : %d %d %d %d\n", page_num, record_num, del_page, del_record);
		fclose(fp);
		
	}
	
	else if(argv[1][0]=='d'){
		delete(fp, argv[3]);
		fclose(fp);
	}
	
	else if(argv[1][0]=='i'){
		FILE *idxfp;
		if((idxfp = fopen(argv[3], "w+"))<0){
			printf("file open error\n");
		}
		createIndex(idxfp,fp);
	}
	
	else if(argv[1][0]=='b'){
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
		if(pageNum==-1 && recordNum==-1){
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
