/**
 * brief: IOS project 2 - Building H2O
 * file: proj2.c
 * author: Behal Tomas xbehal02@stud.fit.vubr.cz
 * date: 2022-04-15 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <time.h>

#include "proj2.h"

// Arguments validation
bool args_valid(int argc, char *argv[], ARGS *args){
    if(argc != 5){
        return false;
    }

    char *emptystr = "";
    char *buffer;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i],emptystr) == 0)
            return false;

        int ret = strtol(argv[i], &buffer, 10);

        if ( ret < 0 || *buffer != '\0' )
            return false;
        else if ( (i == 1 || i ==2 ) && ret == 0 )
            return false;
        else if ( (i == 3 || i == 4) && ret > 1000 )
            return false;
    }
    // Set arguments to structure
    args->NO = atoi(argv[1]);
    args->NH = atoi(argv[2]);
    args->TI = atoi(argv[3]);
    args->TB = atoi(argv[4]);
    return true;
} 

// Memory cleanup
void sh_mem_cleanup(int sh_id, SH_MEMORY *sh_mem){
    shmdt(sh_mem);
    shmctl(sh_id, IPC_RMID, NULL);
}

// Semaphore cleanup
void sem_cleanup(SEMAPHORE *sem){
    sem_close(sem->oxygen);
    sem_close(sem->hydrogen);
    sem_close(sem->mutex);
    sem_close(sem->barrier1);
    sem_close(sem->barrier2);
    sem_close(sem->bondH);
    sem_close(sem->molecule);
    sem_close(sem->print);
}

// Semaphore unlink
void sem_un(){
    sem_unlink("/xbehal02_sem_oxygen");
    sem_unlink("/xbehal02_sem_hydrogen");
    sem_unlink("/xbehal02_sem_mutex");
    sem_unlink("/xbehal02_sem_barrier1");
    sem_unlink("/xbehal02_sem_barrier2");
    sem_unlink("/xbehal02_sem_bondH");
    sem_unlink("/xbehal02_sem_molecule");
    sem_unlink("/xbehal02_sem_print");
}

void process_oxygen(ARGS *args, SEMAPHORE *sem, SH_MEMORY *sh_memory, FILE *file, int idO){
    srand(time(NULL) * getpid());

    // Oxygen started
    sem_wait(sem->mutex);
        sem_wait(sem->print);
            sh_memory->counter += 1;
            fprintf(file, "%d: O %d: started\n", sh_memory->counter, idO);
            fflush(file);
        sem_post(sem->print);
    sem_post(sem->mutex);

    usleep((rand() % (args->TI + 1)) * 1000);

    // Oxygen going to queue
    sem_wait(sem->mutex);
        sem_wait(sem->print);
            sh_memory->counter += 1;
            sh_memory->NO += 1;
            fprintf(file, "%d: O %d: going to queue\n", sh_memory->counter, idO);
            fflush(file);
        sem_post(sem->print);
    
    // Checking if there is enough hydrogen in queue -> then molecule can be created
    // Or molecule cant be created -> prints not enough Hydrogen
    if(sh_memory->NH >= 2){

        sh_memory->NH -= 2;
        sh_memory->NO -= 1;
        sem_post(sem->mutex);

        sem_wait(sem->molecule);

        sem_wait(sem->mutex);
            sem_post(sem->hydrogen);
            sem_post(sem->hydrogen);

            sem_post(sem->oxygen);
        sem_post(sem->mutex);
    }
    else if(sh_memory->H20 == 0){
            sem_post(sem->oxygen);
            
                sem_wait(sem->print);
                    sh_memory->counter++;
                    fprintf(file, "%d: O %d: not enough H\n", sh_memory->counter, idO);
                    fflush(file);
                sem_post(sem->print);

        sem_post(sem->mutex);
	
	    exit(0);
	}
    else{
        sem_post(sem->mutex);
    }

    // Oxygen waiting to be released -> waits for two Hydrogens
    sem_wait(sem->oxygen);

    // If there is not enough Hydrogens to bond with -> prints not enough Hydrogen
    if(sh_memory->H20 <= sh_memory->cntO){
        sem_wait(sem->mutex);
            sem_post(sem->oxygen);
                sem_wait(sem->print);
                    sh_memory->counter++;
                    fprintf(file, "%d: O %d: not enough H\n", sh_memory->counter, idO);
                    fflush(file);
                sem_post(sem->print);
        sem_post(sem->mutex);
	
        // Kills process
	    exit(0);
	}

    // Oxygen starts to bond with two Hydrogens
    sem_wait(sem->mutex);
        sem_wait(sem->print);
            sh_memory->counter += 1;
            fprintf(file, "%d: O %d: creating molecule %d\n", sh_memory->counter, idO, sh_memory->NoM);
            fflush(file);
        sem_post(sem->print);
    sem_post(sem->mutex);

    // Sleep to simulate bonding
    usleep((rand() % (args->TB + 1)) * 1000);

    // Start of barrier
    sem_wait(sem->mutex);
    sh_memory->cnt += 1;
        if(sh_memory->cnt == 3){
            sem_wait(sem->barrier2);
            sem_post(sem->barrier1);
        }
    sem_post(sem->mutex);

    sem_wait(sem->barrier1);
    sem_post(sem->barrier1);

    // Oxygen signals to Hydrogens molecule is created
    sem_post(sem->bondH);
    sem_post(sem->bondH);

    // Molecule is created
    sem_wait(sem->mutex);
        sem_wait(sem->print);
            sh_memory->counter += 1;
            fprintf(file, "%d: O %d: molecule %d created\n", sh_memory->counter, idO, sh_memory->NoM);
            fflush(file);
        sem_post(sem->print);
    sem_post(sem->mutex);

    // Second part of barrier
    sem_wait(sem->mutex);
        sh_memory->cnt -= 1;
        if(sh_memory->cnt == 0){ // If all 3 processes are finished -> molecule is created
            sem_wait(sem->barrier1);
            sem_post(sem->barrier2);

            sh_memory->NoM += 1; // Molecule counter increments
            sem_post(sem->molecule); // MOlecule barrier is released
        }
    sem_post(sem->mutex);

    sem_wait(sem->barrier2);
    sem_post(sem->barrier2);

    // Checking if this Oxygen isnt the last one that can create molecule -> if so, releases Oxygen and Hydrogens
    sem_wait(sem->mutex);
        sh_memory->cntO += 1;
        if(sh_memory->H20 <= sh_memory->cntO) // If true -> relases Oxygen
            sem_post(sem->oxygen);

        if((sh_memory->H20 * 2) <= sh_memory->cntH) // If true -> relases Hydrogen
            sem_post(sem->hydrogen);
    sem_post(sem->mutex);

    // Kills process
    exit(0);
}

void process_hydrogen(ARGS *args, SEMAPHORE *sem, SH_MEMORY *sh_memory, FILE *file, int idH){
    srand(time(NULL) * getpid());

    // Hydrogen started
    sem_wait(sem->mutex);
        sem_wait(sem->print);
        sh_memory->counter += 1;
        fprintf(file, "%d: H %d: started\n", sh_memory->counter, idH);
        fflush(file);
        sem_post(sem->print);
    sem_post(sem->mutex);
    
    usleep((rand() % (args->TI + 1)) * 1000);

    // Hydrogen going to queue
    sem_wait(sem->mutex);
        sem_wait(sem->print);
            sh_memory->counter += 1;
            sh_memory->NH += 1;
            fprintf(file, "%d: H %d: going to queue\n", sh_memory->counter, idH);
            fflush(file);
        sem_post(sem->print);

    // Checking if there is enough hydrogen and enough oxygen in queue -> then molecule can be created
    // Or molecule cant be created -> prints not enough Oxygen or Hydrogen
    if(sh_memory->NH >= 2 && sh_memory->NO >= 1){
        
        sh_memory->NH -= 2;
        sh_memory->NO -= 1;
        sem_post(sem->mutex);

        sem_wait(sem->molecule);

        sem_wait(sem->mutex);
            sem_post(sem->hydrogen);
            sem_post(sem->hydrogen);

            sem_post(sem->oxygen);
        sem_post(sem->mutex);
    }
    else if(sh_memory->H20 == 0){
        sem_post(sem->hydrogen);
        
            sem_wait(sem->print);
                sh_memory->counter++;
                fprintf(file, "%d: H %d: not enough O or H\n", sh_memory->counter, idH);
                fflush(file);
            sem_post(sem->print);

        sem_post(sem->mutex);

        exit(0);
	}
    else{
        sem_post(sem->mutex);
    }
    
    // Hydrogen waiting to be released -> waits for Oxygen and Hydrogen
    sem_wait(sem->hydrogen);
    
    // If there is not enough Oxygen or Hydrogen to bond with -> prints not enough Oxygen or Hydrogen
    if((sh_memory->H20 * 2) <= sh_memory->cntH){
        sem_wait(sem->mutex);
            sem_post(sem->hydrogen);
                sem_wait(sem->print);
                    sh_memory->counter++;
                    fprintf(file, "%d: H %d: not enough O or H\n", sh_memory->counter, idH);
                    fflush(file);
                sem_post(sem->print);
        sem_post(sem->mutex);
	
        // Kills process
	    exit(0);
	}

    // Hydrogen starts to bond with Oxygen
    sem_wait(sem->mutex);
        sem_wait(sem->print);
            sh_memory->counter += 1;
            fprintf(file, "%d: H %d: creating molecule %d\n", sh_memory->counter, idH, sh_memory->NoM);
            fflush(file);
        sem_post(sem->print);
    sem_post(sem->mutex);

    // Start of barrier
    sem_wait(sem->mutex);
        sh_memory->cnt += 1;
        if(sh_memory->cnt == 3){// If there are 3 processes in queue -> molecule can be created
            sem_wait(sem->barrier2);
            sem_post(sem->barrier1);
        }
    sem_post(sem->mutex);

    sem_wait(sem->barrier1);
    sem_post(sem->barrier1);

    // Hydrogen is waiting for signal from Oxygen that molecule is created
    sem_wait(sem->bondH);

    // Molecule is created
    sem_wait(sem->mutex);
        sem_wait(sem->print);
            sh_memory->counter += 1;
            fprintf(file, "%d: H %d: molecule %d created\n", sh_memory->counter, idH, sh_memory->NoM);
            fflush(file);
        sem_post(sem->print);
    sem_post(sem->mutex);

    // Second part of barrier
    sem_wait(sem->mutex);
        sh_memory->cnt -= 1;
        if(sh_memory->cnt == 0){ // If all 3 processes are finished -> molecule is created
            sem_wait(sem->barrier1);
            sem_post(sem->barrier2);
            
            sh_memory->NoM += 1; // Molecule counter increments
            sem_post(sem->molecule); // Molecule barrier is released
        }
    sem_post(sem->mutex);

    sem_wait(sem->barrier2);
    sem_post(sem->barrier2);


    //Checking if this Hydrogen isnt last that can create molecule -> if so, releases Hydrogen and Oxygen
    sem_wait(sem->mutex);
    sh_memory->cntH += 1;
    if(sh_memory->H20 <= sh_memory->cntO) // If true -> relases Oxygen
        sem_post(sem->oxygen);

    if((sh_memory->H20 * 2) <= sh_memory->cntH) // If true -> relases Hydrogen
        sem_post(sem->hydrogen);
    
    sem_post(sem->mutex);

    // Kills process 
    exit(0);
}

// How many molecules can be created for given arguments NO and NH
int molecule_count(int a, int b){
    return (((a) < (b)) ? (a) : (b));
}

int main(int argc, char *argv[])
{   
    // Check for invalid arguments
    ARGS args;
    if (!args_valid(argc, argv, &args)){
        fprintf(stderr,"Wrongly used arguments\nUsage: ./proj2 [NO] [NH] [TI] [TB]\n"
        "\tNO - Number of oxygen atoms (NO>0)\n\tNH - Number of hydrogen atoms (NH>0)\n"
        "\tTI - Time for atoms to go to queue (0<=TI<=1000)\n\tTB - Time for molecule to be build (0<=TB<=1000)\n");
        return EXIT_FAILURE;
    }

    // Output file setup
    FILE *file;
    if ((file = fopen("proj2.out", "w")) == NULL) {
        fprintf(stderr,"Problem with output file opening\n");
        return EXIT_FAILURE;
    }

    // Shared memory setup
    SH_MEMORY *sh_memory;
    key_t sh_key = ftok("/xbehal02_proj2", 9);
    int sh_id = shmget(sh_key, sizeof(SH_MEMORY), IPC_CREAT | 0666);
    if (sh_id < 0) {
        fprintf(stderr,"Problem with shmemory\n");
        shmctl(sh_id, IPC_RMID, NULL);
        fclose(file);
        return EXIT_FAILURE;
    }

    sh_memory = (SH_MEMORY *) shmat(sh_id, NULL, 0);
    if (sh_memory == (void *) -1) {
        fprintf(stderr,"Problem with shmemory\n");
        sh_mem_cleanup(sh_id, sh_memory);
        fclose(file);
        return EXIT_FAILURE;
    }

    // Initialize shared memory
    sh_memory->counter = 0;
    sh_memory->NO = 0;
    sh_memory->NH = 0;
    sh_memory->NoM = 1;
    sh_memory->H20 = molecule_count((args.NH / 2), args.NO);
    sh_memory->cntO = 0;
    sh_memory->cntH = 0;
    sh_memory->cnt = 0;
 
    // Semaphore setup
    SEMAPHORE sem;
    sem.oxygen = sem_open("/xbehal02_sem_oxygen", O_CREAT | O_EXCL, 0666, 0);
    sem.hydrogen = sem_open("/xbehal02_sem_hydrogen", O_CREAT | O_EXCL, 0666, 0);
    sem.mutex = sem_open("/xbehal02_sem_mutex", O_CREAT | O_EXCL, 0666, 1);
    sem.barrier1 = sem_open("/xbehal02_sem_barrier1", O_CREAT | O_EXCL, 0666, 0);
    sem.barrier2 = sem_open("/xbehal02_sem_barrier2", O_CREAT | O_EXCL, 0666, 1);
    sem.bondH = sem_open("/xbehal02_sem_bondH", O_CREAT | O_EXCL, 0666, 0);
    sem.molecule = sem_open("/xbehal02_sem_molecule", O_CREAT | O_EXCL, 0666, 1);
    sem.print = sem_open("/xbehal02_sem_print", O_CREAT | O_EXCL, 0666, 1);

    //Check for semaphore errors
    if (sem.oxygen == SEM_FAILED || sem.hydrogen == SEM_FAILED || sem.mutex == SEM_FAILED || sem.barrier1 == SEM_FAILED || sem.barrier2 == SEM_FAILED || sem.bondH == SEM_FAILED || sem.molecule == SEM_FAILED || sem.print == SEM_FAILED) {
        fprintf(stderr,"Problem with semaphore setup\n");
        sem_cleanup(&sem);
        sem_un();
        sh_mem_cleanup(sh_id, sh_memory);
        fclose(file);
        return EXIT_FAILURE;
    }

    pid_t oxygen;
    pid_t hydrogen;

    // For loop that creates hydrogen processes
    for (int idH = 1; idH <= args.NH; idH++)
    {
        hydrogen = fork();
        if (hydrogen == 0){ // Child process
            process_hydrogen(&args, &sem, sh_memory, file, idH);

            sem_cleanup(&sem);
            fclose(file);

            exit(0);
        }
        else if(hydrogen < 0){ // Fork failed
            fprintf(stderr, "Problem fork oxygen\n");

            sh_mem_cleanup(sh_id, sh_memory);
            sem_cleanup(&sem);
            sem_un();

            fclose(file);
            
            exit(0);
        }
    }

    // For loop that creates oxygen processes
    for (int idO = 1; idO <= args.NO; idO++)
    {
        oxygen = fork();
        if (oxygen == 0){ // Child process
            process_oxygen(&args, &sem, sh_memory, file, idO);
            
            sem_cleanup(&sem);
            fclose(file);
            
            exit(0);
        }
        else if(oxygen < 0){ // Fork failed
            fprintf(stderr, "Problem fork oxygen\n");

            sh_mem_cleanup(sh_id, sh_memory);
            sem_cleanup(&sem);
            sem_un();

            fclose(file);
            exit(0);
        }
    }
    
    // Final cleanup
    sh_mem_cleanup(sh_id, sh_memory);
    sem_cleanup(&sem);
    sem_un();
    fclose(file);

    // Waits for all processes to finish
    while(wait(NULL) > 0);

    return EXIT_SUCCESS;
}