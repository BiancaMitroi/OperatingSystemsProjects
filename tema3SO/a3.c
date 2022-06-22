// includerea bibliotecilor necesare pentru rularea programului
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

// definirea macro-ului pentru denumirea pipe-ului prin care programul transmite informatie tester-ului
#define WRITING_PIPE "RESP_PIPE_25990"

// definirea macro-ului pentru denumirea pipe-ului prin care programul primeste informatie de la tester
#define READING_PIPE "REQ_PIPE_25990"

// definirea macro-ului pentru denumirea regiunii partajate 
#define FILE_NAME "yxU0LX"

// definirea macro-ului pentru permisiunile cu care se creeaza regiunea de memorie partajata
#define PERMISSIONS 0664

// definirea macro-ului pentru varianta
#define VARIANT 25990

// declararea descriptorilor de fisier pentru deschiderea pipe-urilor in citire, respectiv scriere, pentru deschiderea regiunii de memorie partajata, pentru dechiderea fisierului mapat in memorie, declararea dimensiunii regiunii partajate
int rfd = -1, wfd = -1, shmFd = -1;
unsigned int mappedFileD = 0, shmDim = 0;

// declararea dimensiunii fisierului mapat in memorie
unsigned int mappedFileDim;

// declararea adresei de la care se face partajarea memoriei
volatile char* sharedMemAddr;

// decalararea adresei de la care se face maparea fisierului cu descriptorul mappedFileD in memoie
char* mapAddr;

// implementarea functiei care converteste 4 valori pe un octet la o valoare pe 4 octeti
unsigned int castCharToUnsignedInt(char a0, char a1, char a2, char a3){
	return ((a0 & 0xFF) << 0) | ((a1 & 0xFF) << 8) | ((a2 & 0xFF) << 16) | ((a3 & 0xFF) << 24);
}

// implementarea functiei care transmite tester-ului mesajul de eroare
void sendError(){
	unsigned int dim = 5;
	write(wfd, &dim, 1); // string field
	write(wfd, "ERROR", 5);
}

// implementarea functiei care transmite tester-ului mesajul de succes
void sendSuccess(){
	unsigned int dim = 7;
	write(wfd, &dim, 1); // string field
	write(wfd, "SUCCESS", 7);
}

// implementarea functiei care conecteaza programul cu tester-ul 
void connectToTester(){
	//crearea pipe-ului de scriere
	if(mkfifo(WRITING_PIPE, PERMISSIONS) != 0){
		perror("Eroare la crearea pipe-ului pentru scriere\n");
		exit(1);
	}
	// deschiderea pipe-urilor
	rfd = open(READING_PIPE, O_RDONLY);
	wfd = open(WRITING_PIPE, O_WRONLY);
	if(rfd == -1){
		perror("Eroare la deschiderea pipe-ului pentru citire\n");
		exit(1);
	}
	if(wfd == -1){
		perror("Eroare la deschiderea pipe-ului pentru scriere\n");
		exit(1);
	}
	//transmiterea mesajului de conectare
	unsigned int dim = 7;
	write(wfd, &dim, 1); // string field
	write(wfd, "CONNECT", 7);

}
void ping(){
	unsigned int pingPongDim = 4;
	write(wfd, &pingPongDim, 1); // string field
	write(wfd, "PING", 4);
	write(wfd, &pingPongDim, 1); // string field
	write(wfd, "PONG", 4);
	unsigned int variantNumber = VARIANT;
	write(wfd, &variantNumber, sizeof(unsigned int));
}

void createShm(){
	// transmiterea mesajului pentru alocarea regiunii partajate
	unsigned int dim = 10;
	char ok = 1;
	write(wfd, &dim, 1); // string field
	write(wfd, "CREATE_SHM", 10);

	// deschiderea regiunii
	shmFd = shm_open(FILE_NAME, O_CREAT | O_RDWR, PERMISSIONS);
	if(shmFd < 0)
		ok = 0;

	// citirea dimensiunii regiunii si modificarea dimensiunii alocate la deschidere
	read(rfd, &shmDim, sizeof(unsigned int));
	ftruncate(shmFd, shmDim);

	// maparea regiunii partajate
	sharedMemAddr = (volatile char*) mmap(NULL, shmDim, PROT_WRITE | PROT_READ, MAP_SHARED, shmFd, 0);
	if(sharedMemAddr == (void*)-1)
		ok = 0;

	if(!ok)
		sendError();
	else
		sendSuccess();
}

void writeToShm(){
	// declararea si citirea din pipe a adresei si a valorii care se vrea scrisa in mapare
	unsigned int offset = 0, value = 0;
	char ok = 1;
	read(rfd, &offset, sizeof(unsigned int));
	read(rfd, &value, sizeof(unsigned int));

	// transmiterea mesajului de scriere in memoria partajata
	unsigned int dim = 12;
	write(wfd, &dim, 1); // string field
	write(wfd, "WRITE_TO_SHM", 12);

	if(offset < 0 || offset + 3 > shmDim)
		ok = 0;

	if(!ok)
		sendError();
	else{
		// scrierea valorii pe octetii din mapare corespunzatori adresei citite din pipe
		sharedMemAddr[offset + 3] = (char)(value >> 24);
		sharedMemAddr[offset + 2] = (char)((value & 0x00FF0000) >> 16);
		sharedMemAddr[offset + 1] = (char)((value & 0x0000FF00) >> 8);
		sharedMemAddr[offset + 0] = (char)(value & 0x000000FF);
		sendSuccess();
	}
}

void mapFile(){
	// fisierul care se vrea mapat se transmite prin pipe
	// se transmite mesajul de mapare a fisierului
	char ok = 1;
	unsigned int dim = 8;
	write(wfd, &dim, 1); // string field
	write(wfd, "MAP_FILE", 8);

	read(rfd, &dim, 1);
	char* fileName = (char*)malloc((dim + 1) * sizeof(char));
	read(rfd, fileName, dim);
	fileName[dim] = '\0';

	mappedFileD = open(fileName, O_RDONLY);
	if(mappedFileD == -1)
		ok = 0;

	// se extrage dimensiunea fisierului de mapat
	mappedFileDim = lseek(mappedFileD, 0, SEEK_END);
	lseek(mappedFileD, 0, SEEK_SET);

	mapAddr = (char*)mmap(NULL, mappedFileDim, PROT_WRITE | PROT_READ, MAP_PRIVATE, mappedFileD, 0);
	if(mapAddr == (void*)-1)
		ok = 0;

	if(!ok)
		sendError();
	else
		sendSuccess();
	free(fileName);
}

void readFromFileOffset(){
	// se extrag din pipe offset-ul si nr de octeti care trebuiesc transmisi din fisierul mapat in memoria partajata
	unsigned int offset = 0, byteNr = 0;
	read(rfd, &offset, sizeof(unsigned int));
	read(rfd, &byteNr, sizeof(unsigned int));

	char ok = 1;
	if(offset + byteNr > mappedFileDim)
		ok = 0;

	// transmitere de mesaj catre tester
	unsigned int dim = 21;
	write(wfd, &dim, 1); // string field
	write(wfd, "READ_FROM_FILE_OFFSET", 21);

	if(!ok)
		sendError();
	else{
        for(int i = 0; i < byteNr; i++)
            sharedMemAddr[i] = mapAddr[i + offset];
		sendSuccess();
	}
}

void readFromFileSection(){
	// se citesc din pipe numarul sectiunii, offset-ul relativ la sectiune si numarul de octeti pentru care se citeste din fisierul mapat in memoria partajata
	unsigned int sectionNr = 0, offset = 0, byteNr = 0;
	read(rfd, &sectionNr, sizeof(unsigned int));
	read(rfd, &offset, sizeof(unsigned int));
	read(rfd, &byteNr, sizeof(unsigned int));

	// transmitere de mesaj
	unsigned int dim = 22;
	write(wfd, &dim, 1); // string field
	write(wfd, "READ_FROM_FILE_SECTION", 22);

	char ok = 1;

	//extragerea offset-ului relativ la inceputul fisierului (pseudo-cursor)
	unsigned int absoluteOffset = mappedFileDim - 1; // cursorul se afla la sfarsitul fisierului // \/
	printf("The final byte is at address: %X\nThe magic value is :%c%c%c%c\n", absoluteOffset, mapAddr[absoluteOffset - 3], mapAddr[absoluteOffset - 2], mapAddr[absoluteOffset - 1], mapAddr[absoluteOffset]); // 2631 hexazecimal
	if(mapAddr[absoluteOffset] != 'Q' || mapAddr[absoluteOffset - 1] != 'W' || mapAddr[absoluteOffset - 2] != 'G' || mapAddr[absoluteOffset - 3] != 'q'){ // validarea MAGIC-ului \/
		printf("Eroarea s-a trimis pe ramura validarii magic ului\n");
		ok = 0;
	}
	printf("Header size is: %d\n", castCharToUnsignedInt(mapAddr[absoluteOffset - 5], mapAddr[absoluteOffset - 4], 0, 0));
	absoluteOffset = mappedFileDim - castCharToUnsignedInt(mapAddr[absoluteOffset - 5], mapAddr[absoluteOffset - 4], 0, 0); // cursorul s-a mutat la inceputul header-ului \/
	printf("The beginning of the header is at addres: %x\n", absoluteOffset);
	if((mapAddr[absoluteOffset] & 0xff) < 45 || (mapAddr[absoluteOffset] & 0xff) > 145){ // validarea versiunii \/
		printf("Eroarea s-a trimis pe ramura validarii versiunii\n");
		ok = 0;
	}
	printf("The version is: %x\n", (mapAddr[absoluteOffset] & 0xff));
	absoluteOffset++;
	printf("The number of sections is: %x\n", mapAddr[absoluteOffset]);
	if(mapAddr[absoluteOffset] < 2 || mapAddr[absoluteOffset] > 15){ // validarea numarului de sectiuni \/
		printf("Eroarea s-a trimis pe ramura validarii numarului de sectiuni\n");
		ok = 0;
	}
	if(sectionNr > mapAddr[absoluteOffset]){ // validarea numarului sectiunii transmise prin pipe
		printf("Eroarea s-a trimis pe ramura validarii numarului sectiunii trimis prin pipe\n");
		sendError();
		return;
	}
	absoluteOffset++;
	while(sectionNr != 0){ // se cauta adresa la care se tine offset-ul sectiunii transmise prin pipe
		absoluteOffset += 20;
		sectionNr--;
	}
	absoluteOffset -= 8;
	printf("The absolute offset reaches at address: %X\n", absoluteOffset);
	//printf("%x\n", absoluteOffset);
	unsigned int sectDimOffset = absoluteOffset + 4;
	absoluteOffset = castCharToUnsignedInt(mapAddr[absoluteOffset], mapAddr[absoluteOffset + 1], mapAddr[absoluteOffset + 2], mapAddr[absoluteOffset + 3]) + offset;
	if(byteNr + offset > castCharToUnsignedInt(mapAddr[sectDimOffset], mapAddr[sectDimOffset + 1], mapAddr[sectDimOffset + 2], mapAddr[sectDimOffset + 3])){
		// transmitere de mesaj
		// unsigned int dim = 22;
		// write(wfd, &dim, 1); // string field
		// write(wfd, "READ_FROM_FILE_SECTION", 22);
		printf("Eroarea s-a trimis pe ramura validarii numarului de octeti trimis prin pipe\n");
		sendError();
		return;
	}

	// logica de citire din sectiune
	

	if(!ok){
		//printf("ERROR\n");
		sendError();
	}else{
		for(int i = 0; i < byteNr; i++)
            sharedMemAddr[i] = mapAddr[absoluteOffset + i];//, printf("%X ", mapAddr[absoluteOffset + i]);
		//printf("SUCCESS\n");
		sendSuccess();
	}
}

void readFromLogicalSpaceOffset(){
	unsigned int logicalOffset = 0, byteNr = 0;
	read(rfd, &logicalOffset, sizeof(unsigned int));
	read(rfd, &byteNr, sizeof(unsigned int));

	char ok = 1;

	unsigned int dim = 30;
	write(wfd, &dim, 1);
	write(wfd, "READ_FROM_LOGICAL_SPACE_OFFSET", 30);

	//extragerea offset-ului relativ la inceputul fisierului (pseudo-cursor)
	unsigned int absoluteOffset = mappedFileDim - 1; // cursorul se afla la sfarsitul fisierului // \/
	printf("The final byte is at address: %X\nThe magic value is :%c%c%c%c\n", absoluteOffset, mapAddr[absoluteOffset - 3], mapAddr[absoluteOffset - 2], mapAddr[absoluteOffset - 1], mapAddr[absoluteOffset]); // 2631 hexazecimal
	if(mapAddr[absoluteOffset] != 'Q' || mapAddr[absoluteOffset - 1] != 'W' || mapAddr[absoluteOffset - 2] != 'G' || mapAddr[absoluteOffset - 3] != 'q'){ // validarea MAGIC-ului \/
		printf("Eroarea s-a trimis pe ramura validarii magic ului\n");
		ok = 0;
	}
	printf("Header size is: %d\n", castCharToUnsignedInt(mapAddr[absoluteOffset - 5], mapAddr[absoluteOffset - 4], 0, 0));
	absoluteOffset = mappedFileDim - castCharToUnsignedInt(mapAddr[absoluteOffset - 5], mapAddr[absoluteOffset - 4], 0, 0); // cursorul s-a mutat la inceputul header-ului \/
	printf("The beginning of the header is at addres: %x\n", absoluteOffset);
	if((mapAddr[absoluteOffset] & 0xff) < 45 || (mapAddr[absoluteOffset] & 0xff)  > 145){ // validarea versiunii \/
		printf("Eroarea s-a trimis pe ramura validarii versiunii\n");
		ok = 0;
	}
	printf("The version is: %x\n", (mapAddr[absoluteOffset] & 0xff));
	absoluteOffset++;
	unsigned int sectionNr = mapAddr[absoluteOffset];
	printf("The number of sections is: %x\n", mapAddr[absoluteOffset]);
	if(mapAddr[absoluteOffset] < 2 || mapAddr[absoluteOffset] > 15){ // validarea numarului de sectiuni \/
		printf("Eroarea s-a trimis pe ramura validarii numarului de sectiuni\n");
		ok = 0;
	}
	absoluteOffset++;
	unsigned int* sectionDim = (unsigned int*)calloc(sectionNr, sizeof(unsigned int));
	unsigned int* sectionAddr = (unsigned int*)calloc(sectionNr, sizeof(unsigned int));
	unsigned int* startAddr = (unsigned int*)calloc(sectionNr, sizeof(unsigned int));
	unsigned int* endAddr = (unsigned int*)calloc(sectionNr, sizeof(unsigned int));
	//unsigned int start = 0, end = 0, 
	unsigned int currentSection = 0;
	for(int i = 0; i < sectionNr; i++){ // se cauta adresa la care se tine dimensiunea sectiunii
		absoluteOffset += 12;
		sectionAddr[i] = castCharToUnsignedInt(mapAddr[absoluteOffset], mapAddr[absoluteOffset + 1], mapAddr[absoluteOffset + 2], mapAddr[absoluteOffset + 3]);
		absoluteOffset += 4;
		sectionDim[i] = castCharToUnsignedInt(mapAddr[absoluteOffset], mapAddr[absoluteOffset + 1], mapAddr[absoluteOffset + 2], mapAddr[absoluteOffset + 3]);
		absoluteOffset += 4;
		if(i == 0){
			startAddr[i] = 0;
			endAddr[i] = sectionDim[i];
		}else{
			startAddr[i] = ((endAddr[i - 1] / 4096) + 1) * 4096;
			endAddr[i] = startAddr[i] + sectionDim[i];
		}
			
		// if(endAddr[i] < logicalOffset)
		// 	currentSection = i + 1;
	}
	for(int i = 0; i < sectionNr; i++){
		if(startAddr[i] > logicalOffset){
			currentSection = i;
			break;
		}
	}
	int relativeOffset = logicalOffset - startAddr[currentSection - 1];
	if(relativeOffset + byteNr > sectionDim[currentSection - 1])
		ok = 0;
	if(!ok)
		sendError();
	else{
		for(int i = 0; i < byteNr; i++){
			sharedMemAddr[i] = mapAddr[sectionAddr[currentSection - 1] + relativeOffset + i];
		}
		sendSuccess();
	}
}

void giveUp(){
	close(rfd);
	close(wfd);
	unlink(WRITING_PIPE);
	exit(0);
}


int main(){
	connectToTester();
	while(1){
		// se citeste comanda data de tester
		unsigned int commandDim = 0;
		read(rfd, &commandDim, 1);
		char* commandString = (char*) calloc(commandDim + 1,  sizeof(char));
		read(rfd, commandString, commandDim);

		// se alege ce functie sa se execute in functie de ce comanda a dat tester-ul
		if(strcmp(commandString, "PING") == 0)
			ping();
		else if(strcmp(commandString, "CREATE_SHM") == 0)
			createShm();
		else if(strcmp(commandString, "WRITE_TO_SHM") == 0)
			writeToShm();
		else if(strcmp(commandString, "MAP_FILE") == 0)
			mapFile();
		else if(strcmp(commandString, "READ_FROM_FILE_OFFSET") == 0)
			readFromFileOffset();
		else if(strcmp(commandString, "READ_FROM_FILE_SECTION") == 0)
			readFromFileSection();
		else if(strcmp(commandString, "READ_FROM_LOGICAL_SPACE_OFFSET") == 0)
			readFromLogicalSpaceOffset();
		else if(strcmp(commandString, "EXIT") == 0)
			giveUp();
		// se face loc pentru a primi o comanda noua
		free(commandString);
	}
	return 0;
}
