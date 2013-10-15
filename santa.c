/* Druhy projekt do predmetu Operacni systemy */
/* Autor: Lubomir Stefl(xstefl05) */
/* Soubor: santa.c */
/* Datum dokonceni: 30.4.2013*/
/* Program resi synchronizacni problem znamy pod pojmem Santa Claus problem*/

#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<unistd.h>
#include<semaphore.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/mman.h>
#include<time.h>

#define CHYBA_PARAM 1
#define SDILENA_PAMET 2
#define CHYBA_FORK 3
#define CHYBA_VYTV_SEM 4
#define CHYBA_MAZ_SEM 5
#define CHYBA_MAZ_PAM 6
#define CHYBA_OTEV_SOU 7
#define CHYBA_ZAV_SOU 8

void arg(int argc, char *argv[]);
void chyba(int num);
void santa(void);
void napoveda(void);
void elf(void);
void sdilena_pamet(void);
void semafory(void);
void mazani_sem(void);
void mazani_pameti(void);
int gen_cisel(void);
int gen_cisel_vyr(void);
void zav_sou(void);

FILE *soubor;

int navsteva = 0, vyroba = 0, obsluha = 0, cislo_ulohy = 0, dovolena_elf = 0,
    tmp_elf = 0, elf_id = 0, aktivni_skritci = 0, obsluhujici_skritci = 0, cislo_elfa = 0;

int *p_cislo_ulohy = 0, *p_dovolena_elf = 0, *p_aktivni_skritci = 0,
    *p_obsluhujici_skritci = 0;

sem_t *s_santa, *s_elf, *s_zapis, *s_santa_end, *s_pomoc_elf, *s_cekarna, *s_sdilena;

pid_t pid_elfa, pid_santy;

/**hlavni funkce main**/
int main(int argc, char *argv[])
{
    if((soubor=fopen("santa.out", "w")) == NULL)
    {
        chyba(CHYBA_OTEV_SOU);
        return 2;
    }

    if((argc==2) && (strcmp("--help", argv[1]) == 0))
    {
        napoveda();
        return 0;
    }

/**volani argumentu**/
    arg(argc, argv);
    tmp_elf = cislo_elfa;

 /**********volani vytvoreni sdilene pameti******************/
    sdilena_pamet();
/***********volani vytvoreni semafaoru*****/
    semafory();

/********vytvoreni santy a elfu********/


    for(int i=0; i < tmp_elf + 1; i++)
	{
		switch(fork())
		{
			case 0: //dite
				if(i==0) // prvni dite == santa
				{
					santa();
					return 0;
				}
				else       // ostatni jsou elfove
				{
					elf_id=i;
					elf();
					return 0;
				}
				break;

			case -1:
				chyba(CHYBA_FORK);
				mazani_sem();
				mazani_pameti();
				return 2;
				break;

			default: //rodic

				break;
		}
	}

    for(int i = 0; i < tmp_elf + 1; i++)
    {
        wait(NULL);
    }
    mazani_sem();
    mazani_pameti();
    zav_sou();
    return 0;
}

/**funkce prace santy**/
void santa(void)
{
    sem_wait(s_zapis);  //nastartovani santy
        sem_wait(s_sdilena);
        (*p_cislo_ulohy)++;
        fprintf(soubor,"%d: santa: started\n", *p_cislo_ulohy);
        fflush(NULL);
        sem_post(s_sdilena);
    sem_post(s_zapis);


    while(1)
    {

        sem_wait(s_santa); // cekan pozadavek
        if((*p_dovolena_elf) != tmp_elf)
        {

            sem_wait(s_sdilena);

            (*p_cislo_ulohy)++;
            fprintf(soubor,"%d: santa: checked state: %d: %d\n", *p_cislo_ulohy, *p_aktivni_skritci, *p_obsluhujici_skritci);
            fflush(NULL);
            (*p_cislo_ulohy)++;
            fprintf(soubor,"%d: santa: can help\n", *p_cislo_ulohy);
            fflush(NULL);
            sem_post(s_sdilena);

            usleep(gen_cisel());     //obsluha skritka

           sem_post(s_cekarna);
        }
        else
        {   //zkontroluje zda uz jsou vsichni skritci na dovolene a zkonci

            sem_wait(s_sdilena);
            (*p_cislo_ulohy)++;
            fprintf(soubor,"%d: santa: checked state: %d: %d\n", *p_cislo_ulohy, *p_aktivni_skritci, *p_obsluhujici_skritci);
            fflush(NULL);
            sem_post(s_sdilena);
            break;
        }

    }

    sem_wait(s_sdilena);

    (*p_cislo_ulohy)++; //vypise, ze santa skoncil
    fprintf(soubor,"%d: santa: finished\n", *p_cislo_ulohy);
    fflush(NULL);
    sem_post(s_sdilena);

    sem_post(s_santa);
    sem_post(s_santa_end);      //otevre semafor skritkum ze take muzou skoncit

}

/**funkce prace skritka**/
void elf(void)
{
        int pomoc = 0;

        sem_wait(s_zapis);   //nastartovani vsech elfu

            sem_wait(s_sdilena);
            (*p_cislo_ulohy)++;
            fprintf(soubor,"%d: elf: %d: started\n", *p_cislo_ulohy, elf_id);
            fflush(NULL);
            (*p_aktivni_skritci)++;

            if(*p_aktivni_skritci == tmp_elf)
            {
                sem_post(s_elf);        //pusti az jsou nastartovani vsichni skritci

            }
            sem_post(s_sdilena);

        sem_post(s_zapis);


        sem_wait(s_elf);        //cekani az se nastartuji vsichni skritci
        sem_post(s_elf);




        while(pomoc < navsteva)
        {

             usleep(gen_cisel_vyr()); // prace skritka

                    sem_wait(s_sdilena);
                    (*p_cislo_ulohy)++;     // po praci vola, ze potrebuje pomoc
                    fprintf(soubor,"%d: elf: %d: needed help\n", *p_cislo_ulohy, elf_id);
                    fflush(NULL);
                    sem_post(s_sdilena);



            sem_wait(s_pomoc_elf); //prvni projde a potom dalsi ceka az se obslouzi v pripade jednoho a v pripade 3 jdou 3

            sem_wait(s_sdilena);
                    (*p_cislo_ulohy)++;
                    fprintf(soubor,"%d: elf: %d: asked for help\n", *p_cislo_ulohy, elf_id);
                    fflush(NULL);
                    (*p_obsluhujici_skritci)++;
            sem_post(s_sdilena);

            sem_wait(s_sdilena);
                if(*p_aktivni_skritci > 3 && *p_obsluhujici_skritci == 3)
                {
                    sem_post(s_santa);      //kdyz jsou tri tak pusti santu abz mohl obslouzit skritky
                }

                else if(*p_aktivni_skritci <= 3 && *p_obsluhujici_skritci == 1)
                {
                    sem_post(s_santa);
                }
                else
                    sem_post(s_pomoc_elf);

            sem_post(s_sdilena);

            sem_wait(s_cekarna); // ceka dokud santa neobslouzi skritka(skritky)

                sem_wait(s_sdilena);
                    (*p_cislo_ulohy)++;
                    fprintf(soubor,"%d: elf: %d: got help\n", *p_cislo_ulohy, elf_id);
                    fflush(NULL);
                    (*p_obsluhujici_skritci)--;
                    pomoc++;


                    if(pomoc == navsteva) //kdyz uz skritci dostali uvedeny pocet dle argumentu adchazi na dovolenou
                    {
                       (*p_cislo_ulohy)++;
                        fprintf(soubor,"%d: elf: %d: got a vacation\n", *p_cislo_ulohy, elf_id);
                        fflush(NULL);
                        (*p_dovolena_elf)++;
                        (*p_aktivni_skritci)--;
                    }

                    if(*p_obsluhujici_skritci == 0)
                    {
                        sem_post(s_pomoc_elf);
                    }
                    else
                        sem_post(s_cekarna);
                sem_post(s_sdilena);

        }

        sem_wait(s_sdilena);
                if((*p_dovolena_elf) == tmp_elf) //kdyz jsou vsichni skritci na dovolena zavola santu
                {
                    sem_post(s_santa);
                }
        sem_post(s_sdilena);

        sem_wait(s_santa_end); //az skonci santa tak skonci vsichni skritci

            (*p_cislo_ulohy)++;
            fprintf(soubor,"%d: elf: %d: finished\n", *p_cislo_ulohy, elf_id);
            fflush(NULL);
            zav_sou();
        sem_post(s_santa_end);
}

/**funkce na vytvoreni semaforu**/
void semafory(void)
{
    if((s_santa = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)
    {
        chyba(CHYBA_VYTV_SEM);
    }
    if((s_elf = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)
    {
        chyba(CHYBA_VYTV_SEM);
    }

    if((s_zapis = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)
    {
        chyba(CHYBA_VYTV_SEM);
    }
    if((s_santa_end = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)
    {
        chyba(CHYBA_VYTV_SEM);
    }
    if((s_pomoc_elf = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)
    {
        chyba(CHYBA_VYTV_SEM);
    }
    if((s_cekarna = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)
    {
        chyba(CHYBA_VYTV_SEM);
    }

    if((s_sdilena = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)
    {
        chyba(CHYBA_VYTV_SEM);
    }


    if((sem_init(s_santa, 1, 0)) == -1)
    {
        mazani_pameti();
        chyba(CHYBA_VYTV_SEM);
    }
    if((sem_init(s_elf, 1, 0)) == -1)
    {
        mazani_pameti();
        mazani_sem();
        chyba(CHYBA_VYTV_SEM);
    }
    if((sem_init(s_zapis, 1, 1)) == -1)
    {
        mazani_pameti();
        mazani_sem();
        chyba(CHYBA_VYTV_SEM);
    }
    if((sem_init(s_santa_end, 1, 0)) == -1)
    {
        mazani_pameti();
        mazani_sem();
        chyba(CHYBA_VYTV_SEM);
    }
    if((sem_init(s_pomoc_elf, 1, 1)) == -1)
    {
        mazani_pameti();
        mazani_sem();
        chyba(CHYBA_VYTV_SEM);
    }
    if((sem_init(s_cekarna, 1, 0)) == -1)
    {
        mazani_pameti();
        mazani_sem();
        chyba(CHYBA_VYTV_SEM);
    }
    if((sem_init(s_sdilena, 1, 1)) == -1)
    {
        mazani_pameti();
        mazani_sem();
        chyba(CHYBA_VYTV_SEM);
    }

}

/**funkce pro mazani semaforu**/
void mazani_sem(void)
{
    if((sem_destroy(s_santa)) == -1)
    {
        chyba(CHYBA_MAZ_SEM);
    }
    if((sem_destroy(s_elf)) == -1)
    {
        chyba(CHYBA_MAZ_SEM);
    }
    if((sem_destroy(s_zapis)) == -1)
    {
        mazani_pameti();
        mazani_sem();
        chyba(CHYBA_VYTV_SEM);
    }

    if((sem_destroy(s_santa_end)) == -1)
    {
        mazani_pameti();
        mazani_sem();
        chyba(CHYBA_VYTV_SEM);
    }
    if((sem_destroy(s_pomoc_elf)) == -1)
    {
        mazani_pameti();
        mazani_sem();
        chyba(CHYBA_VYTV_SEM);
    }
    if((sem_destroy(s_cekarna)) == -1)
    {
        mazani_pameti();
        mazani_sem();
        chyba(CHYBA_VYTV_SEM);
    }

    if((sem_destroy(s_sdilena)) == -1)
    {
        mazani_pameti();
        mazani_sem();
        chyba(CHYBA_VYTV_SEM);
    }
}

/**funkce na vytvoreni sdilene pameti**/
void sdilena_pamet(void)
{
/********************cislo ulohy***************/
    if((cislo_ulohy = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
    {
        chyba(SDILENA_PAMET);
    }

    if((p_cislo_ulohy = (int *)shmat(cislo_ulohy, NULL, 0)) == (void *) -1)
    {
        chyba(SDILENA_PAMET);
    }
    *p_cislo_ulohy=0;

/*************skritci na dovolene****************/
    if((dovolena_elf = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
    {
        chyba(SDILENA_PAMET);
    }

    if((p_dovolena_elf = (int *)shmat(dovolena_elf, NULL, 0)) == (void *) -1)
    {
        mazani_pameti();
        chyba(SDILENA_PAMET);
    }
    *p_dovolena_elf=0;

/*******aktivni skritci*****/
    if((aktivni_skritci = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
    {
        chyba(SDILENA_PAMET);
    }

    if((p_aktivni_skritci = (int *)shmat(aktivni_skritci, NULL, 0)) == (void *) -1)
    {
        mazani_pameti();
        chyba(SDILENA_PAMET);
    }
    *p_aktivni_skritci=0;
/******oblsuhujici skritci*****/
     if((obsluhujici_skritci = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
    {
        chyba(SDILENA_PAMET);
    }

    if((p_obsluhujici_skritci = (int *)shmat(obsluhujici_skritci, NULL, 0)) == (void *) -1)
    {
        mazani_pameti();
        chyba(SDILENA_PAMET);
    }
    *p_obsluhujici_skritci=0;

}
/**funkce mazani sdilene pameti**/
void mazani_pameti(void)
{
    shmdt(p_cislo_ulohy);
    shmdt(p_dovolena_elf);
    shmdt(p_aktivni_skritci);
    shmdt(p_obsluhujici_skritci);
    if((shmctl(cislo_ulohy, IPC_RMID, NULL)) == -1)
    {
        chyba(CHYBA_MAZ_PAM);
    }
    if((shmctl(dovolena_elf, IPC_RMID, NULL)) == -1)
    {
        chyba(CHYBA_MAZ_PAM);
    }
    if((shmctl(aktivni_skritci, IPC_RMID, NULL)) == -1)
    {
        chyba(CHYBA_MAZ_PAM);
    }
    if((shmctl(obsluhujici_skritci, IPC_RMID, NULL)) == -1)
    {
        chyba(CHYBA_MAZ_PAM);
    }

}
/**funkce pro kontrolu argumentu**/
void arg(int argc, char *argv[])
{
    char *spatny_arg;

    if(argc != 5 )
    {
        chyba(CHYBA_PARAM);
    }

    else
    {   // pocet navstev skritka u santy
        if(isdigit(*argv[1]))
        {
            navsteva=strtol(argv[1], &spatny_arg, 10);
            if(spatny_arg == argv[1] || *spatny_arg !='\0' || navsteva <= 0)
            {
                chyba(CHYBA_PARAM);
            }
        }
        else
        {
            chyba(CHYBA_PARAM);
        }
        // pocet skritku kteri na zacatku pracuji na vyrobe hracek
        if(isdigit(*argv[2]))
        {
            cislo_elfa=strtol(argv[2], &spatny_arg, 10);
            if(spatny_arg == argv[2] || *spatny_arg !='\0' || cislo_elfa <= 0)
            {
                chyba(CHYBA_PARAM);
            }
        }
        else
        {
            chyba(CHYBA_PARAM);
        }
        // max doba(v milisekundach) vyroby hracek skritkem
        if(isdigit(*argv[3]))
        {
            vyroba=strtol(argv[3], &spatny_arg, 10);
            if(spatny_arg == argv[3] || *spatny_arg !='\0' || vyroba < 0)
            {
                chyba(CHYBA_PARAM);
            }
        }
        else
        {
            chyba(CHYBA_PARAM);
        }
        // max doba(v milisekundach) obsluhy skritku santou
        if(isdigit(*argv[4]))
        {
            obsluha=strtol(argv[4], &spatny_arg, 10);
            if(spatny_arg == argv[4] || *spatny_arg !='\0' || obsluha < 0)
            {
                chyba(CHYBA_PARAM);
            }
        }
        else
        {
            chyba(CHYBA_PARAM);
        }

    }

}
/**funkce na zavirani souboru**/
void zav_sou(void)
{
    if((fclose(soubor)) == EOF)
    {
        chyba(CHYBA_ZAV_SOU);
        mazani_pameti();
        mazani_sem();

    }
}

/**funkce na vypis chyb**/
void chyba(int num)
{
    switch(num)
    {
        case CHYBA_PARAM:
            fprintf(stderr,"Byly zadany spatne parametry.\nNapoveda ./santa --help\n");
            exit(2);
            break;

        case SDILENA_PAMET:
                fprintf(stderr,"Chyba ve vytvareni sdilene pameti.\n");
                break;

        case CHYBA_FORK:
            fprintf(stderr,"Chyba pri vytvareni procesu.\n");
            break;

        case CHYBA_VYTV_SEM:
            fprintf(stderr,"Chyba pri vytvareni semaforu.\n");
            break;

        case CHYBA_MAZ_SEM:
            fprintf(stderr,"Nepodarilo se smazat semafor.\n");
            break;

        case CHYBA_MAZ_PAM:
            fprintf(stderr,"Nepodarilo se smazat pamet.\n");
            break;

        case CHYBA_OTEV_SOU:
            fprintf(stderr,"Nepodarilo se otevrit soubor\n");
            break;

        case CHYBA_ZAV_SOU:
            fprintf(stderr,"Nepodarilo se zavrit soubor\n");
            break;
   }

}

int gen_cisel(void)     // generator cisel pro delku obsluhy santou
{
    if(obsluha == 0)
    {
        return 0;
    }
    else
    {

        int tmp;
        srand (time(NULL));
        tmp = rand()%(vyroba +1);
        if(tmp < 0)
        {
            tmp *= -1;
            return tmp*1000;
        }
        else
            return tmp*1000;
    }

}
int gen_cisel_vyr(void)     // generator cisel pro delku vyroby hracek
{

    if(obsluha == 0)
    {
        return 0;
    }
    else
    {

        int tmp;
        srand (time(NULL));
        tmp = rand()%(obsluha +1);
        if(tmp < 0)
        {
            tmp *= -1;
            return tmp*1000;
        }
        else
            return tmp*1000;
    }

}

void napoveda(void) //funkce pro napovedu
{
    printf("Napoveda pro santa.c\n"
           "Spravny zapis argumentu je takto:\n"
           "./santa.c 1 2 3 4 kde:\n"
           "1 = pocet navstev u skritka\n"
           "2 = pocet skritku\n"
           "3 = max doba vyroby hracek skritkem\n"
           "4 = max doba obsluhy santou\n"
            );
}
