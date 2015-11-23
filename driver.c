#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <mpi.h>
#include <time.h>

#define MAXGV 100000
#define MX 1000
#define MX200 200
#define MAX_SLAVE_THREADS 1000

static const char MAIN_LOG_FLNAME[] = "main.log";
static const char GEN_SMT_FL_NAME[] = "auto.";
static const char SMT_EXTENSION[] = ".smt2";
static const char SOLVER_OUTPUT_EXTENSION[] = ".out.txt";
static const char UNIX_DIR_SEPARATOR[] = "/";

static const char BITVECSZ[] = "32";
//static const char BITVECSZ[] = "16";

//these are command line arguments
static int DISTANCE_ERROR_UPPER_BOUND;
static int DISTANCE_ERROR_LOWER_BOUND;
static int DISTANCE_ERROR_INCREMENT; 
static int POINT_COORDINATES_UPPER_BOUND;

//1 if 3 anchor points provided as command line arguments
//0 otherwise
static int HAS_ANCHOR_POINTS;
static int anchorp1x;
static int anchorp1y;
static int anchorp2x;
static int anchorp2y;
static int anchorp3x;
static int anchorp3y;


char* generate_smt_output_filename(char* logdirname, int delvalue);
void parse_all_files(int numprocs, int numpoints, int distance_error_lower_bound, int distance_error_increment, FILE* fp_logfile, char* tstampdirname);
char* get_distortions(char** pts, int num_delta_distances, int max_distortion_val);
char* remove_char_from_str(char* str, char c);
long print_number_from_smt_string(char* a_point);
char*  get_points(char** pts, int num_points);
void display_array_of_strings(char** x, int numberofpoints, char* displayheader);
int start_point_checker(char* s);
int end_point_checker(char* s);
char* parsefile(char* flname, int numberofpoints, int error_value);
char* int_2_32bithex(int n);
char* int_2_16bithex(int n);
int findspaces(char *inp);
char* padstr(char* ins);
void gencode(double** d, int sz, char* file_location, int current_allowed_error, int has_anchor_pts);
void print_array(double** d, int sz);
void remove_spaces(char* str);
char* gen_timestamp_flname(char* str);

int* thread_delta_table;

int main(int argc, char* argv[]) {
  int rank;
  int master = 0; //master process
  int num_procs;

  MPI_Init(&argc, &argv);      
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);        
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs);     

  //number of points to be generated
  int num_points;

  //pairwise distance between points are taken as input
  double** d; 

  //full path for log file
  char full_main_log_path[MX200];

  if (rank == master) {
    char tstampdirname[MX200];
    FILE* main_log_fp;
    
    if ( (argc == 7) || (argc == 13)) {
      if (argc == 7) {
	HAS_ANCHOR_POINTS = 0;
	//printf("No anchor pts\n");
      }
      else {
	HAS_ANCHOR_POINTS = 1;
	
	anchorp1x = atoi(argv[7]);
	anchorp1y = atoi(argv[8]);
	anchorp2x = atoi(argv[9]);
	anchorp2y = atoi(argv[10]);
	anchorp3x = atoi(argv[11]);
	anchorp3y = atoi(argv[12]);
	//printf("driver recd anchor pts: [(%d, %d), (%d, %d), (%d, %d)]\n", anchorp1x, anchorp1y, anchorp2x, anchorp2y, anchorp3x, anchorp3y);
      }

      //exit(1);

      DISTANCE_ERROR_LOWER_BOUND = atoi(argv[2]);
      DISTANCE_ERROR_UPPER_BOUND = atoi(argv[3]);
      DISTANCE_ERROR_INCREMENT = atoi(argv[4]);
      POINT_COORDINATES_UPPER_BOUND = atoi(argv[5]);

      
      FILE *infp;
      infp = fopen(argv[1], "r");
    
      if (infp == NULL) {
	fprintf(stderr, "Could not open file %s. Quitting.\n", argv[1]);
	exit(1);
      }

      strcpy(tstampdirname, argv[6]);
	
      //char* full_main_log_path = malloc(sizeof(char) * (strlen(tstampdirname)+strlen(MAIN_LOG_FLNAME)+5));
      strcpy(full_main_log_path, tstampdirname);
      strcat(full_main_log_path, UNIX_DIR_SEPARATOR);
      strcat(full_main_log_path, MAIN_LOG_FLNAME);

      main_log_fp = fopen(full_main_log_path, "w+");
      if (main_log_fp == NULL) {
	fprintf(stderr, "Could not open file %s. Quitting.\n", argv[1]);
	exit(1);	 
      } else {
	fprintf(main_log_fp, "Input file: %s\n", argv[1]);
	fprintf(main_log_fp, "SMT code bitvector size: %s.\n", BITVECSZ);
	fprintf(main_log_fp, "Coordinate space: (0,0) to (%d,%d).\n\n", POINT_COORDINATES_UPPER_BOUND, POINT_COORDINATES_UPPER_BOUND);
      }

      //broadcast names to all slaves
      int tstampdir_sz = strlen(tstampdirname)+1;
      MPI_Bcast(&tstampdir_sz, 1, MPI_INT, master, MPI_COMM_WORLD);
      MPI_Bcast(tstampdirname, tstampdir_sz, MPI_CHAR, master, MPI_COMM_WORLD);

      //copy matrix distance file into the newly created log directory
      char* copyfilecmd =  malloc( sizeof(char) * strlen(tstampdirname) + strlen(argv[1]) + 5); //TODO remove magiv number
      sprintf(copyfilecmd, "cp %s %s", argv[1], tstampdirname);
      system(copyfilecmd);
  
      fscanf(infp, "%d", &num_points);
      //printf("[pfromd]: Will try to find %d points.\n", num_points);


    
      d = malloc(num_points * sizeof(double));
      for (int i = 0; i < num_points; i++) {
	d[i] = malloc(num_points * sizeof(double));
	for (int j = 0; j < num_points; j++) {
	  fscanf(infp, "%lf", &d[i][j]);
	}
      }
    

      //actually (num_procs-1) slaves, but we let the 0th entry be -1 to avoid confusion      
      thread_delta_table = malloc(sizeof(int) * num_procs);       
      thread_delta_table[0] = -1;
    } else {
      fprintf(stderr, "Usage: ./driver file_name distance_min_error distance_max_error distance_error_increments log_directory_location. Quitting [p1.x p1.y p2.x p2.y p3.x p3.y].\n");
      exit(1);
    }

    //print_array(d, num_points);
    //printf("quitting\n");

    for (int del_vals = DISTANCE_ERROR_LOWER_BOUND; del_vals <= DISTANCE_ERROR_UPPER_BOUND; del_vals += DISTANCE_ERROR_INCREMENT) {
      char* full_file_path = malloc(sizeof(char) * (strlen(tstampdirname) + strlen(GEN_SMT_FL_NAME) + 50)); //TODO remove magic number
      strcpy(full_file_path, "");
      strcat(full_file_path, tstampdirname);
      strcat(full_file_path, UNIX_DIR_SEPARATOR);
      strcat(full_file_path, GEN_SMT_FL_NAME);

      char* del_val_as_str = malloc(sizeof(char) * 20); //TODO: remove magic number
      sprintf(del_val_as_str, "%d", del_vals);

      strcat(full_file_path, del_val_as_str);
      strcat(full_file_path, SMT_EXTENSION);

      //printf("will gen file with name %s\n", full_file_path);
      //printf("\nshould call gen code here (%d, %s, %d, %d)\n", num_points, full_file_path, del_vals, HAS_ANCHOR_POINTS);
      gencode(d, num_points, full_file_path, del_vals, HAS_ANCHOR_POINTS);
      //printf("gencode done for del value = %d\n", del_vals);
    }
    //printf("all gencodes done.\n");
    //exit(1);
      
    //printf("all gencodes worked, but quitting\n");
    //exit(1);

    int ready_del_value = 1;
    MPI_Bcast(&ready_del_value, 1, MPI_INT, master, MPI_COMM_WORLD); 

    //printf("Master will now inform slaves about deltas.\n"); !!IMP!!
    //printf("Number of slave threads = %d\n", num_procs-1); !!IMP!!
    int flcount = 0;
    for (int i = 1; i <= num_procs-1; i++) {
      flcount++;
    }
    //printf("number of files = %d.\n", flcount);

    //slaves will be told which file to invoke external program (z3) on
    int del_value_4_flname = DISTANCE_ERROR_LOWER_BOUND;
    for (int i = 1; i <= num_procs-1; i++) {
      thread_delta_table[i] = del_value_4_flname;
      MPI_Send(&del_value_4_flname, 1, MPI_INT, i, master, MPI_COMM_WORLD);
      del_value_4_flname += DISTANCE_ERROR_INCREMENT;
    }

    //printf("[MASTER]: READY FOR RECEIVING SLAVE NOTICES.\n");
    int rank_of_slave_process; 
    MPI_Status rstatus;
    for (int k = 1; k <= num_procs-1; k++) {
      MPI_Recv(&rank_of_slave_process, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &rstatus);

      char output_flname_from_slave[500];
      MPI_Recv(output_flname_from_slave, 500, MPI_CHAR, rank_of_slave_process, MPI_ANY_TAG, MPI_COMM_WORLD, &rstatus);
      //printf("[master]: recd. completion msg. for file %s from slave %d.\n", output_flname_from_slave, rank_of_slave_process);
      
      //TODO can parse the file reported by slave
      char* parse_ret_val = parsefile(output_flname_from_slave, num_points, thread_delta_table[rank_of_slave_process]);      
      //printf("[diag] on demand from %d: %s\n", rank_of_slave_process, parse_ret_val);

      if (parse_ret_val != NULL) {
	fprintf(main_log_fp, "%s\n\n", parse_ret_val);
	fflush(main_log_fp);
      } else {
	fprintf(main_log_fp, "[pointfinfer] Parsing file %s failed.\n\n", output_flname_from_slave);
      }
    }
    printf("[pointfinder] master: All slaves have terminated, will parse z3 output now.\n");

    //parse_all_files(num_procs, num_points, DISTANCE_ERROR_LOWER_BOUND, DISTANCE_ERROR_INCREMENT, main_log_fp, tstampdirname);
  } else {
    int rec_dnamesize;
    MPI_Bcast(&rec_dnamesize, 1, MPI_INT, master, MPI_COMM_WORLD);
    char* rec_dirname = malloc(sizeof(char) * rec_dnamesize);
    MPI_Bcast(rec_dirname, rec_dnamesize, MPI_CHAR, master, MPI_COMM_WORLD);
    //printf("(slave) thread %d received %s.\n", rank, rec_dirname);

    //files need to be prepared by master's call to gencode()
    int rec_file_ready;
    MPI_Bcast(&rec_file_ready, 1, MPI_INT, master, MPI_COMM_WORLD);

    char* exec_fl = malloc(sizeof(char) * (rec_dnamesize + strlen(GEN_SMT_FL_NAME) + strlen(SMT_EXTENSION) + 10)); //remove magic number
    strcpy(exec_fl, "");
    strcat(exec_fl, rec_dirname);
    strcat(exec_fl, UNIX_DIR_SEPARATOR);
    strcat(exec_fl, GEN_SMT_FL_NAME);
    
    //IMP: need as many (slave) threads as auto*.smt2 generaed by master.
    //char* rank_minus_one_as_str = malloc(sizeof(char) * 10); //TODO remove magic number
    //sprintf(rank_minus_one_as_str, "%d", rank-1);  
    //strcat(exec_fl, rank_minus_one_as_str);

    int rec_del_value;
    MPI_Status rstatus;
    MPI_Recv(&rec_del_value, 1, MPI_INT, master, MPI_ANY_TAG, MPI_COMM_WORLD, &rstatus);
    

    char* rec_delval_as_str = malloc(sizeof(char) * 10); //TODO remove magic number
    sprintf(rec_delval_as_str, "%d", rec_del_value);
    strcat(exec_fl, rec_delval_as_str);
    strcat(exec_fl, SMT_EXTENSION);
    
    char* solver_output_fl = malloc(sizeof(char) + (rec_dnamesize + strlen(GEN_SMT_FL_NAME) + strlen(SMT_EXTENSION) + 10)); //TODO remove magic number
    strcpy(solver_output_fl, "");
    strcat(solver_output_fl, rec_dirname);
    strcat(solver_output_fl, UNIX_DIR_SEPARATOR);
    strcat(solver_output_fl, GEN_SMT_FL_NAME);
    //strcat(solver_output_fl, rank_minus_one_as_str);
    strcat(solver_output_fl, rec_delval_as_str);
    strcat(solver_output_fl, SOLVER_OUTPUT_EXTENSION);

    //master does not invoke constraint solver
    //printf("(slave) thread %d will execute file %s and write to %s.\n", rank, exec_fl, solver_output_fl); 
 
    //invoke solver
    char str[MAXGV];

    sprintf(str, "z3 %s > %s", exec_fl, solver_output_fl);
    //sprintf(str, "ls -alt > %s", solver_output_fl);

    //printf("[thread %d]: invoking Z3 on %s.\n", rank, exec_fl);
    //printf("[thread %d]: invoking Z3 on %s, output to %s.\n", rank, exec_fl, solver_output_fl);

    int retstatus = system(str);
    //printf("[thread %d]: system call return status = %d\n", rank, retstatus);

    if (retstatus == -1) {
      printf("[pointfinder] master: (thread %d) Error on invoking system command %s.\n", rank, str);
      exit(1);
    } else {
      //see http://linux.die.net/man/2/wait 
      do  {	
	if (WIFEXITED(retstatus)) {
	  //printf("[thread %d]: z3 exited with status %d.\n", rank, WEXITSTATUS(retstatus));

	  
	  /* if (WEXITSTATUS(retstatus) != 0) { */
	  /*   printf("[thread %d]: non-zero exit from z3. quitting.\n", rank); */
	  /*   exit(1); */
	  /* } */
	  
	  int myrank = rank;
	  MPI_Send(&myrank, 1, MPI_INT, master, rank, MPI_COMM_WORLD);
	  MPI_Send(solver_output_fl, strlen(solver_output_fl)+1, MPI_CHAR, master, rank, MPI_COMM_WORLD);
	}	
	else if (WIFSIGNALED(retstatus)) {
	  printf("[thread %d] z3 killed by signal %d\n", rank, WTERMSIG(retstatus));
	} else if (WIFSTOPPED(retstatus)) {
	  printf("[thread %d] z3 by signal %d\n", rank, WSTOPSIG(retstatus));
	} else if (WIFCONTINUED(retstatus)) {
	  printf("[thread %d] z3 continued\n", rank);
	}
      } while (!WIFEXITED(retstatus) && !WIFSIGNALED(retstatus));
    }  
  }

  if (rank == master) {
    printf("[pointfinder] master: all ouput written to %s.\n", full_main_log_path); 
  }

  MPI_Finalize();


  return 0;
}

char* generate_smt_output_filename(char* logdirname, int delvalue) {
  char* out_flname = malloc(sizeof(char) * (strlen(logdirname) + strlen(GEN_SMT_FL_NAME) + strlen(SOLVER_OUTPUT_EXTENSION) + 10) ); //TODO remove magic number
  strcpy(out_flname, "");
  strcat(out_flname, logdirname);
  strcat(out_flname, UNIX_DIR_SEPARATOR);
  strcat(out_flname, GEN_SMT_FL_NAME);
  
  char* del_as_str = malloc(sizeof(char) * 10); //remove magic number
  sprintf(del_as_str, "%d", delvalue);
  strcat(out_flname, del_as_str);
  
  strcat(out_flname, SOLVER_OUTPUT_EXTENSION);
  return out_flname;
}

void parse_all_files(int numprocs, int numpoints, int distance_error_lower_bound, int distance_error_increment, FILE* fp_logfile, char* tstampdirname) {
  int del_file_to_read = distance_error_lower_bound;
  for (int i = 1; i <= numprocs-1; i++) {
    char* out_flname = generate_smt_output_filename(tstampdirname, del_file_to_read);

    char* parse_ret_val = parsefile(out_flname, numpoints, del_file_to_read);
    if (parse_ret_val != NULL) {
	//printf("[master] successfully parsed %s.\n", out_flname);
      fprintf(fp_logfile, "%s\n", parse_ret_val);
    } else {
      fprintf(fp_logfile, "[pointfinder] parsing %s failed.\n", out_flname);
      }
    del_file_to_read += distance_error_increment;
  }
}

char* int_2_16bithex(int n) {
  char hexstr[MX];
  sprintf(hexstr, "%4x", n);

  char* paddedhexstr = malloc(sizeof(char) * (strlen(hexstr) + 2+1));
  sprintf(paddedhexstr, "#x%s", padstr(hexstr));
  //printf("%s",  paddedhexstr);
  
  return paddedhexstr;  
}

char* int_2_32bithex(int n) {
  char hexstr[MX];
  sprintf(hexstr, "%8x", n);

  char* paddedhexstr = malloc(sizeof(char) * (strlen(hexstr) + 2+1));
  sprintf(paddedhexstr, "#x%s", padstr(hexstr));
  //printf("%s",  paddedhexstr);

  return paddedhexstr;
}


//TODO use system to check if file exists
char* parsefile(char* flname, int numberofpoints, int error_value) {
  FILE* fp;
  fp = fopen(flname, "r");

  if (fp == NULL) {
    printf("[master] could not open %s for parsing.\n", flname);
    return NULL;    
  }
  
  char sat_or_not[10]; //remove magic number
  if ( fscanf(fp, "%s\n", sat_or_not) != 1)  {
    printf("[master] error in parsing file %s.\n", flname);
    return NULL;
  }

  if (strcmp(sat_or_not, "unsat") == 0) {  
    char* temp_str = malloc(sizeof(char) * 500); //remove magic number
  
    //printf("UNSAT for %s\n", flname);    
    //fprintf(stderr, "UNSAT for %s\n", flname);    
    //fprintf(logfp, "UNSAT for max. distortion = %d. (%s).\n", error_value, flname);    
    //exit(1);
    sprintf(temp_str, "UNSAT for max. distortion = %d.", error_value);

    return temp_str;
  }

  int parse_state = 1;
  char answers[1000]; //remove magic number
 
  int num_points_parsed = 0;
  char* all_points_parsed[2*numberofpoints];
  //all_points_parsed = malloc(sizeof(char) * 2*numberofpoints); //2 coordinates

  //reading point values
  while (!feof(fp) && parse_state  < 3) {
    fgets(answers, MX, fp);
    answers[strlen(answers)-1] = '\0';
    //printf("[master] just read: %s\n", answers);

    if (start_point_checker(answers) == 0) 
      parse_state = 2;

    if (parse_state <= 2) {
      all_points_parsed[num_points_parsed] = malloc(sizeof(char) * (strlen(answers) +5));
      strcpy(all_points_parsed[num_points_parsed], answers);
      //printf("[master] storing point %s as %s at index %d.\n", answers, all_points_parsed[num_points_parsed], num_points_parsed);
      num_points_parsed++;
    }
    
    if (end_point_checker(answers) == 0) {
      //finished reading all points
      parse_state = 3;    
    }
  }
  

  char delanswers[1000]; //remove magic number
  int num_dels_parsed = 0;
  int num_of_deltas = (numberofpoints * (numberofpoints-1))/2;
  char* all_dels_parsed[num_of_deltas];

  while (!feof(fp) && parse_state < 5)  {
    fgets(delanswers, MX, fp);
    delanswers[strlen(delanswers)-1] = '\0';
    //printf("[master] just read: %s\n", delanswers);

    if (start_point_checker(delanswers) == 0)
      parse_state = 4;

    if (parse_state <= 4) {
      all_dels_parsed[num_dels_parsed] = malloc(sizeof(char) * (strlen(delanswers) + 5)); //remove magic number
      strcpy(all_dels_parsed[num_dels_parsed], delanswers);
      num_dels_parsed++;
    }
    
    if (end_point_checker(delanswers) == 0) {
      //finished reading all deltas
      parse_state = 5;
    }    
  }

  //char* disp_arg = malloc(sizeof(char) * MX);
  //sprintf(disp_arg, "parsed points: ")
  //display_array_of_strings(all_dels_parsed, num_of_deltas);


  char* generated_pts =  get_points(all_points_parsed, 2*numberofpoints);
  //printf("%s", generated_pts);
  //printf("Generated points: %s", generated_pts);

  char* generated_distortions = get_distortions(all_dels_parsed, num_of_deltas, error_value);  
  //printf("%s", generated_distortions);
  //printf("Generated distortions: %s", generated_distortions);

  //printf("[master] delta reading on file %s complete.\n", flname);

  if (parse_state == 5) {    
    //success
    char* retval = malloc(sizeof(char) * MAXGV);
    strcpy(retval, "");
    strcat(retval, generated_pts);
    retval[strlen(retval)] = ';';
    //strcat(retval, ";");
    strcat(retval, generated_distortions);
    return retval; 
  }
  else  {
    //failure somehow
    //this will never happen so can be removed
    return NULL;
  }
}

char* remove_char_from_str(char* str, char c) {
  char* retstr = malloc(sizeof(str) * (strlen(str)+1));

  for (int i = 0, j = 0; i < strlen(str); i++) {
    if (str[i] != c) 
      retstr[j++] = str[i];

    retstr[j] = '\0'; 
  }
  
  return retstr;
}

long print_number_from_smt_string(char* a_point) {
  char* point_only = malloc(sizeof(char) * (strlen(a_point)+1));
  
  point_only = remove_char_from_str(point_only, ')');
  point_only = remove_char_from_str(point_only, '(');
  point_only = strstr(a_point, "#");
  point_only += 2;  //get rid of "#x"
  
  //printf("Extracted point: (%s), as number: (%ld)\n", point_only, strtol(point_only, NULL, 16)); 
  return strtol(point_only, NULL, 16);
}

//returns -1 on error, 1 if string started with ((, 0 otherwise
int start_point_checker(char* s) {
  if (strlen(s) < 2)
    return -1;

  if (s[0] == '(' && s[1] == '(')
    return 0;
  else 
    return 1;
}

//returns 0 if string ended with )), 1 otherwise
int end_point_checker(char* s) {
  int x = strlen(s);

  if (s[x-1] == ')' && s[x-2] == ')')
    return 0;
  else
    return 1;
}


void gencode(double** d, int sz, char* file_location, int current_allowed_error, int has_anchor_pts) {
  char* zero_as_hexstr = int_2_32bithex(0);
      
  FILE* fp;
  fp = fopen(file_location, "w+");

  if (fp == NULL) {
    fprintf(stderr, "Can't open file. Quitting.\n"); 
    exit(1);
  }

  //printf("gencode: opened file, but will quit.\n");
  //exit(1);

  fprintf(fp, "(set-logic UFBV)\n");
  fprintf(fp, ";(set-option :pp.bv-literals false)\n");
  fprintf(fp, "(set-option :produce-models true)\n");
  fprintf(fp, "\n");

  //declare point constants
  fprintf(fp, ";declaration of points to be discovered\n");
  for (int i = 0; i < sz; i++) {
    fprintf(fp, "(declare-const p%dx (_ BitVec %s))\n", i, BITVECSZ);
    fprintf(fp, "(declare-const p%dy (_ BitVec %s))\n", i, BITVECSZ);
  }
  fprintf(fp, "\n");


  //all points have non-negative coordinates
  //TODO: are we losing things by restricting ourself to integral coordinates
  fprintf(fp, ";all points have non-negative (integral?) coordinates\n");
  for (int i = 0; i < sz; i++) {
    fprintf(fp, "(assert (bvuge p%dx %s))\n", i, zero_as_hexstr);
    fprintf(fp, "(assert (bvuge p%dy %s))\n", i, zero_as_hexstr);
  }
  fprintf(fp, "\n");

  //all points have bounded coordinates
  char* points_upper_bound = int_2_32bithex(POINT_COORDINATES_UPPER_BOUND);
  fprintf(fp, ";all points have bounded coordinates\n");
  for (int i = 0; i < sz; i++) {
    fprintf(fp, "(assert (bvule p%dx %s))\n", i, points_upper_bound); 
    fprintf(fp, "(assert (bvule p%dy %s))\n", i, points_upper_bound); 
  }
  fprintf(fp, "\n");


  //if provided, anchor points are known exactly
  if (has_anchor_pts == 1) {
    fprintf(fp, "; anchor points are known exactly\n");
    fprintf(fp, "(assert (= p0x %s))\n", int_2_32bithex(anchorp1x));
    fprintf(fp, "(assert (= p0y %s))\n", int_2_32bithex(anchorp1y));
    fprintf(fp, "(assert (= p1x %s))\n", int_2_32bithex(anchorp2x));
    fprintf(fp, "(assert (= p1y %s))\n", int_2_32bithex(anchorp2y));
    fprintf(fp, "(assert (= p2x %s))\n", int_2_32bithex(anchorp3x));
    fprintf(fp, "(assert (= p2y %s))\n", int_2_32bithex(anchorp3y));
    fprintf(fp, "\n");
  }

  //declare distance constants
  fprintf(fp, ";declaration of distances between points (note: main diagonal entries will all be zero, and the (square) matrix will be symmetric)\n");
  for (int i = 0; i < sz; i++) {
    for (int j = 0; j < sz; j++) {
      fprintf(fp, "(declare-const dst_p%d_p%d (_ BitVec %s))\n", i, j, BITVECSZ);
    }
  }
  fprintf(fp, "\n");

  //main diagonal entries are always 0
  fprintf(fp, ";main diagonal entries are always zero\n");
  for (int i = 0; i < sz; i++)
    fprintf(fp, "(assert (= dst_p%d_p%d %s))\n", i, i, zero_as_hexstr);
  fprintf(fp, "\n");

  //all (non main diagonal) entries are positive
  fprintf(fp, ";all (non main diagonal) entries are positive\n");
  for (int i = 0; i < sz; i++) {
    for (int j = 0; j < sz; j++) {
      if (i != j)
	fprintf(fp, "(assert (bvugt dst_p%d_p%d %s))\n", i, j, zero_as_hexstr);
    }
  }
  fprintf(fp, "\n");


  //distance matrix is always symmetric (TODO: confirm correctness)
  fprintf(fp, ";distance matrix is always symmetric\n");
  for (int i = 0; i < sz; i++) 
    for (int j = 0; j < sz; j++)  {
      if (i < j)
	fprintf(fp, "(assert (= dst_p%d_p%d dst_p%d_p%d))\n", i, j, j, i);
    }
  fprintf(fp, "\n");

  //manually set distances for the matrix
  fprintf(fp, ";all independent distance matrix entries\n");
  for (int i = 0; i < sz; i++)
    for (int j = 0; j < sz; j++)  {
      if (i < j) {
	int roundval = (int) floor(d[i][j]);
  	fprintf(fp, "(assert (= dst_p%d_p%d %s))\n", i, j, int_2_32bithex(roundval));
      }
    }
  fprintf(fp, "\n");


  //declare pointdist helper function
  fprintf(fp, ";helper function that calculates distance\n");
  fprintf(fp, "(declare-fun pointdist ((_ BitVec %s) (_ BitVec %s) (_ BitVec %s) (_ BitVec %s)) (_ BitVec %s))\n", BITVECSZ, BITVECSZ, BITVECSZ, BITVECSZ, BITVECSZ);
  fprintf(fp, "\n");


  //definition of distance helper function using square of euclidean distance
  /* fprintf(fp, ";definition of distance helper function: the square of euclidean distance\n"); */
  /* fprintf(fp, "(assert (forall ((x1 (_ BitVec %s)) (x2 (_ BitVec %s)) (y1 (_ BitVec %s)) (y2 (_ BitVec %s))) (= (pointdist x1 x2 y1 y2) (bvadd (bvmul (bvsub x2 x1) (bvsub x2 x1)) (bvmul (bvsub y2 y1) (bvsub y2 y1))) ) ))\n", BITVECSZ, BITVECSZ, BITVECSZ, BITVECSZ);   */
  /* fprintf(fp, "\n"); */

  //definition of distance helper function using manhattan distance
  fprintf(fp, ";definition of distance helper function using manhattan distance \n");
  fprintf(fp, "(assert (forall ((x1 (_ BitVec %s)) (x2 (_ BitVec %s)) (y1 (_ BitVec %s)) (y2 (_ BitVec %s))) (= (pointdist x1 x2 y1 y2)   (bvadd  (ite (bvuge x2 x1) (bvsub x2 x1) (bvsub x1 x2)  )  (ite (bvuge y2 y1) (bvsub y2 y1) (bvsub y1 y2)))   ) ) )\n", BITVECSZ, BITVECSZ, BITVECSZ, BITVECSZ);
  fprintf(fp, "\n");


  //distance error relaxation constants
  //need  (n*n - n)/2 constants
  fprintf(fp, ";allowed errors for distances (note: one error variable per independent distance matrix entry\n");
  for (int i = 0; i < sz; i++) 
    for (int j = 0; j < sz; j++)  {
      if (i < j) {
	fprintf(fp, "(declare-const del_p%d_p%d (_ BitVec %s))\n", i, j, BITVECSZ);
	}
    }
  fprintf(fp, "\n");


  //allowed range for errors
  char* error_upper_bound = int_2_32bithex(current_allowed_error);
  fprintf(fp, ";allowed range of errors in distance\n");
  for (int i = 0; i < sz; i++) 
    for (int j = 0; j < sz; j++)  {
      if (i < j) {
	fprintf(fp, "(assert (bvuge del_p%d_p%d %s))\n", i, j, zero_as_hexstr);
	fprintf(fp, "(assert (bvule del_p%d_p%d %s))\n", i, j, error_upper_bound);
	}
    }  
  fprintf(fp, "\n");


  /* //(relaxed) euclidean distance constraints */
  /* fprintf(fp, ";relaxed euclidean distance constraints\n"); */
  /* for (int i = 0; i < sz; i++) */
  /*   for (int j = 0; j < sz; j++)  { */
  /*     if (i < j) { */
  /* 	fprintf(fp, "(assert (bvule (bvmul dst_p%d_p%d dst_p%d_p%d) (bvadd (pointdist p%dx p%dx p%dy p%dy) del_p%d_p%d)))\n", i, j, i, j, i, j, i, j, i, j); */
  /* 	fprintf(fp, "(assert (bvuge (bvmul dst_p%d_p%d dst_p%d_p%d) (bvsub (pointdist p%dx p%dx p%dy p%dy) del_p%d_p%d)))\n", i, j, i, j, i, j, i, j, i, j); */
  /*     } */
  /*   } */
  /* fprintf(fp, "\n"); */

  //(relaxed) Manhattan distance constraints
  fprintf(fp, ";relaxed Manhattan distance constraints\n");
  for (int i = 0; i < sz; i++)
    for (int j = 0; j < sz; j++)  {
      if (i < j) {
  	fprintf(fp, "(assert (bvule dst_p%d_p%d (bvadd (pointdist p%dx p%dx p%dy p%dy) del_p%d_p%d)))\n", i, j, i, j, i, j, i, j);
  	fprintf(fp, "(assert (bvuge dst_p%d_p%d (bvsub (pointdist p%dx p%dx p%dy p%dy) del_p%d_p%d)))\n", i, j, i, j, i, j, i, j);
      }
    }
  fprintf(fp, "\n");
							


  fprintf(fp, "(check-sat)\n"); 

  //printing all points generated
  char gvstr[MAXGV] = "";
  for (int i = 0; i < sz; i++) {
    char p1s[MX];
    char p2s[MX];
    sprintf(p1s, "p%dx ", i);
    sprintf(p2s, "p%dy", i);
    strcat(gvstr, p1s);
    strcat(gvstr, p2s);
    if (i < sz-1)
      strcat(gvstr, " ");
  }
  fprintf(fp, "(get-value (%s))", gvstr);
  fprintf(fp, "\n");



  //printing all deltas (error margins)
  char gvdelstr[MAXGV] = "";
  int  numentries = 0;
  for (int i = 0; i < sz; i++) 
    for (int j = 0; j < sz; j++)  {
      if (i < j) {
	char delstr[MX];
	sprintf(delstr, "del_p%d_p%d", i, j);
	strcat(gvdelstr, delstr);
	numentries++;
	if (numentries < (sz*sz - sz)/2)
	  strcat(gvdelstr, " ");
      }
    }
  fprintf(fp, "(get-value (%s))", gvdelstr);
  fprintf(fp, "\n");


  fprintf(fp, "(exit)\n");
  
  fclose(fp);

  //printf("wrote bunch of stuff, almost the end.\n");
  //exit(1);
  
  return;
}

char*  get_distortions(char** pts, int num_delta_distances, int max_distortion_val) {  
  char* retstr = malloc(sizeof(char) * MAXGV);
  strcpy(retstr, "[");

  for (int i = 0; i < num_delta_distances; i++) {
    if (i == num_delta_distances-1) {
      //printf("%ld, (%d)]\n", print_number_from_smt_string(pts[i]), max_distortion_val);
      sprintf(retstr+strlen(retstr),  "%ld, (%d)]", print_number_from_smt_string(pts[i]), max_distortion_val);
    }
    else {
      //printf("%ld, ", print_number_from_smt_string(pts[i]));
      sprintf(retstr+strlen(retstr), "%ld, ", print_number_from_smt_string(pts[i]));
    }
  }

  return retstr;
}

char* get_points(char** pts, int num_points) {
  char* retstr = malloc(sizeof(char) * MAXGV);
  strcpy(retstr, "[");
  
  for (int i = 0; i < num_points-1; i+= 2) {
    if (i == num_points - 2) {
      //printf("(%s, %s)]\n", pts[i], pts[i+1]);
      //printf("(%ld, %ld)]\n", print_number_from_smt_string(pts[i]), print_number_from_smt_string(pts[i+1]));      
      sprintf(retstr+strlen(retstr), "(%ld, %ld)]", print_number_from_smt_string(pts[i]), print_number_from_smt_string(pts[i+1]));
    }
    else {
      //printf("(%s, %s), ", pts[i], pts[i+1]);
      //printf("(%ld, %ld), ", print_number_from_smt_string(pts[i]), print_number_from_smt_string(pts[i+1]));
      sprintf(retstr+strlen(retstr), "(%ld, %ld), ", print_number_from_smt_string(pts[i]), print_number_from_smt_string(pts[i+1]));
    }
  }

  return retstr;
}

void display_array_of_strings(char** x, int numberofpoints, char* displayheader) {
  printf("%s[", displayheader);

  for (int i = 0; i < numberofpoints; i++) {
    if (i == numberofpoints -1)
      printf("%s]\n", x[i]);        
    else 
      printf("%s, ", x[i]);    
  }
}

void print_array(double** d, int sz) {
  printf("\nArray:\n");
  for (int i = 0; i < sz; i++) {
    for (int j = 0; j < sz; j++) {
      printf("%lf", d[i][j]);
      printf( (j == (sz-1))? "": " ");
    }
    printf("\n");
  }
}


char* gen_timestamp_flname(char* str) {
  char* newstr;
  int len = strlen(str);
  newstr = malloc((len+1+4) * sizeof(char));

  strcpy(newstr, "log_");
  int nindex = 4;
  char *p;
  for (p = str; *p != '\0'; p++) {
    if ( (*p == ' ') || (*p == ':')) {
      newstr[nindex++] = '_';
    } 
    else if (*p == '\n') {
    } else {
      newstr[nindex++] = *p;
    }    
  }
  newstr[nindex] = '\0';
  //printf("flname: %s\n", newstr);
  return newstr;
}

int findspaces(char *inp) {
  char *p;
  int count = 0;
  for (p = inp; *p != '\0'; p++) {
    if (*p == ' ')
      count++;
    else if (*p == '\t')
      count += 8;
  }
  return count;
}

char* padstr(char* ins) {
  char *p;
  for (p = ins; *p != '\0'; p++)
    if (*p == ' ')
      *p = '0';       
  return ins;
}
