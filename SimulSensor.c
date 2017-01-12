/***************************************************************
* Révision 1 : Christophe Duhil
* Ajout d'un second device DEVICENAMEB
*
****************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pthread.h>

#include "unDriver.h"

#define DEVICENAMEA "/dev/myDevice1"
#define DEVICENAMEB "/dev/myDevice2" // @ChD : ajout d'un second driver

#define initVal 1
#define pseudoPeriod 2

static int devID_A; 
static int devID_B;
static int *pWrite;

struct timeval  timeZero;	// instant de démarrage du simulateur
struct timeval  currentTime; 	// instant courant
struct timezone zone;


// procedure d'envoie des donneés 
void * sendData(void * null)
{	
	int timeWrite;
	int i = 0;

	while (1)
	{
		sleep(pseudoPeriod);		

		*pWrite += 3; 
		// ecrit l'info contenue dans pWrite vers devId de la taille de sizeof(int)
		write(devID_A,(char*)pWrite,sizeof(int));
		write(devID_B,(char*)pWrite,sizeof(int));

		 
		gettimeofday(&currentTime,&zone);
		timeWrite = currentTime.tv_sec-timeZero.tv_sec;

		//printf("ecriture de %i(%i)(%i)\n",*pWrite,i,timeWrite);
		i++;
	
	}
	// deconexion du divice 
	close(devID_A);
	close(devID_B);
	pthread_exit(NULL);
}

int main(void)
{

	pthread_t sendDataID;
//	pthread_attr_t sendDataAttr;
	int rptc;

	
	pWrite= (int*) malloc(sizeof(int));

	devID_A = open(DEVICENAMEA,O_RDWR);
	devID_B = open(DEVICENAMEB,O_RDWR);
	*pWrite = initVal;

	printf("test Driver A sur fd = %i\n", devID_A);
	printf("test Driver B sur fd = %i\n", devID_B);

	if (devID_A>0 && devID_B>0)
	{
//		pthread_attr_init(&sendDataAttr);
//		pthread_attr_setdetachstate(&sendDataAttr,PTHREAD_CREATE_JOINABLE);
		
		gettimeofday(&timeZero,&zone); // mémorisation de l'instant de lancement du driver
		// ouverture du thread pour l'envoie des données (cf sendData)
		rptc = pthread_create(&sendDataID,NULL,sendData,NULL);

		if (rptc==0)
		{
			printf("ecriture ok\n");
		}
		else
		{
			printf("pb creation thread\n");
		}
	}
	else
	{
		printf("erreur sur l'ouverture du device\n");
	}
	
	pthread_exit(NULL);
}
