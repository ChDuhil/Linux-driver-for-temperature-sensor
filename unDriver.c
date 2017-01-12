
/*******************************************************************************************************
* Révision 3: Chritophe Duhil
*		Modification de myRead et myWrite pour controler l'âge de la donnée 
*		si l'age est superieur à MAXTIME la donnée n'est pas valide 
*                        **********************
* Révision 2: Christophe Duhil
*		Dans myRead :vérification de l'age de la donnée. 
*		Si l'age est supérieur à MAXTIME, la valeur n'est pas lue et reste identique à la valeur précédente
*						***********************
* Révision 1: christophe Duhil
*	1/ 	Ajout d'un test dans  myWrite pour la vérification de la validité des données.
*		Pour être valide, les données du capteur doivent être comprises entre MINVALUE et MAXVALUE
*	 	Si valide la variable iscorrect de la structure sensorInfo est mise à 1 et la donnée recopiée dans
*		la variable info de la structure sensor info
*		Si non valide iscorrect =0 et la donnée n'est pas recopiée.
*
*	2/	Dans myRead ajout d'un test sur la variable iscorrect de la structure sensorInfo pour vérifier la validité 
*		des données.
*		si valide : les données sont recopiées dans le buffer et la fonction retourne la taille de sensorInfo.
*		si non valide retourne 0
*********************************************************************************************************/


#include <linux/module.h> // needed for modules
#include <linux/kernel.h> // needed for KERN_ALERT
#include <linux/init.h>   // needed for macros
#include <linux/fs.h>     // needed for driver management
#include <asm/uaccess.h>  // needed for copy_to/from_user
#include <linux/time.h>	  // needed for time services
#include <linux/timer.h>  // needed for timer services
 

#include "unDriver.h"
#include "sensor.h"  // @chD : fichier contenant la plage de valeurs du capteur.

#define TIMERPERIOD 1250

MODULE_AUTHOR("JP Babau");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("premier driver");


static int myMajor = 0;   // allocation dynamique


int tabTime[MAXDEVICES];  // @chD : tableau de l'instant de la derniere ecriture pour le device [i]

struct timeval timeZero;	// instant de démarrage du driver
struct timeval currentTime; 	// instant courant

struct timer_list myTimer;	// timer périodique
static int counter;
spinlock_t lockCounter;		// protection de counterf

// structure spécifique au périphérique
typedef struct
{
  int ad;		// id périphérique
  sensorInfo *info;	// données spécifiques
  //struct semaphore semProtect; // mutex de protection du périphérique
} devInfo;

// tableau statique des périphériques
devInfo theDevices[MAXDEVICES];


static ssize_t myOpen(struct inode *inode, struct file* file)
{
	printk("open Divice");
	//unsigned  int minor  = MINOR(inode->i_rdev);
	unsigned  int minor  = iminor(inode);

	// le minor correspond au rang dans le tableau des périphériques
	if ((minor>=0) && (minor<MAXDEVICES) )
	{

		// association descripteur périphérique/données spécifiques
		file->private_data = (devInfo*) &theDevices[minor];

		printk(KERN_ALERT "open() OK sur device %i\n",minor);
		return 0;
	}
	else
	{
		printk(KERN_ALERT "open() KO sur device %i\n",minor);
		return -1;		
	}
}

/**************************************************************************************
*	Modification de myRead pour la  vérification des données :
*	Ajout d'un test pour la vérification de la l'age de la donnée
*
***************************************************************************************/
static ssize_t myRead(struct file *file, char * buf, size_t count, loff_t *ppos)
{
	devInfo * p = (devInfo*) file->private_data;
	unsigned int minor = MINOR(file->f_dentry->d_inode->i_rdev);


	do_gettimeofday(&currentTime);//  recupération du temps courant
	// comparaison du temps courant par rapport au temps départ
	if (p->info->isCorrect && (currentTime.tv_sec-timeZero.tv_sec-tabTime[minor]<= MAXTIME))
	{
		copy_to_user(buf,(sensorInfo*)(p->info), sizeof(sensorInfo));
	} else {
		return 0; // @chD : si supérieur à maxtime retun 0.
	}
	
	printk(KERN_ALERT "read() %i(%i) dans le device %i\n",p->info->value,p->info->cpt,minor);

	return (sizeof(sensorInfo));
}

/*********************************************************************************
*  Modification de myWrite pour la  vérification des données :
*  Ajout d'un test sur les valeurs minimales et maximales de de la plage de donées
*  Ajour d'un test sur l'age de la donnée.
*  Suppression de p->info->time = currentTime.tv_sec-timeZero.tv_sec;
**********************************************************************************/
static ssize_t myWrite(struct file *file, const char * buf, size_t count, loff_t *ppos)
{

	devInfo * p = (devInfo*) file->private_data;
	unsigned int minor = MINOR(file->f_dentry->d_inode->i_rdev);
	printk("writefunc");

	// sauvegarde des données périphériques
	int old_value = p->info->value; // sauvegarde de la donnée précédente
	copy_from_user((int*)&(p->info->value), buf, sizeof(int)); // récupération de la donnée du capteur 


	// test sur la validitées des données la valeur doit être dans la plage de validité MINVALUE - MAXVALUE
	if ((p->info->value)>MINVALUE && (p->info->value)< MAXVALUE)
	{
		p->info->isCorrect = 1 ; // isCorrect = 1 si la donnée est valide
	}
	else
	{
		p->info->isCorrect = 0 ; // isCorrect = 0 is la donnée n'est pas valide
		p->info->value = old_value; // la valeur (t-1) est recopiée 
		return 0; // la fonction retourne 0
	}
	
	p->info->cpt++;
	do_gettimeofday(&currentTime);
	// p->info->time = currentTime.tv_sec-timeZero.tv_sec; // ligne supprimée pour la gestion de l'age de la donnée.
	tabTime[minor] = currentTime.tv_sec-timeZero.tv_sec; // save time when wrote
	
	spin_lock_irq(&lockCounter);
	p->info->pollData = counter;
	spin_unlock_irq(&lockCounter);

	return (sizeof(int));
}

static ssize_t myRelease(struct inode *inode, struct file* file)
{
	printk(KERN_ALERT "close()\n");
	return 0;
}


/**********************************************************************************************
*	Modification de myIoctl
*	remplacement du case RAZ pour un fonctionnement uniquement avec myDevice0
***********************************************************************************************/
static int myIoctl(struct inode * inode, struct file * file, unsigned int cmd, unsigned long arg)
{
	devInfo * p = (devInfo*) file->private_data;
	unsigned int minor = MINOR(file->f_dentry->d_inode->i_rdev);
	int ret;

	switch(cmd)
	{
		//case RAZ : p->info->cpt=0;break; ** commande d'origine passée en commentaire **
		case RAZ0 : 
			if (minor == 0) p->info->cpt=0; // commande spécifique a mydivice0
			break; // @chD commande spécifique a mydivice0
		default : ret = -EINVAL;break;
	}

	printk(KERN_ALERT "ioctl() %i dans le device %i\n",cmd,minor);

	return (ret);
}

static struct file_operations fops =
{
	open : myOpen,
	read : myRead,
	write : myWrite,
	ioctl : myIoctl,
	release : myRelease
};


void periodicCounter(unsigned long arg)
{
	spin_lock_irq(&lockCounter);
	counter++;
	spin_unlock_irq(&lockCounter);

//	printk(KERN_DEBUG "timer Activation : %i\n",counter);	
	
	myTimer.expires += (unsigned long) TIMERPERIOD;
	add_timer(&myTimer);
}

static int __init install(void)
{
	int installCR;
	int i;

	printk(KERN_ALERT "hello\n");

	installCR = register_chrdev(myMajor,DRIVERNAME,&fops);

	if (installCR <0)
	{
		printk(KERN_WARNING "probleme d'installation\n");
		return installCR;
	}

	myMajor = installCR;

	// initialisation des informations périphériques
	for (i=0;i<MAXDEVICES;i++)
	{
		theDevices[i].info=kmalloc(sizeof(sensorInfo),GFP_KERNEL);	
		theDevices[i].info->cpt = 0;
		theDevices[i].info->value = -1;
		//sema_init(&theDevices[i].semProtect,0);
	}
	spin_lock_init(&lockCounter);		
	
	do_gettimeofday(&timeZero); // mémorisation de l'instant de lancement du driver

	init_timer(&myTimer);
	myTimer.expires = jiffies + (unsigned long) TIMERPERIOD;
	myTimer.function = periodicCounter;
	counter = 0;
	add_timer(&myTimer);

	printk(KERN_DEBUG "installation OK, MAJOR NUMBER: %i\n",myMajor);
	return 0;
}

static void __exit desinstall(void)
{
	//int desInstallCR;
	int i;

	printk(KERN_ALERT "see you\n");

	unregister_chrdev(myMajor,DRIVERNAME);

	for (i=0;i<MAXDEVICES;i++)
	{
		kfree(theDevices[i].info);	
	}

	del_timer(&myTimer);

	printk(KERN_DEBUG "desinstallation OK\n");
}


module_param(myMajor,int,0);
MODULE_PARM_DESC(myMajor,"le numero de majeur en parametre du module");

module_init(install);
module_exit(desinstall);

