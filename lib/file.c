#include "file.h"

#define N_ALLOC 2048

/* Returns the file extension
*/
const char *get_filename_ext(const char *filename) {                                                  
  const char *dot = strrchr(filename, '.');                                                         
  if(!dot || dot == filename) return "";                                                            
  return dot + 1;                                                                                   
}   

/*******************************************************************/

/* Changes to a working directory, where everything will be read
 * from and written to
 */ 
int_t file_chdir(char* dir){

  char* oldwd = getcwd(NULL,0);
  if (!oldwd) die(__func__);
  if (chdir(dir) == -1) die(__func__);

  free(oldwd);
  return 0;
}

/*******************************************************************/

//Open a file and returns a pointer
FILE* file_open(char *c_file, const char * c_mode){

  FILE* f_in;

  f_in = fopen(c_file, c_mode);
  if (!f_in) perror ("file_open");

  fseek(f_in, 0, SEEK_SET);

  return f_in;
}


int_t file_close(FILE* f_in){

  fclose(f_in);
  if (!f_in) perror ("file_close");

  return 0;
}

/*******************************************************************/

size_t file_size(FILE* f_in){

  fseek(f_in, 0, SEEK_END);
  size_t length = ftell(f_in);
  rewind(f_in);

  return length;
}

/*******************************************************************/

uint_t file_read(FILE* f_in){

  uint_t value;

  fread(&value, sizeof(uint_t), 1, f_in);

  //	printf("read = %" PRIdN"\n", value);

  return value;
}

/*******************************************************************/

int_t file_write(FILE* f_out, uint_t value){

  //	printf("write(%" PRIdN")\n", value);
  fwrite(&value, sizeof(uint_t), 1, f_out);

  return 0;
}

/*******************************************************************/

char* file_load(FILE* f_in) {

  size_t len = 0;
  ssize_t size = 0;
  char *c_aux = NULL;

  /*Read one line*/
  size = getline(&c_aux, &len, f_in);
  if (size == -1) perror("file_load");

  /*Copy line to c_buffer*/
  char *c_buffer = (char*) malloc((size+1)*sizeof(char));
  strncpy(c_buffer, c_aux, size);

  c_buffer[size-1] = END_MARKER;
  c_buffer[size] = '\0';

  free(c_aux);

  return c_buffer;
}

/*******************************************************************/

// read line by line
char** load_multiple_txt(char *c_file, int_t *k, size_t *n) {

  FILE* f_in = file_open(c_file, "rb");
  if(!f_in){
    fprintf (stderr, "file_open of '%s' failed: %s.\n", c_file, strerror (errno));
    exit (EXIT_FAILURE);
  }

  int_t n_alloc = N_ALLOC;
  char **c_buffer = (char**) malloc(n_alloc*sizeof(char*));

  int_t i;
  for(i=0; i<*k; i++){

    size_t len = 0; c_buffer[i] = NULL;		
    ssize_t size = getline(&c_buffer[i], &len, f_in);
    if (size == -1){
      *k = i;
      free(c_buffer[i]);
      break;		
    }
    c_buffer[i][size-1] = 0;

    (*n) += size;

    if(i==n_alloc-1){
      n_alloc+=N_ALLOC;
      c_buffer = (char**) realloc(c_buffer, n_alloc*sizeof(char*));
    }
  }
  fclose(f_in);

  return c_buffer;
}

// read sequences separeted by '>' line

char** load_multiple_fasta(char *c_file, int_t *k, size_t *n){

  FILE* f_in = file_open(c_file, "rb");
  if(!f_in){
    fprintf (stderr, "file_open of '%s' failed: %s.\n", c_file, strerror (errno));
    exit (EXIT_FAILURE);
  }

  int_t n_alloc = N_ALLOC;
  char **c_buffer = (char**) malloc(n_alloc*sizeof(char*));

  char *buf = NULL;
  size_t len = 0;

  ssize_t size = getline(&buf, &len, f_in);// first sequence
  if (size == -1) perror("file_load");
  free(buf);

  int_t count=0;
  int_t i;
  for(i=0; i<*k; i++){

    if(i!=count){
      *k = i;
      break;		
    }

    int_t c_alloc = N_ALLOC;
    c_buffer[i] = (char*) malloc(c_alloc*sizeof(char));

    size_t p=0;
    len = 0; buf=NULL;
    while(getline(&buf, &len, f_in)!=-1){

      if(buf[0] == '>'){
        count++;
        break;
      }

      if(p+len>c_alloc){
        c_alloc += len+N_ALLOC;
        c_buffer[i] = (char*) realloc(c_buffer[i], sizeof(char) * c_alloc);
      }

      strcpy(&c_buffer[i][p], buf);
      p+=strlen(buf)-1;

      free(buf);
      buf=NULL; len=0;
    }

    free(buf);
    c_buffer[i][p] = 0;
    (*n) += p+1;

    if(i==n_alloc-1){
      n_alloc+=N_ALLOC;
      c_buffer = (char**) realloc(c_buffer, n_alloc*sizeof(char*));
    }
  }
  fclose(f_in);

  return c_buffer;
}

/*******************************************************************/

char** file_load_multiple(char* c_file, int_t *k, size_t *n) {

  /* .ext
   * .txt   - strings per line
   * .fasta - strings separated by '>' line
   */

  const char *type = get_filename_ext(c_file);

  char **c_buffer = NULL; // = (char**) malloc(k*sizeof(char*));
  if(*k==0) *k=I_MAX;

  if(strcmp(type,"txt") == 0){
    c_buffer = load_multiple_txt(c_file, k, n);
  }
  else if(strcmp(type,"fasta") == 0){
    c_buffer = load_multiple_fasta(c_file, k, n);
  }
  else{
    printf("Error: file not recognized (.txt, .fasta, .fastq)\n");
    return 0;
  }

  return c_buffer;
}

/**********************************************************************/

void mkdir(const char* c_file){

  char c_aux[500];

  strcpy (c_aux,"mkdir -p ");
  strcat (c_aux, c_file);

  system (c_aux);//remove .bin

}

/**********************************************************************/

int_t file_text_write(unsigned char *str, int_t n, char* c_file, const char* ext){

  FILE *f_out;
  char *c_out = (char*) malloc((strlen(c_file)+strlen(ext)+3)*sizeof(char));

  sprintf(c_out, "%s.%s", c_file, ext);
  f_out = file_open(c_out, "wb");

  int_t i;
  for(i=0; i<n;i++) if(str[i]) str[i]--;

  fwrite(str, sizeof(unsigned char), n, f_out);

  file_close(f_out);
  free(c_out);

  return 1;
}

int_t file_text_int_write(int_t *str, int_t n, char* c_file, const char* ext){

  FILE *f_out;
  char *c_out = (char*) malloc((strlen(c_file)+strlen(ext))*sizeof(char));

  sprintf(c_out, "%s.%s", c_file, ext);
  f_out = file_open(c_out, "wb");

  fwrite(str, sizeof(int_t), n, f_out);

  file_close(f_out);
  free(c_out);

  return 1;
}

/*******************************************************************/

int_t file_text_read(unsigned char** str, char* c_file, const char* ext){

  FILE *f_in;
  char *c_in = (char*) malloc((strlen(c_file)+strlen(ext))*sizeof(char));

  sprintf(c_in, "%s.%s", c_file, ext);
  f_in = file_open(c_in, "rb");

  fseek(f_in, 0L, SEEK_END);
  size_t size = ftell(f_in);
  rewind(f_in);

  int_t n = size/sizeof(unsigned char);

  *str = (unsigned char*) malloc(n*sizeof(unsigned char));
  fread(*str, sizeof(unsigned char), n, f_in);

  file_close(f_in);
  free(c_in);

  return n;
}

int_t file_text_int_read(int_t** str_int, char* c_file, const char* ext){

  FILE *f_in;
  char *c_in = (char*) malloc((strlen(c_file)+strlen(ext))*sizeof(char));

  sprintf(c_in, "%s.%s", c_file, ext);
  f_in = file_open(c_in, "rb");

  fseek(f_in, 0L, SEEK_END);
  size_t size = ftell(f_in);
  rewind(f_in);

  int_t n = size/sizeof(int_t);

  *str_int = (int_t*) malloc(n*sizeof(int_t));
  fread(*str_int, sizeof(int_t), n, f_in);

  file_close(f_in);
  free(c_in);

  return n;
}

/*******************************************************************/

int_t file_bwt_write(unsigned char *str, int_t *SA, int_t n, char* c_file, const char* ext){

  FILE *f_out;
  char *c_out = (char*) malloc((strlen(c_file)+strlen(ext)+3)*sizeof(char));

  sprintf(c_out, "%s.%s", c_file, ext);
  f_out = file_open(c_out, "wb");

  int_t i;
  for(i=0; i<n;i++){
    char j = (SA[i])? str[SA[i]-1]:'#';
    if(j==0) j = '$';
    fwrite(&j, sizeof(unsigned char), 1, f_out);
  }

  file_close(f_out);
  free(c_out);

  return 1;
}

int_t file_bwt_int_write(int_t *str, int_t *SA, int_t n, char* c_file, const char* ext){

  FILE *f_out;
  char *c_out = (char*) malloc((strlen(c_file)+strlen(ext))*sizeof(char));

  sprintf(c_out, "%s.%s", c_file, ext);
  f_out = file_open(c_out, "wb");

  int_t i;
  for(i=0; i<n;i++){
    int_t j = (SA[i])? str[SA[i]-1]:0;
    fwrite(&j, sizeof(int_t), 1, f_out);
  }

  file_close(f_out);
  free(c_out);

  return 1;
}


/*******************************************************************/
