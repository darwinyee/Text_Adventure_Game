/*******************************************************************************
** Author:       Darwin Yee, ONID 933915366
** Date:         10-17-2019
** Project 2:    The Game
** Description:  This program takes the room files and sets up the game.  
*******************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#define TRUE 1
#define FALSE 0
#define START_ROOM 0
#define MID_ROOM 1
#define END_ROOM 2


//Global mutex lock variable for multithread time reporting
pthread_mutex_t myLock = PTHREAD_MUTEX_INITIALIZER;

//This is the Room structure to store room related information
struct Room {
     char* roomName;
     int* connectedRoomIdx; //array of 6 slot, fill with the index of the room name 
     int roomType;
     int roomID;
     int connectionCt;
};

//This function initializes the room array, assuming the room name list has been built.
void InitializeRooms(char idxToRoom[][9], struct Room* roomlist, int roomCt){
	
	int i;
	for(i = 0; i < roomCt; i++){
		roomlist[i].roomName = malloc(sizeof(char)*9);
		assert(roomlist[i].roomName != 0);
		memset(roomlist[i].roomName, '\0', 9);
        strcpy(roomlist[i].roomName, idxToRoom[i]);
		
		roomlist[i].connectedRoomIdx = (int*)malloc(sizeof(int)*6);
		assert(roomlist[i].connectedRoomIdx != 0);
		roomlist[i].roomType = MID_ROOM;
		roomlist[i].connectionCt = 0;
		roomlist[i].roomID = i;
	}
	
}

//This function retrieves the start room, assuming curLocation is a string set to '\0';
void GetStartRoom(char* curLocation, struct Room* roomlist, int roomCt){
	
	int i;
	for(i = 0; i < roomCt; i++){
		if(roomlist[i].roomType == START_ROOM){
			strcpy(curLocation, roomlist[i].roomName);
			return;
		}
	}
	
}

int ReadRoomDir(char idxToRoom[][9], struct Room* roomlist, int roomCt){
	
	//get the newest room directory
	char newestDir[256];
	memset(newestDir,'\0',sizeof(newestDir));
	if(!GetNewestRoomDirectory(newestDir, 256)){
		
		//set up the array containing all the names of the rooms
		int status = GetRoomFilename(idxToRoom, newestDir);
		assert(status == 0);
		
		//initializes the room array
		InitializeRooms(idxToRoom, roomlist, roomCt);
		
		//read all the room files and store the room information to room array
		status = StoreRoomInfo(idxToRoom, newestDir, roomlist, roomCt);
		assert(status == 0);
		
		return 0;
	}else{
		return 1;
	}

}

int GetRoomIdx(char* target, char idxToRoom[][9], int roomCt){
	
	int i;
	for(i = 0; i < roomCt; i++){
		if(strcmp(target, idxToRoom[i]) == 0)
			return i;
	}
	
	return -1;
}

int* AddToStepArr(int* stepArr, int stepCt, int userInIdx){
	//create a new array
	int* oldArr = stepArr;
	stepArr = (int*) malloc(sizeof(int) * stepCt);
	assert(stepArr != 0);
	
	//loop over the oldArr and copy all the values to new	
	if(oldArr){
	    int i;
	    for(i = 0; i < stepCt-1; i++){
		    stepArr[i] = oldArr[i];
	    }
		
		//free oldArr
	    free(oldArr);	
	}
	
	stepArr[stepCt-1] = userInIdx;
	
	//return new arry address
	return stepArr;
}

/*This function is referenced from the lecture.
This function obtains the newest directory in the game folder.*/
int GetNewestRoomDirectory(char* newestDir, int charLen){
	
	int status = 0;
	int newestDirTime = -1;
	char targetDirPrefix[32] = "yeeda.rooms.";
	
	DIR* dirToCheck = opendir("."); //open up the directory this program was run in
	struct dirent *fileInDir; //Holds the current subdir of the starting dir
	struct stat dirAttributes; //Holds information we've gained about subdir
	
	if(dirToCheck > 0){  //if the current directory can be opened
		while((fileInDir = readdir(dirToCheck)) != NULL){ //if there is still file/dir in the directory
				
			if(strstr(fileInDir->d_name, targetDirPrefix) != NULL){  //if the file/dir has prefix
				stat(fileInDir->d_name, &dirAttributes);  //get the stats of the file/dir
				
				if((int)dirAttributes.st_mtime > newestDirTime){
					newestDirTime = (int)dirAttributes.st_mtime;
					memset(newestDir, '\0', charLen*sizeof(char));
					strcpy(newestDir, fileInDir->d_name);
				}
			}
		}
		
	}else{
		printf("Could Not Open Directory!!\n");
		status = 1;
	}
	
	closedir(dirToCheck);
	return status;
}

//This function gathers all the room names, assuming the room directory has only seven rooms.
int GetRoomFilename(char roomNameList[][9], char* roomDir){
	
	int status = 0;
	char targetDirPrefix[32] = "_room";
	
	DIR* dirToCheck = opendir(roomDir); //open up the directory this program was run in
	struct dirent *fileInDir; //Holds the current subdir of the starting dir
	
	int count = 0;
	if(dirToCheck > 0){
		while((fileInDir = readdir(dirToCheck)) != NULL && count < 7){
			char* p = strstr(fileInDir->d_name, targetDirPrefix);   //p is a pointer pointing to the first char of matched string.
			if(p){
				
				*p = '\0';
				memset(roomNameList[count], '\0', 9);
				strcpy(roomNameList[count], fileInDir->d_name);
				count++;
			}
		}
	}
	
	closedir(dirToCheck);
	
	if(count < 7)
		status = 1;
	return status;
}

//This function reads each room files, extracts the information of each room and stores to a struct room array.
int StoreRoomInfo(char roomNameList[][9], char* roomDir, struct Room* roomlist, int roomCt){

	int i;
	for(i = 0; i < roomCt; i++){
		
		//create file name
		char filename[256];
		memset(filename, '\0', sizeof(filename));
		sprintf(filename, "./%s/%s_room", roomDir, roomNameList[i]);
		
		//open the file for read
		FILE *thisfile = fopen(filename, "r");
		char *thisLine = NULL;
		size_t buffersize = 0;
		int lineCt = 0;
		ssize_t lineSize;
		if(thisfile){
			//get the first line
			lineSize = getline(&thisLine, &buffersize, thisfile);
			
			//loop through all the lines in the file
			while(lineSize >= 0){
				//remove the newline from thisLine
				thisLine[lineSize-1] = '\0';				
				lineCt++;
				
				//use sscanf to separate line into three parts
				char firstPart[15], secondPart[15], thirdPart[15];
				memset(firstPart, '\0', 15);
				memset(secondPart, '\0', 15);
				memset(thirdPart, '\0', 15);
				sscanf(thisLine, "%s %s %s", firstPart, secondPart, thirdPart);
				
				//printf("P1: %s\nP2: %s\nP3: %s\n", firstPart,secondPart,thirdPart);
				
				//use the line parts to build the data
				assert(AddRoomInfo(firstPart, secondPart, thirdPart, roomNameList, &roomlist[i], roomCt) != 1);
				
				//move on to the next line
				lineSize = getline(&thisLine, &buffersize, thisfile);
			}
			
			//free the getline memory
			free(thisLine);
			thisLine = NULL;
		
			//close the file
			fclose(thisfile);
			
		}else{
			//error occurs
			return 1;
		}	
	}
	return 0;
	
}

//This function writes the current line data to the array of rooms
int AddRoomInfo(char* firstpart, char* secondpart, char* thirdpart, char roomNameList[][9], struct Room* thisRoom, int roomCt){
	
	if(strcmp(firstpart, "CONNECTION") == 0){
		//This is a connection line
		thisRoom->connectedRoomIdx[thisRoom->connectionCt] = GetRoomIdx(thirdpart, roomNameList, roomCt);
		thisRoom->connectionCt++;
		
	}else{
		if(strcmp(secondpart, "TYPE:") == 0){
			//This is the type line
			if(strcmp(thirdpart, "START_ROOM") == 0)
				thisRoom->roomType = START_ROOM;
			else if(strcmp(thirdpart, "END_ROOM") == 0)
				thisRoom->roomType = END_ROOM;
			
		}else if(strcmp(secondpart, "NAME:") == 0){
			//This is the name line
			strcpy(thisRoom->roomName, thirdpart);
			
		}else{
			//something is wrong about the file, return 
			return 1;
		}
		
	}
	
	return 0;
}

//This function checks to see if the user's input location is reachable from the current location
int IsReachable(int userIn, int curIdx, struct Room* roomlist){
	
	int i;
	for(i = 0; i < roomlist[curIdx].connectionCt; i++){
		if(roomlist[curIdx].connectedRoomIdx[i] == userIn){
			return 1;
		}
	}
	
	return 0;
}

//This function frees the dynamic memory
void FreeMemory(struct Room* roomlist, int roomCt, int* stepArr){
	
	//free the dynamic arrays in roomlist
	int i;
	for(i = 0; i < roomCt; i++){
		if(roomlist[i].roomName != NULL)
			free(roomlist[i].roomName);		
		if(roomlist[i].connectedRoomIdx != NULL)
		    free(roomlist[i].connectedRoomIdx);
	}
	
	//free the steps array
	if(stepArr != NULL)
		free(stepArr);
	
}


//This function extracts the time, prints on screen and saves to a file using a different thread.
void* GetCurrentTime(){
	
	//the thread first tries to lock the lock
	pthread_mutex_lock(&myLock);
	
	//cancel the thread if cancel is received
	pthread_testcancel();
	
	//if it can lock, extract the current time, save to a file and print on screen
	//referenced from linux.die.net
	char timeStr[200];
	memset(timeStr, '\0', sizeof(timeStr));
	time_t t = time(NULL);
	struct tm *tmp = localtime(&t);
	assert(tmp != 0);
	
	//structure the time string
	if(strftime(timeStr, sizeof(timeStr), "%l:%M%P, %A, %B %d, %Y", tmp) == 0){
		printf("error writing time!\n\n");
	}
	else{
		//print the time in the specified format and save to file
		FILE *fileOut = fopen("currentTime.txt", "w");
		assert(fileOut != 0);
		
		fprintf(fileOut, timeStr);
		fclose(fileOut);
	}

	//this thread is done so unlock the lock
	pthread_mutex_unlock(&myLock);
	
	return NULL;
	
}


//This function used by thread 2 to read the time file and displays the time on screen.
void ReadTime(){
	//read the file and print the time on screen
	FILE *timeIn = fopen("currentTime.txt", "r");
	assert(timeIn != 0);
	char *thisLine = NULL;
	size_t buffersize = 0;
	ssize_t lineSize;
	//get the first line
	lineSize = getline(&thisLine, &buffersize, timeIn);
				
	//print the time on screen
	printf("%s\n\n", thisLine);
				
	//free the getline memory
	free(thisLine);
	thisLine = NULL;
		
	//close the file
	fclose(timeIn);
}

int main() {
	
	//lock the myLock
	pthread_mutex_lock(&myLock);
	
	//create another thread
	pthread_t thread2;
	int createThreadErr = pthread_create(&thread2, NULL, GetCurrentTime, NULL);
	assert(createThreadErr == 0);
	
     //global variable to keep step count
	 int stepCt = 0;
	 
	 //global variable to store user steps indices
	 int* stepIdx = NULL;
	 
	 //global variable to store array idx->roomName, there are 7 rooms.
	 char idxToRoom[7][9];
	 
	 //global array variable to store seven room structure
	 struct Room roomlist[7];
	
	 //Read the files from the directory and stores to the Room array
	 if(!ReadRoomDir(idxToRoom, roomlist, 7)){
	 
		//Game loop
		int endGame = 0;
		char curLocation[9];
		memset(curLocation, '\0', 9);
		GetStartRoom(curLocation, roomlist, 7);
	 
		while(!endGame){
		 
			printf("CURRENT LOCATION: %s\n", curLocation);
			int curIdx = GetRoomIdx(curLocation, idxToRoom, 7);
			printf("POSSIBLE CONNECTIONS: %s", idxToRoom[roomlist[curIdx].connectedRoomIdx[0]]);
			int i;
			for(i = 1; i < roomlist[curIdx].connectionCt; i++){
			    printf(", %s", idxToRoom[roomlist[curIdx].connectedRoomIdx[i]]);
			}
			printf(".\nWHERE TO? >");
			
			//get user input
			char* userIn = NULL;
			size_t buffersize = 0;
			int inCharCt = getline(&userIn, &buffersize, stdin);
			printf("\n");
			
			//remove the newline from userIn
			userIn[inCharCt-1] = '\0';
			
			//react according to user input
			
			//if the input is "time", print the time
			if(strcmp(userIn, "time") == 0){
				
				//unlock the lock
				pthread_mutex_unlock(&myLock);
				
				//wait for thread 2
				pthread_join(thread2, NULL);
				
				//lock the lock
				pthread_mutex_lock(&myLock);
				
				//read the file and print the time on screen
				ReadTime();
				
				//create thread2 again
				createThreadErr = pthread_create(&thread2, NULL, GetCurrentTime, NULL);
				assert(createThreadErr == 0);				
				
			}
			else{  //if not, then check if user inputs a valid room
			    int userInIdx = GetRoomIdx(userIn, idxToRoom, 7);
			    if(IsReachable(userInIdx, curIdx, roomlist)){
				    memset(curLocation, '\0',9);
				    strcpy(curLocation, userIn);
				    stepCt++;
				
				    //Add idx to step array
				    stepIdx = AddToStepArr(stepIdx, stepCt, userInIdx);
				
				    //check to see if it is the last room
				    if(roomlist[userInIdx].roomType == END_ROOM)
					    endGame = 1;
				
			    }else{
				    printf("HUH? I DON\'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
			    }
			}
			//free getline memory allocation
			free(userIn);
			
			//reset userIn
			userIn = NULL;
		}
		
		//display ending messages
		printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
		printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", stepCt);		
		int i;
		for(i = 0; i < stepCt; i++){
			printf("%s\n", idxToRoom[stepIdx[i]]);
		}
		
		//Cancel thread2
		pthread_cancel(thread2);
		//Unlock
		pthread_mutex_unlock(&myLock);
		//join thread2
		pthread_join(thread2, NULL);
		
	 }else{
		 printf("Read Directory error! Abort Game!");
		 return 1;	
	 }

	 
	 //free the memory
	 FreeMemory(roomlist, 7, stepIdx);
	 
     return 0;
}