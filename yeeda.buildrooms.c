/*******************************************************************************
** Author:       Darwin Yee, ONID 933915366
** Date:         10-17-2019
** Project 2:    BuildRoom
** Description:  This program creates a series of files that hold descriptions
                 of the in-game rooms and how the rooms are connected.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0
#define START_ROOM 0
#define MID_ROOM 1
#define END_ROOM 2


//Global constant variable room names and the room type
const char *nameList[10] = { "Kimon","Aldin","Rodney","Ubendu","Wischard","Hervey","Helinand","Alwyne","Heirax","Thon" };
const char *roomTypeList[3] = { "START_ROOM","MID_ROOM","END_ROOM" };

//This is the Room structure to store room related information
struct Room {
     char* roomName;
     int* connectedRoomIdx; //array of 7 slot, with 0 as no connection and 1 as connected
     int roomType;
     int roomID;
     int connectionCt;
};

//This function checks to see if the target exists in an array.  Integer only.
int CheckExists(int target, int* arr, int arrlen) {
	 int i;
     for (i = 0; i < arrlen; i++) {
          if (arr[i] == target)
               return TRUE;
     }
     return FALSE;
}

//This function randomly selects 7 rooms from the list of 10 and stores them in the finalRoomList string array.
void SelectRooms(char finalRoomList[][9], int roomCt) {

     //array to store selected room idx
     int* roomIdx = (int*)malloc(sizeof(int)*roomCt);
     assert(roomIdx != 0);

     //reset roomlist and set the roomIdx array to -1
	 int i;
     for (i = 0; i < roomCt; i++) {
          memset(finalRoomList[i], '\0', 9);
          roomIdx[i] = -1;
     }

     //keep picking a number between 0 to 9 until a sequece of 7 unique numbers is picked
     int numCt = 0;
     while (numCt < roomCt) {
          int curNum = rand() % 10;
          if (!CheckExists(curNum, roomIdx, roomCt)) {
               roomIdx[numCt] = curNum;
               numCt++;
          }
     }

     //write the room name to finalRoomList
     for (i = 0; i < roomCt; i++) {
          strcpy(finalRoomList[i], nameList[roomIdx[i]]);
     }

     //free roomIdx
     free(roomIdx);
}

//This function initializes each room in the room array.
void InitializeRooms(struct Room* roomlist, char finalRoomList[][9], int roomCt) {

     //Select 7 rooms from 10 roomlist
     SelectRooms(finalRoomList, 7);
	
	 int i;
     for (i = 0; i < roomCt; i++) {

          //setup the room name
          roomlist[i].roomName = (char*)malloc(sizeof(char) * 9);
          assert(roomlist[i].roomName != 0);
          memset(roomlist[i].roomName, '\0', 9);
          strcpy(roomlist[i].roomName, finalRoomList[i]);

          //allocate memory for the array
          roomlist[i].connectedRoomIdx = (int*)malloc(sizeof(int) * roomCt);
          assert(roomlist[i].connectedRoomIdx != 0);
		  int j;
          for (j = 0; j < roomCt; j++) {
               roomlist[i].connectedRoomIdx[j] = 0;
          }

          //set roomType and roomReady
          roomlist[i].roomType = MID_ROOM;
          roomlist[i].roomID = i;
          roomlist[i].connectionCt = 0;
     }

}

//This function sets the room type randomly, assuming all rooms are set to MID_ROOM
void SetRoomTypes(struct Room* roomlist, int roomCt) {

     //Set the START_ROOM
     roomlist[rand() % 7].roomType = START_ROOM;
     
     //Set the END_ROOM
     int curIdx;
     do {
          curIdx = rand() % 7;
          if (roomlist[curIdx].roomType == MID_ROOM) {
               roomlist[curIdx].roomType = END_ROOM;
          }
     } while (roomlist[curIdx].roomType == START_ROOM);
}

//This function generates the directory and the room files
void CreateRooms(struct Room* roomlist, char finalRoomList[][9], int roomCt) {

	//create the directory name
	char dirName[256];
	memset(dirName, '\0', sizeof(dirName));
	sprintf(dirName, "yeeda.rooms.%d", getpid());

	if(mkdir(dirName, 0755) == 0){
		int i;
		for(i = 0; i < roomCt; i++){
			//create the file name
			char curFileName[256];
			memset(curFileName, '\0', sizeof(curFileName));
			sprintf(curFileName, "./%s/%s_room", dirName, roomlist[i].roomName);
			
			//open the file
			FILE *filept = fopen(curFileName, "w");
			if(filept){
				
				//write the name of the room
				fprintf(filept, "ROOM NAME: %s\n", roomlist[i].roomName);
				
				//loop to write all the connections
				int j, k = 1;
				for(j = 0; j < roomCt; j++){
					if(roomlist[i].connectedRoomIdx[j] == 1){
					    fprintf(filept, "CONNECTION %d: %s\n", k, finalRoomList[j]);
						k++;
					}
				}
				
				//print the room type to file
				fprintf(filept, "ROOM TYPE: %s\n", roomTypeList[roomlist[i].roomType]);
				
				fclose(filept);

			}else{
				printf("Error writing to %s!", curFileName);
				return;
			}
			
		}
	}
     
}

//This function frees the dynamically allocated memory for the room structure
void FreeRoomMemory(struct Room* roomlist, int roomCt) {
     if (roomlist != 0) {
		  int i;
          for (i = 0; i < roomCt; i++) {
               free(roomlist[i].roomName);
               free(roomlist[i].connectedRoomIdx);
          }
     }
}

/*Room generating functions referenced from program outline*/

//This function returns 1 if the whole graph has been built, 0 otherwise
int IsGraphFull(struct Room* roomlist, int roomCt) {
     //if all the rooms have connections between 3 and 6, return 1
	 int i;
     for (i = 0; i < roomCt; i++) {
          if (roomlist[i].connectionCt < 3 || roomlist[i].connectionCt > 6)
               return FALSE;
     }
     return TRUE;
}

//This function returns a room pointer pointing to a randomly selected room
struct Room* GetRandomRoom(struct Room* roomlist, int roomCt) {
     return &roomlist[rand() % roomCt];
}

//This function returns 1 if a connection can be added from Room x
int CanAddConnectionFrom(struct Room* x) {
     if (x->connectionCt < 6)
          return TRUE;
     return FALSE;
}

//This function returns 1 if a connection from Room x to Room y already exists, 0 otherwise
int ConnectionAlreadyExists(struct Room* x, struct Room* y) {
     if (x->connectedRoomIdx[y->roomID] == 0)
          return FALSE;
     return TRUE;
}

//This function connects room x and y together but does not check if the connnection is valid
void ConnectRoom(struct Room* x, struct Room* y) {
     x->connectedRoomIdx[y->roomID] = 1;
     x->connectionCt++;
}

//This function returns 1 if Room x and y are the same, 0 otherwise
int IsSameRoom(struct Room* x, struct Room* y) {
     if (x->roomID == y->roomID)
          return TRUE;
     return FALSE;
}

//This function adds a random, valid outbound connection from a Room to another Room
void AddRandomConnection(struct Room* roomlist, int roomCt) {
     struct Room* A = 0;
     struct Room* B = 0;

     //Get a room that still needs new connection
     while (TRUE) {
          A = GetRandomRoom(roomlist, roomCt);
          if (CanAddConnectionFrom(A))
               break;
     }

     //Get another room that is not the same as A, has not connected to A and can accept new connection
     do {
          B = GetRandomRoom(roomlist, roomCt);
     } while (CanAddConnectionFrom(B) == FALSE || IsSameRoom(A, B) == TRUE || ConnectionAlreadyExists(A, B) == TRUE);

     //Connect them both ways
     ConnectRoom(A, B);
     ConnectRoom(B, A);

}


int main() {
     //set the random seed
     srand((unsigned)time(0));

     //select 7 rooms randomly
     char finalRoomList[7][9];
     struct Room roomlist[7];

     //initialize the rooms
     InitializeRooms(roomlist, finalRoomList, 7);

     //Generate random connections for the rooms
     while (!IsGraphFull(roomlist, 7)) {
          AddRandomConnection(roomlist, 7);
     }

     //Assign room type
     SetRoomTypes(roomlist, 7);

     //Create the directory and files
     CreateRooms(roomlist, finalRoomList, 7);

     //Clean up dynamically allocated variables
     FreeRoomMemory(roomlist, 7);

     /*TESTING AREA
     char* testing = "Hello testing!";

     printf("%s\n", testing);
	 int i;
     for (i = 0; i < 7; i++) {
          printf("RoomName: %s, RoomType: %s \n", roomlist[i].roomName, roomTypeList[roomlist[i].roomType]);
		  int j;
          for (j = 0; j < 7; j++) {
               if (roomlist[i].connectedRoomIdx[j] == 1) {
                    printf(" %s ", finalRoomList[j]);
               }
          }
          printf("\nConnection Count: %d\n", roomlist[i].connectionCt);

     }*/

     return 0;
}