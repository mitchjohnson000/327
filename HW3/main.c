#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

char board[105][160];
unsigned char array_hardness[16800];
int checkIfValid(int x,int y);
int switch_one_global = 1;
int switch_two_global = 1;
void printBoard();
void connectRooms();
void loadDungeon();
void saveDungeon();
struct room
{
	int x;
	int y;
};
struct room rooms[11];


int main(int argc, char *argv[]){
	char * load = "--load";
	char * save = "--save";

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
	
	if(switch_two_global == 0){
		saveDungeon();
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
	printf("%s\n","load");

}

void saveDungeon(){

	char * home = getenv("HOME");
	char *path = strcat(home,"/.rlg327/dungeon.bin");
	char * filetype = "RLG327-S2017";
	FILE * file;
	int version = 0;
	unsigned int size = 16820 + (11 * 4);
	int numOfRooms = 11;
	char *filetype_buffer = malloc(strlen(filetype));
	unsigned char buffer_hardness[16800];
	unsigned int * version_buffer = malloc(sizeof(unsigned int));
	unsigned int * size_buffer = malloc(sizeof(unsigned int));
	int z;
	for(z=0;z<16800;z++){
		printf("%u\n",array_hardness[z]);
	}


	printf("%s\n", path);
	file = fopen(path,"w+");
	if(file==NULL){
		printf("%s\n","File is NULL");
	}
	fwrite(filetype,strlen(filetype),1,file);
	fwrite(&version,sizeof version,1,file); 
	fwrite(&size,sizeof size,1,file);
	fwrite(&array_hardness,sizeof array_hardness,1,file);
	fseek(file, SEEK_SET, 0);
	fread(filetype_buffer,strlen(filetype),1,file);
	fread(version_buffer,sizeof version,1,file);
	fread(size_buffer,sizeof size,1,file);
	fread(buffer_hardness,sizeof array_hardness,1,file);

	//printf("%s\n",buffer_hardness);

	fclose(file);
	}
