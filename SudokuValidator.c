#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <omp.h>

#define ROW 9
#define COLUMN 9
#define true 1

#define SUBMATRIX_SIZE 9
#define SUBMATRIX_ROWS 9
#define SUBMATRIX_COLS 9

char sudoku_board[ROW][COLUMN];
int vc, vr;

void load_sudoku_board(int file_descriptor) {

    // Enable nested parallelism
    omp_set_nested(true);

    // Set number of threads to 9
    omp_set_num_threads(9);

    // Get file information, including size
    struct stat file_info;
    int status = fstat(file_descriptor, &file_info);
    int file_size = file_info.st_size;

    // Map the file data to a character array
    char *file_data = (char *) mmap(0, file_size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);

    // Load the file data into a 2D array representing the sudoku_board board
    int current_index = 0;
    int current_row[9];
    #pragma omp parallel for private(current_row) //schedule(dynamic)
    for(int i = 0; i < ROW; i++){
        #pragma omp parallel for
        for(int j = 0; j < COLUMN; j++){
            sudoku_board[i][j] = file_data[current_index];
            current_index++;
        }
    }
}


int verify_rows() {

    // Enable nested parallelism
    omp_set_nested(true);

    // Set number of threads to 9
    omp_set_num_threads(9);

    // Array to hold the current row being verified
    int current_row[9];

    // Flag to indicate whether all rows are valid
    int all_rows_valid = 1;

    // Verify each row
    #pragma omp parallel for private(current_row) //schedule(dynamic)
    for(int i = 0; i < ROW; i++) {

        // String of numbers to verify against
        char numbers_to_verify[] = "123456789";

        // Pointer to current character in the string
        char* current_number;

        // Check that each number appears exactly once in the current row
        for(current_number = &numbers_to_verify[0]; *current_number != '\0'; current_number++) {
            int found = 0;
            int counter = 0;
            while(found == 0 && counter < COLUMN){
                if(sudoku_board[i][counter] == *current_number) found = 1;
                counter++;
            }
            if(found == 0) all_rows_valid = 0;
        }
    }

    // Return -1 if any row is invalid, 1 if all rows are valid
    return all_rows_valid ? 1 : -1;
}

int verify_rows_args(char temp[ROW][COLUMN]) {

    // Enable nested parallelism
    omp_set_nested(true);

    // Set number of threads to 9
    omp_set_num_threads(9);

    // Array to hold the current row being verified
    int current_row[9];

    // Flag to indicate whether all rows are valid
    int all_rows_valid = 1;

    // Verify each row
    #pragma omp parallel for private(current_row) //schedule(dynamic)
    for(int i = 0; i < ROW; i++) {

        // String of numbers to verify against
        char numbers_to_verify[] = "123456789";

        // Pointer to current character in the string
        char* current_number;

        // Check that each number appears exactly once in the current row
        for(current_number = &numbers_to_verify[0]; *current_number != '\0'; current_number++) {
            int found = 0;
            int counter = 0;
            while(found == 0 && counter < COLUMN){
                if(temp[i][counter] == *current_number) found = 1;
                counter++;
            }
            if(found == 0) all_rows_valid = 0;
        }
    }

    // Return -1 if any row is invalid, 1 if all rows are valid
    return all_rows_valid ? 1 : -1;
}


int verify_columns() {

    // Enable nested parallelism
    omp_set_nested(true);

    // Set number of threads to 9
    omp_set_num_threads(9);

    // Array to hold the current row being verified
    int current_col[9];

    // Flag to indicate whether all rows are valid
    int all_cols_valid = 1;

    // Verify each col
    #pragma omp parallel for private(current_col) //schedule(dynamic)
    for(int i = 0; i < COLUMN; i++) {

        // String of numbers to verify against
        char numbers_to_verify[] = "123456789";

        // Pointer to current character in the string
        char* current_number;

        // Check that each number appears exactly once in the current col
        for(current_number = &numbers_to_verify[0]; *current_number != '\0'; current_number++) {
            int found = 0;
            int counter = 0;
            while(found == 0 && counter < ROW){
                if(sudoku_board[counter][i] == *current_number) found = 1;
                counter++;
            }
            if(found == 0) all_cols_valid = 0;
        }
    }

    // Return -1 if any row is invalid, 1 if all rows are valid
    return all_cols_valid ? 1 : -1;
}

int verify_sub_matrix() {

    omp_set_nested(true);
    omp_set_num_threads(3);

    char submatrix[SUBMATRIX_SIZE][SUBMATRIX_SIZE];
    int row_counter = 0, col_counter = 0;
    int a[SUBMATRIX_SIZE];
    #pragma omp parallel for private(a) //schedule(dynamic)
    for(int submatrix_row=0; submatrix_row<SUBMATRIX_ROWS; submatrix_row++) {
        #pragma omp parallel for
        for(int submatrix_col=0; submatrix_col<SUBMATRIX_COLS; submatrix_col++) {
            #pragma omp parallel for
            for(int row=0; row<SUBMATRIX_SIZE; row++) {
                #pragma omp parallel for 
                for(int col=0; col<SUBMATRIX_SIZE; col++) {
                    submatrix[row_counter][col_counter] = sudoku_board[row+(submatrix_row*SUBMATRIX_SIZE)][col+(submatrix_col*SUBMATRIX_SIZE)];
                    col_counter++;
                }
            }
            col_counter = 0;
            row_counter++;
        }
    }
    return verify_rows_args(submatrix);
}


void *vcol_runner() {
    printf("vcol thread id: %ld\n", syscall(SYS_gettid));
    vc = verify_columns();
    pthread_exit(0);
}

void *vrow_runner() {
    printf("vrow thread id: %ld\n", syscall(SYS_gettid));
    vr = verify_rows();
    pthread_exit(0);
}

int main(int argc, char *argv[]) {

    omp_set_num_threads(1);

    
    /* Verify sudoku_board file is being passed */
    if(argc < 2) {
        printf("No sudoku_board file was passed"); return 1;
    }

    /* Load sudoku_board to memory */
    int input;
    if( (input = open(argv[1], O_RDONLY) ) < 0) {
		perror("Error opening source file");
		return 1;
	} else {
        load_sudoku_board(input);

        /* Get pid info*/
        pid_t parent_pid = getpid();
        
        int f = fork();
        if(f < 0) {
            perror("Error forking!");
            return 1;
        } else if(f == 0) { // Child process
            
            /* Convert pid to string */
            char p_pid[6];
            sprintf(p_pid, "%d", (int) parent_pid);
            execlp("ps","ps","-p", p_pid, "-lLf", NULL);

        } else { // Parent process

            /* Create and join column verify thread */
            pthread_t ver_cols;
            if(pthread_create(&ver_cols, NULL, vcol_runner, NULL)){ perror("Error creating thread"); return 1;}
            if(pthread_join(ver_cols, NULL)) { perror("Error joining thread"); return 1; }

            printf("main thread: %ld\n", syscall(SYS_gettid));
            printf("waiting for child\n");
            usleep(30000);
            printf("children finished\n");

            /* Create and join row verify thread */
            pthread_t ver_row;
            if(pthread_create(&ver_row, NULL, vrow_runner, NULL)){ perror("Error creating thread"); return 1;}
            if(pthread_join(ver_row, NULL)) { perror("Error joining thread"); return 1; }

            /* Display if solution is valid or not */
            if(vr == 0 && vc == 0) {
                printf("--- * Valid solution * ---\n");
            } else {
                printf("--- * Invalid solution * ---\n");
            }
        }

        /* second fork */
        int ff = fork();
        if(ff == 0) { // Child process
            /* Convert pid to string */
            char p_pid[6];
            sprintf(p_pid, "%d", (int) parent_pid);
            execlp("ps","ps","-p", p_pid, "-lLf", NULL);
        } else {
            printf("waiting child\n");
            usleep(30000);
            printf("children finished\n");
            return 0;
        }
    }
}