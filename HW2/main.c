#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char board[105][160];
int checkIfValid(int x,int y);
void printBoard();
void connectRooms();
struct room
{
	int x;
	int y;
};
struct room rooms[11];


int main(int argc, char *argv[]){
	srand(time(NULL));

	int i,j,z;
	for(i =0;i<160;i++){
		for(j=0;j<105;j++){
			board[j][i] = ' ';
		}
	}

	for(i=0;i<11;i++){
		int randX =  rand() % 151 + 1;
		int randY = rand() % 98 + 1;
		rooms[i].x = randX;
		rooms[i].y = randY;
		printf("%d\n",randX);
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
	connectRooms();
	printBoard();

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
