#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

char board[105][160];
unsigned char array_hardness[16800];
int checkIfValid(int x,int y);
int switch_one_global = 1;
int switch_two_global = 1;
unsigned int version = 0;
unsigned int size = 0;
unsigned char * filetype;
int load_and_save = 1;
struct room
{
	int x;
	int y;
	int w;
	int h;
};
struct room rooms[11];
int numOfRooms = 11;
char * home;
char * path;
void printBoard();
void connectRooms();
void loadDungeon();
void saveDungeon(int from_load,char* filetype,int version, int size, unsigned char buffer_hardness[],struct room load_rooms[]);



int main(int argc, char *argv[]){
	char * load = "--load";
	char * save = "--save";

	home = getenv("HOME");
	path = strcat(home,"/.rlg327/dungeon.bin");

	if(argc>=3){
		char * switch_one = argv[1];
		char * switch_two = argv[2];
		if(strcmp(switch_one,load)==0||strcmp(switch_two,load)==0){
			switch_one_global = 0;
		}
		if(strcmp(switch_one,save)==0||strcmp(switch_two,save)==0){
			switch_two_global = 0;
			}
	}

	if(argc==2){
		char * switch_one = argv[1];
		if(strcmp(switch_one,load)==0){
			switch_one_global = 0;
		}
		if(strcmp(switch_one,save)==0){
			switch_two_global = 0;
		}
	}

	if(switch_one_global == 0){
		loadDungeon();
	}

	srand(time(NULL));

	int i,j,z,hard_counter;
	hard_counter = 0;
	for(i =0;i<105;i++){
		for(j=0;j<160;j++){
			board[i][j] = ' ';
			int hardness = rand() % 254 + 1;
			if(i==0||j==0||i==104||j==159){
				//printf("%s\n","border");
				array_hardness[hard_counter] = (unsigned char)255;	
			}else{
				array_hardness[hard_counter] = (unsigned char) hardness;
			}
			hard_counter++;

		}
	}

	for(i=0;i<11;i++){
		int randX =  rand() % 151 + 1;
		int randY = rand() % 98 + 1;
		rooms[i].x = randX;
		rooms[i].y = randY;
		while(checkIfValid(randX,randY) == 1){
			randX = rand() % 151 + 1;
			randY = rand() % 98 + 1;	
			rooms[i].x = randX;
			rooms[i].y = randY;
		}

		for(z =0;z<7;z++){
			for(j=0;j<5;j++){
			board[randY + j][randX + z] = '.';
			}
		}		
	}
	
	if(load_and_save != 0){
		if(switch_two_global == 0) saveDungeon(1,NULL,0,0,NULL,NULL);
	}
	connectRooms();

	//printBoard();

return 0;
}

void printBoard(){
	int i,j;
	for(i =0;i<105;i++){
		printf("\n");
		for(j=0;j<160;j++){
			printf("%c",board[i][j]);	
		}		
	}
}

int checkIfValid(int x, int y){
	int i,j;
	for(i=0;i<9;i++){
		for(j=0;j<7;j++){
			if(board[y+j-1][x+i-1] == '.') return 1;
		}
	}
	return 0;
}

void connectRooms(){
	int i,j,z;
	for(i=0;i<10;i++){
		int x1 = rooms[i].x;
		int y1 = rooms[i].y;

		int x2 = rooms[i+1].x;
		int y2 = rooms[i+1].y;

		if(x1<x2){
		for(j=x1;j<x2;j++){
			if(board[y1][j] == ' ') board[y1][j]='#';

		}
		}else{
			for(j=x1;j>x2;j--){
				if(board[y1][j] == ' ') board[y1][j]='#';
			}
		}

		if(y1<y2){
		for(z=y1;z<y2;z++){
			if(board[z][x2] == ' ') board[z][x2]='#';
				
			}
		}else{
			for(z=y1;z>y2;z--){
				if(board[z][x2] == ' ') board[z][x2]='#';
			}
		}
	}
}

void loadDungeon(){
	FILE * file;
	char *filetype_buffer = malloc(12);
	unsigned char buffer_hardness[16800];
	unsigned int * version_buffer = malloc(sizeof(unsigned int));
	unsigned int * size_buffer = malloc(sizeof(unsigned int));
	int i;

	file = fopen(path,"r");
	if(file==NULL) printf("%s\n","File is NULL");
	fread(filetype_buffer,12,1,file);
	fread(version_buffer,4,1,file);
	fread(size_buffer,4,1,file);
	fread(buffer_hardness,16800,1,file);

	version = *version_buffer;
	size = *size_buffer;


	printf("%s\n",filetype_buffer);
	printf("%d\n",*version_buffer);
	printf("%d\n",*size_buffer);
	numOfRooms = (*size_buffer-16820)/4;

	struct room load_rooms[numOfRooms];
	unsigned char load_hardness[16800];
	printf("%d\n",numOfRooms);

	for(i=0;i<numOfRooms;i++){
		uint8_t * x = malloc(sizeof(uint8_t));
		uint8_t * y = malloc(sizeof(uint8_t));
		uint8_t * w = malloc(sizeof(uint8_t));
		uint8_t * h = malloc(sizeof(uint8_t));

		fread(x,sizeof(uint8_t),1,file);
		fread(y,sizeof(uint8_t),1,file);
		fread(w,sizeof(uint8_t),1,file);
		fread(h,sizeof(uint8_t),1,file);
		printf("%hhu\n",*x);
		printf("%hhu\n",*y);
		printf("%hhu\n",*w);
		printf("%hhu\n",*h);
		load_rooms[i].x = *x;
		load_rooms[i].y = *y;
		load_rooms[i].h = *h;
		load_rooms[i].w = *w; 
		free(x);
		free(y);
		free(h);
		free(w);
	}
	fclose(file);
	//free(path);

	if(switch_two_global==0 && switch_one_global!=0){
		saveDungeon(0,filetype_buffer,*version_buffer,*size_buffer,buffer_hardness,load_rooms);
		load_and_save = 0;
	}

}

void saveDungeon(int from_load,char* filetype,int version,int size,unsigned char buffer_hardness[],struct room load_rooms[]){

	if(from_load == 0){
		printf("%s\n","from_load");
		FILE * file;
		int version = 0;
		unsigned int size = 16820 + (11 * 4);
		int z,i;	

	file = fopen(path,"w");
	if(file==NULL) printf("%s\n","File is NULL");
		fwrite(filetype,strlen(filetype),1,file);
		fwrite(&version,sizeof version,1,file); 
		fwrite(&size,sizeof size,1,file);
		fwrite(&array_hardness,sizeof array_hardness,1,file);

	for(i=0;i<numOfRooms;i++){
		uint8_t x = load_rooms[i].x;
		uint8_t y = load_rooms[i].y;
		uint8_t w = load_rooms[i].w;
		uint8_t h = load_rooms[i].h;
		fwrite(&x,sizeof(uint8_t),1,file);
		fwrite(&y,sizeof(uint8_t),1,file);
		fwrite(&w,sizeof(uint8_t),1,file);
		fwrite(&h,sizeof(uint8_t),1,file);
	}	
	fclose(file);

	}else{

		char * filetype_gen = "RLG327-S2017";
		FILE * file;
		int version_gen = 0;
		unsigned int size_gen = 16820 + (11 * 4);
		int z,i;

		printf("%s\n", "not from load");
		printf("%s\n", path);

		file = fopen(path,"w");
		if(file==NULL) printf("%s\n","File is NULL");
		fwrite(filetype_gen,strlen(filetype_gen),1,file);
		fwrite(&version_gen,sizeof version_gen,1,file); 
		fwrite(&size_gen,sizeof size_gen,1,file);
		fwrite(&array_hardness,sizeof array_hardness,1,file);

	for(i=0;i<numOfRooms;i++){
		uint8_t x = rooms[i].x;
		uint8_t y = rooms[i].y;
		uint8_t w = 7;
		uint8_t h = 5;
		fwrite(&x,sizeof(uint8_t),1,file);
		fwrite(&y,sizeof(uint8_t),1,file);
		fwrite(&w,sizeof(uint8_t),1,file);
		fwrite(&h,sizeof(uint8_t),1,file);
	}	
		fclose(file);		
	}
}
