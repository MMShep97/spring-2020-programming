#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

// argv[1] = input file
// argv[2] = # of children

struct key_value
{
    int child_num;
    int sorted_indice;
    char * output_pair;
    int filtered_num;
};

// qsort compare
int compare( const void* a, const void* b)
{
     int int_a = * ( (int*) a );
     int int_b = * ( (int*) b );

     if ( int_a == int_b ) return 0;
     else if ( int_a < int_b ) return -1;
     else return 1;
}

int main(int argc, char *argv[], char* env[]) {

    char *p;
    char * file_path = argv[1];
    long num_children = strtol(argv[2], &p, 10);


// --------------------------------------------------------------------------------
// -------------------------- File Processing -------------------------------------
// --------------------------------------------------------------------------------
    int lines_allocated = 128;
    int lines_total = 0;
    int max_line_len = 100;

    /* Allocate line_list of text */
    char **line_list = (char **)malloc(sizeof(char*)*lines_allocated);
    if (line_list==NULL)
        {
        fprintf(stderr,"Out of memory (1).\n");
        exit(1);
        }

    FILE *fp = fopen(file_path, "r");
    if (fp == NULL)
        {
        fprintf(stderr,"Error opening file.\n");
        exit(2);
        }

    int i;
    for (i=0;1;i++)
        {
        int j;

        /* Have we gone over our line allocation? */
        if (i >= lines_allocated)
            {
            int new_size;

            /* Double our allocation and re-allocate */
            new_size = lines_allocated*2;
            line_list = (char **)realloc(line_list,sizeof(char*)*new_size);
            if (line_list==NULL)
                {
                fprintf(stderr,"Out of memory.\n");
                exit(3);
                }
            lines_allocated = new_size;
            }
        /* Allocate space for the next line */
        line_list[i] = malloc(max_line_len);

        if (line_list[i]==NULL)
            {
            fprintf(stderr,"Out of memory (3).\n");
            exit(4);
            }
        if (fgets(line_list[i],max_line_len-1,fp)==NULL)
            break;

        /* Get rid of CR or LF at end of line */
        for (j=strlen(line_list[i])-1;j>=0 && (line_list[i][j]=='\n' || line_list[i][j]=='\r');j--)
            //  printf("line_list %s\n", line_list[i]);
            //  printf("line_list %s\n", line_list[i]);
            ;
        line_list[i][j+1]='\0';
        }
    /* Close file */
    fclose(fp);

    lines_total = i;
    // printf("line_list count: %d\n", lines_total);

    // printf("line_list[0] %s\n", line_list[0]);
    // printf("line_list[1] %s\n", line_list[1]);

    // for (int i = 0; i < lines_total; i++) {
    //     char * copy = strdup(line_list[i]);
    //     char * space = copy;
        
    //     while (space = strchr(space, ' ')) *space = '\n'; 
    //     printf ("test: %s\n", copy);
    // }
    int token_count = 0;
    char * filtered_list[1000];
    int filter_indice = 0;
    struct key_value kv[1000];

    //for every line, filter for second number and store into filtered_list
    for (int i = 0; i < lines_total; i++) {
        char * copied_line = malloc(20);
        strcpy(copied_line, line_list[i]);
        kv[i].output_pair = line_list[i];
        // printf("Copied Line : %s\n", copied_line);
        char * token = strtok(copied_line, " ");
        token_count = 0;
   // loop through the string to extract all other tokens
        while( token != NULL ) {
            if (token_count == 1) {
                // store value
                // printf("extracted: %s\n", token);
                filtered_list[filter_indice] = token;
                kv[filter_indice].filtered_num = atoi(token);
                filter_indice++;
            }
            // printf( " %s\n", token ); //printing each token
            token = strtok(NULL, " ");
            token_count++;
        }
        // printf("new line\n");
    }

    printf("\nFiltered List: \n");

    for (int i = 0; i < lines_total; i++) {
        // printf("%s ", filtered_list[i]);
        printf("%d ", kv[i].filtered_num);
    }

    printf("\nLines List: \n");

    for (int i = 0; i < lines_total; i++) {
        printf("%s \n", kv[i].output_pair);
        // printf("%s \n", line_list[i]);
    }    

    printf("\n");

    // printf("\nSplit\n");

// ------------- Split filtered lines into different parts for child processes ----------------
// https://stackoverflow.com/questions/36526259/split-c-array-into-n-equal-parts

int chunk_size = lines_total / num_children;
int child_number = 0;
for (int i = 0; i < num_children; i++) {
    int start = i * chunk_size;
    int end = start + chunk_size - 1;
    if (i == num_children - 1) {
        end = lines_total - 1;
    }

    for (int i = start; i <= end; i++) {
        // printf("filter: %d ", kv[i].filtered_num);
        kv[i].child_num = child_number;
    }
        child_number++;

        // printf("\n");
}



// --------------------------------------------------------------------------------
// -------------------------- Child Processes -------------------------------------
// --------------------------------------------------------------------------------
    for(int i=0;i<num_children;i++) // loop will run n times 
    { 
        int filter_list[1000];
        int temp_count = 0;
        int current_child_num = i;
        // gets portion of list into correct child processes
        for (int j = 0; j < lines_total; j++) {
            if (kv[j].child_num == current_child_num) {
                filter_list[temp_count] = kv[j].filtered_num;
                kv[j].sorted_indice = temp_count;
                temp_count++;
            }
        }
        // child process creation
        if(fork() == 0) 
        {
            printf("FILTER LIST @ CHILD %d: ", current_child_num);
            for (int c = 0; c < temp_count; c++) {
                printf("%d ", filter_list[c]);
            }

printf("\n");
            printf("INDICE\n");

            for (int b = 0; b < lines_total; b++) {
                printf("%d ", kv[b].sorted_indice);
            }

            // https://stackoverflow.com/questions/3893937/sorting-an-array-in-c
            qsort(filter_list, temp_count, sizeof(int), compare );

            printf("FILTER LIST --SORTED-- @ CHILD %d: ", current_child_num);
            for (int c = 0; c < temp_count; c++) {
                printf("%d ", filter_list[c]);
            }



            printf("\n");
            printf("[son] pid %d from [parent] pid %d\n",getpid(),getppid()); 
            exit(0); 
        } 
    } 
    for(int i=0;i<num_children;i++) {// loop will run n times 
        wait(NULL); 
    }

    return 0;
}

