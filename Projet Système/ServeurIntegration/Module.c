#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

pthread_mutex_t mtestServer;
pthread_mutex_t mSyncCp;

typedef enum{
    PROD, BACK, TWICE
}SERVEUR;

typedef struct{
	char* choixAction;
	char* error;
}INFOLOGS;


typedef struct{
	int nbDifferentFile;
}STATSSYNCROLIST;

typedef struct{
	int nbFileCopied;
	int nbFileRemoved;
}STATSCOPYLIST;

typedef struct{
	int nbIterations;
}STATSLOGS;

typedef struct{
	int nbIterations;
	int nbOpenBackServer;
	int nbOpenProdServer;
}STATSTESTSERVER;

typedef struct{
	STATSLOGS logs;
	STATSTESTSERVER testserver;
	STATSCOPYLIST copyList;
	STATSSYNCROLIST syncroList;
}STATS;

void* test_server();
int isServersOn();
void* synchro_list();
void* copy_list();
void* fileCpRm(void* arg);

void* logs(void* infoLog);
void showLogs();
void recupStats();
void putStats();
void showStats();

STATS globalStats;

int main(){
	pthread_t testServer;
	pthread_t synchroList;
	pthread_t copyList;
	SERVEUR testServerE = TWICE;
	int exit = 6;
	recupStats();
	
	
	pthread_mutex_lock(&mtestServer);
	pthread_mutex_lock(&mSyncCp);
	
	pthread_create(&testServer, NULL, test_server, (void*)&testServerE);
	
	do{
		printf("\nQue voulez-vous faire?\n\
1) Tester si les serveurs sont allumé\n\
2) Synchroniser la liste de fichiers\n\
3) Copier les différents fichiers\n\
4) Afficher le fichier des Stats\n\
5) Afficher le fichier des Logs\n\
0) Quitter l'application\n\
Choix : ");
		scanf(" %d", &exit);
		
		if(exit == 1){
			if(isServersOn()){
				printf("\nLes deux serveurs sont allumés !\n\n");
			} else {
				printf("\nAu moins un des deux serveurs n'est pas allumé...\n\n");
			}
		} else if(exit == 2) {
			isServersOn();
			pthread_mutex_unlock(&mSyncCp);
			pthread_create(&synchroList, NULL, synchro_list, NULL);
		} else if(exit == 3) {
			isServersOn();
			pthread_mutex_unlock(&mSyncCp);
			pthread_create(&synchroList, NULL, synchro_list, NULL);
			pthread_create(&copyList, NULL, copy_list, NULL);
		} else if(exit == 4) {
			showStats();
		} else if(exit == 5) {
			showLogs();
		}
		
		
		
	}while(exit);
	
	return 0;
}

void* test_server(void* s){
	
	SERVEUR serv = *(SERVEUR*)s;
	
	
	pthread_mutex_unlock(&mtestServer);
	while(1){
		pthread_mutex_lock(&mtestServer);
		int prodOpen, backOpen;
		if(serv == PROD || serv == TWICE){
			FILE* fProd = NULL;
			fProd = fopen("../ServeurProduction/isOn.txt", "r");
			fscanf(fProd, " %d", &prodOpen);
			fclose(fProd);
		}
		if(serv == BACK || serv == TWICE){
			FILE* fBack = NULL;
			fBack = fopen("../ServeurBackUp/isOn.txt", "r");
			fscanf(fBack, " %d", &backOpen);
			fclose(fBack);
		}
		FILE* fSState = NULL;
		fSState = fopen("./ServerState.txt", "w+");
		fprintf(fSState, "%d\n%d", prodOpen, backOpen);
		fclose(fSState);
		pthread_mutex_unlock(&mtestServer);
		sleep(2);
	}
	return NULL;
}

int isServersOn(){
	char on = 0;
	
	int prodOpen, backOpen;
	INFOLOGS ILogs;
	ILogs.choixAction = malloc(sizeof(char)*15);
	ILogs.error = malloc(sizeof(char)*10);
	
	pthread_mutex_lock(&mtestServer);
	FILE* fSState = NULL;
	fSState = fopen("./ServerState.txt", "r");
	fscanf(fSState, " %d\n%d", &prodOpen, &backOpen);
	fclose(fSState);
	
	ILogs.choixAction="test_server";
	ILogs.error = "no error";	
	logs((void*)&ILogs);
	if(prodOpen == 1)
		globalStats.testserver.nbOpenProdServer ++;
	if(backOpen == 1)
		globalStats.testserver.nbOpenBackServer ++;
	globalStats.testserver.nbIterations ++;
	putStats();
	
	pthread_mutex_unlock(&mtestServer);
	
	if(prodOpen && backOpen){
		on = 1;
	}
	
	
	return on;
}

void* synchro_list(){

	pthread_mutex_lock(&mSyncCp);
    char commandeDiff[1000] = "diff -qr ../ServeurBackUp/ ../ServeurProduction | sed -e 's/.*..\\/ServeurBackUp\\/: /SUPPR/g' -e 's/Only in ..\\/ServeurProduction\\///g' -e 's/Only in ..\\/ServeurProduction://g' -e 's/Only in ..\\/ServeurBackUp\\//SUPPR/' -e 's/: /\\//' -e 's/ //' | cut -d' ' -f3 >";
    char nomFichierDiff[] = "fichierDiff.txt";
    //commandeDiff+nomFichierDiff
    strcat(commandeDiff,nomFichierDiff);
    //printf("Commande qui va etre executer : %s\n", commandeDiff);
    system(commandeDiff);

    char commandeSed2[1000] = "sed 's/..\\/ServeurProduction\\///' ";
    char redirDif[] = "> listeDifference.txt";
    //commandeDiff+nomFichierDiff
    strcat(commandeSed2,nomFichierDiff);

    strcat(commandeSed2,redirDif);
    system(commandeSed2);
    
    char* tmp = malloc(sizeof(char)*10);
    FILE* fichier;
    fichier = fopen("listeDifference.txt","r");
    while (fgets(tmp, 10, fichier) != NULL)
    {
        globalStats.syncroList.nbDifferentFile ++;
    }
    free(tmp);
    putStats();
    INFOLOGS ILogs;
	ILogs.choixAction = malloc(sizeof(char)*15);
	ILogs.error = malloc(sizeof(char)*10);
	ILogs.choixAction[0] = '\0';
	ILogs.error[0] = '\0';
	strcat(ILogs.choixAction,"copy_list");
	strcat(ILogs.error,"non error");
	logs((void*)&ILogs);
	free(ILogs.choixAction);
	free(ILogs.error);
    
    
	pthread_mutex_unlock(&mSyncCp);
	return NULL;
}

void* copy_list(){
	
	
	pthread_mutex_lock(&mSyncCp);
	int nombre_fichiers = 0;
    FILE *fichier = NULL;
    fichier = fopen("listeDifference.txt", "r");

    if (fichier == NULL)
    {
        exit(0);
    }

    char a_mettre_a_jour[100][1000];

    int i = 0;
    while (fgets(a_mettre_a_jour[i], 1000, fichier) != NULL)
    {
		a_mettre_a_jour[i][strlen(a_mettre_a_jour[i]) - 1] = '\0';
        i++;
    }
	nombre_fichiers = i;
	
	pthread_t* cpFile = malloc(sizeof(pthread_t) * nombre_fichiers);
	
    // pour chaque fichier à mettre à jour, on copie la nouvelle version du serveur de production et on la colle dans le serveur backup
    for (int i = 0; i < nombre_fichiers; i++)
    {
		pthread_create(&cpFile[i], NULL, fileCpRm, (void*)a_mettre_a_jour[i]);
    }
    
     for (int i = 0; i < nombre_fichiers; i++)
    {
		pthread_join(cpFile[i], NULL);
    }
    putStats();
    INFOLOGS ILogs;
	ILogs.choixAction = malloc(sizeof(char)*15);
	ILogs.error = malloc(sizeof(char)*10);
	ILogs.choixAction[0] = '\0';
	ILogs.error[0] = '\0';
	strcat(ILogs.choixAction,"copy_list");
	strcat(ILogs.error,"non error");
	logs((void*)&ILogs);
	free(ILogs.choixAction);
	free(ILogs.error);
	
	free(cpFile);
    fclose(fichier);
	pthread_mutex_unlock(&mSyncCp);
	return NULL;
}

void* fileCpRm(void* arg){
	if(!strcmp(arg, "")){
		return NULL;
	}
	char* file = (char*)arg;
	
	char src[200] = {0};
	char dest[200] = {0};
	//char message[100] = {0};
	strcat(src, "../ServeurProduction/");
	strcat(dest, "../ServeurBackUp/");
	
	if(strstr(file, "SUPPR") == NULL){
		char cmd[1000] = {0};
		strcat(src, file);
		strcat(dest, file);
		
		//~ printf("src : %s\n", src);
		//~ printf("dest : %s\n", dest);
		
		strcat(cmd, "cp -af ");
		strcat(cmd, src);
		strcat(cmd, " ");
		strcat(cmd, dest);
		
		//~ printf("cmd : %s\n", cmd);
		system(cmd);
		globalStats.copyList.nbFileCopied ++;
	} else {
		char cmd[1000] = {0};
		
		char* cursor = file + 5;
		strcat(src, cursor);
		strcat(dest, cursor);
		
		//~ printf("src : %s\n", src);
		//~ printf("dest : %s\n", dest);
		
		strcat(cmd, "rm -f ");
		strcat(cmd, src);
		strcat(cmd, " ");
		strcat(cmd, dest);
		
		//~ printf("cmd : %s\n", cmd);
		system(cmd);
		globalStats.copyList.nbFileRemoved ++;
	}	
	return NULL;	
}

void recupStats(){
	FILE* fichier;
	fichier = fopen("log/stats.txt","r");
	
	
	//TEST_SERVER
	while(fgetc(fichier) != '='){	
	}
	fscanf(fichier,"%d",&globalStats.testserver.nbIterations);
	while(fgetc(fichier) != '='){	
	}
	fscanf(fichier,"%d",&globalStats.testserver.nbOpenBackServer);
	while(fgetc(fichier) != '='){	
	}
	fscanf(fichier,"%d",&globalStats.testserver.nbOpenProdServer);
	//LOGS
	while(fgetc(fichier) != '\n'){	
	}
	while(fgetc(fichier) != '='){	
	}
	fscanf(fichier,"%d",&globalStats.logs.nbIterations);
	//COPYLIST
	while(fgetc(fichier) != '\n'){	
	}
	while(fgetc(fichier) != '='){	
	}
	fscanf(fichier,"%d",&globalStats.copyList.nbFileCopied);
	while(fgetc(fichier) != '='){	
	}
	fscanf(fichier,"%d",&globalStats.copyList.nbFileRemoved);
	//SYNCROLIST
	while(fgetc(fichier) != '\n'){	
	}
	while(fgetc(fichier) != '='){	
	}
	fscanf(fichier,"%d",&globalStats.syncroList.nbDifferentFile);
	
	fclose(fichier);
	
}
void putStats(){
	
	FILE* fichier;
	fichier = fopen("log/stats.txt","r+");
	char* buffer = malloc(sizeof(char)*10);
	//TEST_SERVER
	fseek(fichier, 0 , SEEK_SET);
	while(fgetc(fichier) != '='){	
	}
	sprintf(buffer," %d",globalStats.testserver.nbIterations);
	fputs(buffer,fichier);
	while(fgetc(fichier) != '='){	
	}
	sprintf(buffer," %d",globalStats.testserver.nbOpenBackServer);
	fputs(buffer,fichier);
	
	while(fgetc(fichier) != '='){	
	}
	sprintf(buffer," %d",globalStats.testserver.nbOpenProdServer);
	fputs(buffer,fichier);
	//LOGS
	fseek(fichier, 0 , SEEK_SET);
	while(fgetc(fichier) != '\n'){	
	}
	while(fgetc(fichier) != '='){	
	}
	sprintf(buffer," %d",globalStats.logs.nbIterations);
	fputs(buffer,fichier);
	fseek(fichier, 0 , SEEK_SET);
	while(fgetc(fichier) != '\n'){	
	}
	while(fgetc(fichier) != '\n'){	
	}
	while(fgetc(fichier) != '='){	
	}
	sprintf(buffer," %d",globalStats.copyList.nbFileCopied);
	fputs(buffer,fichier);
	while(fgetc(fichier) != '='){	
	}
	sprintf(buffer," %d",globalStats.copyList.nbFileRemoved);
	fputs(buffer,fichier);
	
	
	fseek(fichier, 0 , SEEK_SET);
	while(fgetc(fichier) != '\n'){	
	}
	while(fgetc(fichier) != '\n'){	
	}
	while(fgetc(fichier) != '\n'){	
	}
	while(fgetc(fichier) != '='){	
	}
	sprintf(buffer," %d",globalStats.syncroList.nbDifferentFile);
	fputs(buffer,fichier);
	
	free(buffer);
	fclose(fichier);
	
}

void* logs(void* infoLog)
{
	INFOLOGS iLogs = *(INFOLOGS*)infoLog;
	
	FILE* fichier = NULL;
	char* string = malloc(sizeof(char)*128);
	char* buffer = malloc(sizeof(char)*10);
	
	time_t now = time( NULL );
	int h,min;
	struct tm *local = localtime(&now);
	h = local->tm_hour;      
	min = local->tm_min;
	string[0] = '\0';
	
	if(h<10)
		sprintf(buffer,"0%d",h);
	else
		sprintf(buffer,"%d",h);
	strcat(string, buffer);
	strcat(string, "h");
	if(min<10)
		sprintf(buffer,"0%d",min);
	else
		sprintf(buffer,"%d",min);
	strcat(string, buffer);
	strcat(string," : ");
	strcat(string, iLogs.choixAction);
	strcat(string," : ");
	strcat(string, iLogs.error);

	fichier = fopen("log/logs.txt","a");
		
	if(fichier != NULL){
		
		fputs(string,fichier);
		fputs("\n",fichier);	
		
	}
	fclose(fichier);
	free(buffer);
	free(string);
	globalStats.logs.nbIterations ++;
	
	//Pour mettre son action dans les logs
	//~ INFOLOGS ILogs;
	//~ ILogs.choixAction = malloc(sizeof(char)*15);
	//~ ILogs.error = malloc(sizeof(char)*10);
	//~ ILogs.choixAction = "nom_fct";
	//~ ILogs.error = "ouiOUnon";
	//~ logs((void*)&ILogs);
	
	
	return NULL;
}

void showLogs(){
	
	FILE* fichier = NULL;
	fichier = fopen("log/logs.txt","r+");
	char tab[1000][128];
	int i = 0;
	
	while(fgets(tab[i],128,fichier)){
		printf("%s",tab[i]);
	}
}

void showStats(){
	
	FILE* fichier = NULL;
	fichier = fopen("log/stats.txt","r+");
	char tab[2][128];
	int i = 0;
	
	while(fgets(tab[i],128,fichier)){
		printf("%s",tab[i]);
	}
}


