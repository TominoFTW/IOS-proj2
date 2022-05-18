//sctructures used in the program
typedef struct semaphore_t{
    sem_t *oxygen;
    sem_t *hydrogen;
    sem_t *mutex;
    sem_t *barrier1;
    sem_t *barrier2;
    sem_t *bondH;
    sem_t *molecule;
    sem_t *print;
}SEMAPHORE;

typedef struct sh_memory_t{
    int counter;
    int NO;
    int NH;
    int H20;
    int NoM;
    int cntO;
    int cntH;
    int cnt;
    bool molecule;
}SH_MEMORY;

typedef struct args_t{
    int NO;
    int NH;
    int TI;
    int TB;
}ARGS;

/**
 * @brief Checks if given arguments are valid
 * 
 * @param argc argument count
 * @param argv arguments
 * @param args pointer to structure where arguments will be stored
 */
bool args_valid(int argc, char *argv[], ARGS *args);


/**
 * @brief Oxygen producer
 * 
 * @param args arguments from command line
 * @param sem semaphore struct
 * @param sh_memory shared memory struct
 * @param file file
 * @param idO oxygen id
 */
void use_oxygen(ARGS *args, SEMAPHORE *sem, SH_MEMORY *sh_memory, FILE *file, int idO);


/**
 * @brief Hydrogen producer
 * 
 * @param args arguments from command line
 * @param sem semaphore struct
 * @param sh_memory shared memory struct
 * @param file file
 * @param idH hydrogen id
 */
void use_hydrogen(ARGS *args, SEMAPHORE *sem, SH_MEMORY *sh_memory, FILE *file, int idH);