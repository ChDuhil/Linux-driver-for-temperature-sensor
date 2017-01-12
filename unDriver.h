/********************************************************************************
*
* Révision 1 : Christophe Duhil
*		Suppression de unsigned int time	
*
*********************************************************************************/




#ifndef UNDRIVER_H_
#define UNDRIVER_H_

#include <linux/ioctl.h>

// nombre max de divices pouvant être connecté
#define MAXDEVICES 16
#define DRIVERNAME "myDriver"

#define myDRIVER_IO_MAGIC 'd' 
#define RAZ _IO(myDRIVER_IO_MAGIC, 1)// @chd : TODO : voir comment il est construit
#define RAZ0 _IO(myDRIVER_IO_MAGIC, 2) // @chd : commande de raz pour myDivice0


// structure d'échange application / périphérique
typedef struct
{
  int value;		// donnée capteur
  int isCorrect;	// validité des données capteurs
  int pollData;		// donnée scrutée
  int cpt;		// nombre de mises à jour
  // unsigned int time;	// instant de mise à jour en secondes
} sensorInfo;

#endif

