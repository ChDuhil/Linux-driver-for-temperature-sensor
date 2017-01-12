/************************************************************************************************
* Revision 2 : Christophe Duhil
*	Objectif : lire les informations de températures sur DEVICENAMEA et DEVICENAMEB 
*		faire ma moyenne des deux dernieres lectures
*		Afficher à l'écran la valeur moyenne de DEVICENAMEA et DEVICENAMEB
*							------------------
* 1/ 	Creation d'un thread pollS1 pour lecture sur device1
*		Création de la fonction read_A definissant la tache de pollS1
* 2/    Creation d'un thread pollS2 pour lecture sur device2
*		Création de la fonction read_B definissant la tache de pollS2
* 3/    Création d'un thread Print pour l'impression de la valeur moyenne lue sur les deux capteurs
*		Création de la fonction v_print definissant la tache de Print
* 4/ 	ajout des déclarations de constantes utiles au programme 
*		Modification du main pour l'ouverture de DEVICENAMEA et DEVICENAMEB
*		lancement des threads
*
*							*******************
*
* Révision 1 : Christophe Duihl
*  		Supression de pRead->time
************************************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pthread.h>
#include "unDriver.h"

#define DEVICENAMEA "/dev/myDevice1" // adresse du device1
#define DEVICENAMEB "/dev/myDevice2" // adresse du device 2
#define PERIOD_A 1000 // periode pour usleep en micro seconde
#define PERIOD_B 2000 // periode pour usleep en micro seconde
#define DIVISEUR 2    // Définition d'une constante diviseur

static int valMoyenne ; // variable globale valmoyenne utilisée dans read_A et read_B
static pthread_mutex_t valMoy_mutex; // Déclaration du mutex pour l'utilisation de valMoyenne
static int devID_A ; 
static int devID_B ;
static int val_A = 0; // valeur de la derniere lecture de DEVICENAMEA
static int val_B = 0; // valeur de la dernière lecture de DEVICENAMEB
static int w_val ;	  // Valeur 


/********************************************************************************************
* Fonction read_A
* Tache utilisée par le thread PollS1.
* Lecture de la sonde de température sur DEVICENAMEA toutes les PERIOD_A. 
* Mise à jour de la variable globale valMoyenne : moyenne des deux dernières valeurs lues sur DEVICENAMEA et DEVICENAMEB 
**********************************************************************************************/
void * read_A(void * null){


	sensorInfo *pReadA = (sensorInfo*) malloc(sizeof(sensorInfo));
	int redBytes;
	printf("read_A : test Driver sur fd  = %i\n", devID_A);



	while(1) {
		usleep(PERIOD_A); // Mise en sommeil de la fonction pendant "PERIOD_A"
		ioctl(devID_A,RAZ0,0); // Modification de RAZ vers RAZ0 pour ex2	
		redBytes= read(devID_A,(char*)pReadA,sizeof(sensorInfo));// lecture des informations sensor de DEVICENAMEA

		
		pthread_mutex_lock(&valMoy_mutex); //Début de la section critique 
		if (redBytes) // si la lecture de DEVICENAMEA est conforme 
		{
			
			val_A = pReadA->value ; // Mise à jour de la variable globale val_A
			valMoyenne = (val_A + val_B)/DIVISEUR ; // calcul de la valeur moyenne 
			w_val = 0; // ré_initialisation de la variable valeur erronée à 0
			
		}
		else if (w_val > 1) // si la précédente lecture et la lecture en cours sont fausses.
		{ 
			valMoyenne = -1 ; 
		}
		else  // si la lecture en cours est fausse.
		{
			w_val ++ ;
		}
		pthread_mutex_unlock(&valMoy_mutex); //fin de la section critique.		 
	}
		
		close(devID_A); 
		pthread_exit(NULL);
}

/********************************************************************************************
* Fonction read_B
* Tache utilisée par le thread PollS2.
* Lecture de la sonde de température sur DEVICENAMEA toutes les PERIOD_B. 
* Mise à jour de la variable globale valMoyenne : moyenne des deux dernières valeurs lues sur DEVICENAMEA et DEVICENAMEB 	
**********************************************************************************************/
void * read_B(void * null){


	// instensiation de sensorInfo
	sensorInfo *pReadB = (sensorInfo*) malloc(sizeof(sensorInfo));

	int redBytes;
	
	int once_again = 1;
	

	printf("read_B : test Driver sur fd = %i\n", devID_B);
	


	while (1){

		usleep(PERIOD_B); // Mise en sommeil de la fonction pendant "PERIOD_B"
		ioctl(devID_B,RAZ0,0); // Modification de RAZ vers RAZ0 pour ex2
		
		redBytes= read(devID_B,(char*)pReadB,sizeof(sensorInfo));

		
		pthread_mutex_lock(&valMoy_mutex);//Début de la section critique.
		if (redBytes) // si la lecture de DEVICENAMEA est conforme. 
		{	
			val_B = pReadB->value ; // Mise à jour de la variable globale val_B.
			valMoyenne = (val_A + val_B)/DIVISEUR ; // calcul de la valeur moyenne.
			w_val = 0 ; // ré_initialisation de la variable "valeur erronée" à 0		
		}
		else if (w_val > 1) // si la précédente lecture et la lecture en cours sont fausses.
		{
			valMoyenne = -1 ;
		}
		else // si la lecture en cours est fausse.
		{
			w_val ++ ;

		}
		pthread_mutex_unlock(&valMoy_mutex); // fin de la section critique 		
	}
	close(devID_B); //fermeture de DEVICENAMEB 
	pthread_exit(NULL); // sortie du thread
}


/************************************************************************************************
* Fonction v_print
* Affiche à l'écran la valeur moyenne lue sur DEVICENAMEA et DEVICENAMEB
* l'affichage est déclenché sur modification de valMoyenne.
*************************************************************************************************/
void * v_print(void * null)
{
	int prev_val = -1; //initialisation de prev_val: stock la précédente valeur lue


	while (1)
	{
		
		if (prev_val != valMoyenne && valMoyenne != -1) // si la valeur précédente est modifiée
		{
			printf("valMoyenne = %d\n",valMoyenne); // affichage de la valeur
			prev_val = valMoyenne ; // la valeur présente devient la valeur précédente

		}
		else if (prev_val!=-1 && valMoyenne == -1) // si deux valeurs consécutives en erreur 
		{
			printf("Sensor Error\n"); // affichage d'un message d'érreur.
			prev_val=-1 ;
		}		
	}
	pthread_exit(NULL); // sortie du thread
}


int main(void)
{
	// déclaration des threads
	pthread_t pollS1;	
	pthread_t pollS2;
	pthread_t print;

	
	devID_A = open(DEVICENAMEA,O_RDONLY); // Ouverture de DEVICENAMEA
	if (devID_A>0)
	{
	if(pthread_create(&pollS1,NULL,read_A,NULL)) // lancement du pthread pollS1
		perror("erreur sur pollS1");	
	}
	else
	{
		printf("erreur sur l'ouverture du device\n");
	}
	

	devID_B = open(DEVICENAMEB,O_RDONLY); // Ouverture de DEVICENAMEB
	if (devID_B>0)
	{
	
	if(pthread_create(&pollS2,NULL,read_B,NULL))// lancement du pthread pollS2
		perror("erreur sur pollS2");		
	}
	else
	{
		printf("erreur sur l'ouverture du device\n");
	}



	if(pthread_create(&print,NULL,v_print,NULL))// lancement du pthread print
	{
		perror("erreur sur pollS2");		
	
	}
	else
	{
		printf("erreur sur l'ouverture du device\n");
		
	}

	pthread_exit(NULL); // fermeture des threads

	return 0;
}
