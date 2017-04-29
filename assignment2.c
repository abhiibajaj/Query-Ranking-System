/*  Read invereted-index file data from a text file, and look up and rank 
	queries as specified on the commandline

	Solution to comp10002 2016s2 Assignment2,
	prepared by Arpit Bajaj, bajaja@student.unimelb.edu.au
	October 2016
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>	
#include <string.h>
#include <math.h>


#define EPSILON 1e-20			/* set to 1e-20, for maximum accuracy */

#define MAX_CHARS 999			/* maximum characters for each term */
#define INITIAL 1	    		/* Initial value to be used for malloc */

#define FOUND 1					/* return value if term is found for 
									binary_search */
#define NOT_FOUND -1			/* return value if term is not found for 
									binary_search */

#define K 1.2					/* Constant k to be used during scoring */
#define B 0.75					/* Constant b to be used during scoring */

#define TOP3 3					/* The top 3 scores to be sorted and shown */

/* one entry in a text file of maximum possible characters */
typedef char one_entry_t[MAX_CHARS+1];



/* stores the document and frequency of each pair */
typedef struct {
	int doc;
	int freq;
} pair_t;

/* stores the term as a string, the amount of pairs it has, and it's pairs in
	an array of pair_t*/
typedef struct {
	char *term;
	int npairs;
	pair_t *pair;
} one_data_t;

/******************************************************************************/
/* Function Prototypes */

void int_swap(int *x, int *y);
void double_swap(double *x, double *y);
void print_stage_one(one_data_t *all_entries, int terms, int total_pairs);
void partial_sort(double A[], int nA, int C[], int k);
void sum_words(int A[], int total_doc, int *total_words);
void doc_length(one_data_t *all_entries, int terms, int A[], int total_doc);
void free_all_memory(one_data_t *all_entries, int terms, int *doc_words, 
						int *indexes, double *ld_values, double *doc_scores);
void create_structure(FILE *fp, one_data_t **all_entries, one_entry_t one_word, 
						int *terms, int *total_pairs,
						int *count, int limit, int *spaces, 
						int *total_doc);
void print_pairs(one_data_t *all_entries, int term, int num_pairs);
void avg_doc_length(int total_words, int total_doc, double *avg_length);
void compute_ld(int A[], double C[], int total_doc, double avg_length);
void compute_scores(one_data_t *all_entries, int doc_words[], int total_doc,
					double doc_scores[], double ld_values[], int term_num);
void int_zeros(int A[], int buddy);
void double_zeros(double A[], int buddy);
int read_file(FILE *fp, char W[], int limit, int *spaces);
int near_equal(double x1, double x2);
int binary_search(one_data_t *all_entries, int lo, int hi, char *key, 
					int *locn);
double log2(double num);

/******************************************************************************/

/* main program controls all the action
*/
int
main (int argc, char *argv[]){	
	/* Inititalise the variables required */
	int terms=0, total_pairs=0, spaces=0, count=0, locn=0;
	int total_doc=0, total_words=0, i=0;
	
	int *doc_words, *indexes; /* doc_words holds the words in each document 
									indexes holds the original index for
									document scores */	
	
	char str[MAX_CHARS];	/* Used for strtok, where each line in stdin
								contain at most MAX_CHARS (according to 
								assignment specification) */
	
	char *query;			/* Used to hold every query from stdin */
	
	double avg_length = 0.0;
	
	double *ld_values;		/* Used to hold the ld value for each document */
	double *doc_scores;		/* Used to hold the scores for each document */
	
	one_entry_t one_word;	
	one_data_t *all_entries;	/* Structure to hold all the data */
	
	/* Allocate the required space for the data strucuture and ensure it 
		the space is actually allocated */
	all_entries = malloc(INITIAL * sizeof(*all_entries));
	assert(all_entries!=NULL);
	
	/* Open file and ensure it is valid */
	FILE *fp;
	if((fp = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, ".... Failed\n");
	} else {
		fprintf(stderr, "\n");
		/* Create the all_entries structure */
		create_structure(fp, &all_entries, one_word, &terms, 
						 	&total_pairs, &count, MAX_CHARS, &spaces,
						 	&total_doc);
		
		/* Allocate space for the following arrays, using total_docs
			to get the exact size */
		doc_words = (int *)malloc(total_doc * sizeof(*doc_words));
		assert(doc_words!=NULL);
		ld_values = (double *)malloc(total_doc * sizeof(*ld_values));
		assert(ld_values!=NULL);
		doc_scores = (double *)malloc(total_doc * sizeof(*doc_scores));
		assert(doc_scores!=NULL);
		indexes = (int *)malloc(total_doc * sizeof(*doc_scores));
		assert(indexes!=NULL);
		
		/* Get frequency of words in each document and store them in 
			corresponding index of doc_words */
		doc_length(all_entries, terms, doc_words, total_doc);
		
		/* Sum the words in doc_words and store it in total_words */
		sum_words(doc_words, total_doc, &total_words);
		
		/* Calculate the average length of documents (computed as the arithmetic 
			mean) */
		avg_doc_length(total_words, total_doc, &avg_length);
		
		/* Calculate Ld value for each document and store it in 
			corresponding index of ld_values */
		compute_ld(doc_words, ld_values, total_doc, avg_length);
		
		/* Stage One Output! */
		print_stage_one(all_entries, terms, total_pairs);
		
		/* Get input from the stdin */
		while(fgets(str, sizeof(str), stdin)){
			
			/* Initialise doc_scores with zeros */
			double_zeros(doc_scores, total_doc);
			/* Stage Two and Three Output everytime there is input in stdin */
			printf("\nStage 2 Output:\n");
			
			/* Use strtok to split the input into queries, using \r\n for 
				dimefox */
			query = strtok(str, " ,\r\n");
			while(query!=NULL){
				/* Search for each query using binary search, and store it's
					location in locn. If item is found, compute the scores
					for each document */
				if(binary_search(all_entries, 0, terms, query, &locn)==1){
					printf("\t\"%s\" is term %d\n", query, locn+1);
					compute_scores(all_entries, doc_words, total_doc, doc_scores, 	
								ld_values, locn);
				} else {
					printf("\t\"%s\" is not indexed\n", query);	
				}
				locn=0;	/* Re-initialise location to 0 for the next query */
				query = strtok(NULL, " ,\r\n");
			}
			
			printf("\nStage 3 Output\n");	
			/* Partially sort to get the top 3 scores*/
			partial_sort(doc_scores, total_doc, indexes, TOP3);
			/* Print the top 3 scores, that are not 0 */
			for(i=0;i<TOP3;i++){
				if(doc_scores[indexes[i]]>0.0){
					printf("\tdocument%4d: score  %.3f\n", indexes[i]+1, 
														doc_scores[indexes[i]]);
				}
			}
		
		}
		/* Free all memory from all entries */
		free_all_memory(all_entries, terms, doc_words, indexes, ld_values, 
						doc_scores);
		printf("\nTa da-da-daaah...\n");
		
	}
	/* All complete! */
	
	return 0;	
}

/******************************************************************************/
/* Prints the output for stage one */
void
print_stage_one(one_data_t *all_entries, int terms, int total_pairs) {
	int i=0;
	printf("Stage 1 Output\n");
	printf("index has %d terms and %d (d,fdt) pairs\n", terms, total_pairs);
	
	/* Increment i based on assignment specification */
	while(i<terms){
		printf("term %d is \"%s\":\n", i, all_entries[i].term);
		/* Print each pair */
		print_pairs(all_entries, i, all_entries[i].npairs);
		if(i==0){
			i = 1;
		} else if (i==1) {
			i=terms-2;
		} else if (i==terms-2){
			i =terms-1;
		} else {
			i++;
		}
	}
}

/******************************************************************************/
/* Create the all_entries structure, keeping track of spaces, terms, 
	total documents */

void
create_structure(FILE *fp, one_data_t **all_entries, one_entry_t one_word,
						int *terms, int *total_pairs, 
						int *count, int limit, int *spaces,
						int *total_doc){
	size_t current_size = INITIAL;
	/* Get each word/integer from the input file */
	while(read_file(fp, one_word, MAX_CHARS, spaces)){
		if((*terms) == current_size) {
			/* Reallocate if space runs out in the array */
			current_size *=2;
			(*all_entries) = realloc((*all_entries), 
									current_size*sizeof(**all_entries));
			assert(all_entries!=NULL);
		}
		
		if((*spaces) == 1){
			
			/* If spaces is one, store it in the term section of the structure,
				also allocates space for it */
			(*all_entries)[(*terms)].term = (char *)malloc(1 + strlen(one_word));
			assert((*all_entries)[(*terms)].term!=NULL);
			strcpy((*all_entries)[(*terms)].term,one_word);
			
		} else if ((*spaces)==2){
			
			/* If spaces is two, store it as an integer in the npairs  
				section of the structure, also allocate space for it.
				Increment total pairs with the current pairs read.*/
			(*all_entries)[(*terms)].npairs=atoi(one_word);
			(*total_pairs)+=atoi(one_word);
			/* Allocate space for the pairs based on the current 
				amount of pairs */
			(*all_entries)[(*terms)].pair = (pair_t*)malloc(atoi(one_word)*
											sizeof(pair_t));
			assert((*all_entries)[(*terms)].pair != NULL);
			
		} else {
			/* If spaces mod 2 is not 0, the current word is the document
				number, otherwise it is the frrequency. Store it in pairs
				array, at position count. Count is incremented everytime 
				a frequency is read */
			if ((*spaces)%2 != 0){
				if(atoi(one_word)>(*total_doc)){
					(*total_doc)=atoi(one_word);
				}
				(*all_entries)[(*terms)].pair[(*count)].doc=atoi(one_word);
			} else if ((*spaces)%2==0) {
				(*all_entries)[(*terms)].pair[(*count)].freq=atoi(one_word);			
				(*count)++;
			}
				
		}
		/* If spaces is 0, terms is incremented, and count is reinitialised 
			to 0*/
		if((*spaces)==0){
			(*terms)++;
			(*count)=0;
		}	
	
	}
}

/******************************************************************************/
/* Compute and increment each document score, based on the provided BM25 
	method, based on the current term_num */

void 
compute_scores(one_data_t *all_entries, int doc_words[], int total_doc,
					double doc_scores[], double ld_values[], int term_num){
	
	int i, j, fdt; 
	/* If the term occurs in the document, set fdt to the frequency 
		at that document, otherwise set fdt to 0*/
	for(i=0;i<total_doc;i++){
		for(j=0;j<all_entries[term_num].npairs;j++){
			if(i==all_entries[term_num].pair[j].doc-1){
				fdt = all_entries[term_num].pair[j].freq;
				break;
			} else {
				fdt = 0;
			}
		}
		/* Calculate the doc_scores for the current term_num and add it to the
			previous scores of each document */
		doc_scores[i]+=((((1+K)*fdt)/(ld_values[i]+fdt)) * 
			log2((total_doc+0.5)/(all_entries[term_num].npairs)));
	}
}

/******************************************************************************/


void
partial_sort(double A[], int nA, int C[], int k){
	int i, j, max;
	/* Create a local double array and copy the items from the input 
		array A, into the local array */
	double *new;
	new = (double *)malloc(nA * sizeof(*new));
	for(i=0;i<nA;i++){
		new[i] = A[i];
	}
	/* Fill array C with the original indexes */
	for(i=0;i<nA;i++){
		C[i]=i;
	}
	/* Get the top k elements, and keep track of their elements in C */
	for(i=0;i<k;i++){
		max = i;
		for(j=i+1;j<nA;j++){
			if(new[j]>new[max] || near_equal(new[j], new[max]))	{
				max = j;
			} 
		}
		double_swap(&new[i], &new[max]);
		int_swap(&C[i], &C[max]);
	}
	free(new);
	new = NULL;
}

/******************************************************************************/
/* Compute ld values for each document ad store them in a double array */

void
compute_ld(int A[], double C[], int total_doc, double avg_length){
	int i;
	/* Calculate ld for each document, based on the equation provided
		in the assignment specification */
	for(i=0;i<total_doc;i++){
		C[i] = (double)K*((1-B) + (B * (A[i]/avg_length)));
	}
}

/******************************************************************************/
/* Calculate the average length of the documents */
void 
avg_doc_length(int total_words, int total_doc, double *avg_length){
	(*avg_length) = (double)total_words/total_doc;
}

/******************************************************************************/
/* Calculate log2 */
double
log2(double num){
	return log(num)/log(2);
}

/******************************************************************************/
/*  double array with 0s */
void
double_zeros(double A[], int buddy){
	int i;
	for(i=0;i<buddy;i++){
		A[i]=0.0;
	}
}

/******************************************************************************/
/* Populate integer array with 0s */

void
int_zeros(int A[], int buddy){
	int i;
	for(i=0;i<buddy;i++){
		A[i]=0;
	}
}

/******************************************************************************/
/* Populate double array with 0s */

void
doc_length(one_data_t *all_entries, int terms, int A[], int total_doc){
	int i, j;
	int_zeros(A, total_doc);
	for(i=0;i<terms;i++){
		for(j=0;j<(all_entries[i].npairs);j++){
			A[(all_entries[i].pair[j].doc)-1]+= all_entries[i].pair[j].freq;
		}
	}
}

/******************************************************************************/
/* Calculate the total words in in the file*/

void
sum_words(int A[], int total_doc, int *total_words){
	int i;
	for(i=0;i<total_doc;i++){
		(*total_words)+=A[i];
	}
}

/******************************************************************************/
/* Recursively search for key in all_entries.terms.
	Function was adapted from page 206 of Programming, Problem, Solving and
	Abstraction textbook */

int
binary_search(one_data_t *all_entries, int lo, int hi, char *key, int *locn){
	int mid, outcome;
	
	if(lo>=hi){
		return NOT_FOUND;
	}
	mid = (lo+hi)/2;
	/* Compare key to the middle term. If outcome is negative, binary search
		in the left side of the array, otherwise search in the right side.*/
	if((outcome = strcmp(key, all_entries[mid].term))<0){
		return binary_search(all_entries, lo, mid, key, locn);
	} else if (outcome >0){
		return binary_search(all_entries, mid+1, hi, key, locn);
	} else {
		/* location is found! */
		(*locn) = mid;
		return FOUND;
	}
	
}

/******************************************************************************/
/* Print pairs, when provided with the data structur, the term numeber,
	and how many pairs, the term has*/
void 
print_pairs(one_data_t *all_entries, int term, int num_pairs){
	int i;
	printf("\t");
	for(i=0;i<num_pairs;i++){
		printf("%d,%d", all_entries[term].pair[i].doc,
						all_entries[term].pair[i].freq);
		
		/* Only print the first 10 doc, pairs */
		if(i==9){
			printf("....");
			break;
		}
		/* Seperate each pair with a ';', except the last item */
		if(i!=num_pairs-1){
			printf("; ");
		}
		
	}
	printf("\n");
}

/******************************************************************************/
/* Free all the memory!!!! */
void 
free_all_memory(one_data_t *all_entries, int terms, int *doc_words, 
				int *indexes, double *ld_values, double *doc_scores){
	int i;
	for(i=0; i<terms; i++){
		free(all_entries[i].pair);
		all_entries[i].pair=NULL;
		free(all_entries[i].term);
		all_entries[i].term=NULL;
	}
	free(all_entries);
	free(doc_words);
	free(indexes);
	free(ld_values);
	free(doc_scores);
	all_entries=NULL;
	doc_words=NULL;
	indexes=NULL;
	ld_values=NULL;
	doc_scores=NULL;
}

/******************************************************************************/
/* Extracts an entry from the text file, incrementing spaces when a space 
	character is hit and setting space to 0 when a newline character
	is hit, and returning as soon as one of those character is read.
 */
int
read_file(FILE *fp, char W[], int limit,int *spaces) {
	int len=0,c;
	
	while((c=getc(fp)) && (len<limit)){
		if(c == '\n'){
			W[len]='\0';
			(*spaces)=0;
			return 1;
		} else if (c == ' '){
			W[len]='\0';                  
			(*spaces)++;
			return 1;
		}
		
		W[len++]=c;
	}
	
	return 0;
}

/******************************************************************************/
/* Near equal function used to test doubles for equality, due to inaccuracies
	of double arithmetic. Adapted from workshop solutions to exercise 5.8
	from LMS. */
int
near_equal(double x1, double x2) {
	double ratio;
	if (x1==0.0 && x2==0.0) {
		/* both are zero */
		return 1;
	} else if (x1*x2 <= 0.0) {
		/* one is zero, or they have opposite signs */
		return 0;
	} else {
		ratio = x1/x2;
		if (ratio < 1.0) {
			/* invert the ratio to get number bigger than 1 */
			ratio = 1/ratio;
		}
		/* now check if ratio is only slightly greater than 1 */
		return (ratio < 1+EPSILON);
	}
}

/******************************************************************************/
/* function to swap doubles, adapted from the textbook */
void
double_swap(double *x, double *y) {
	double t;
	t = *x;
	*x = *y;
	*y = t;
}

/******************************************************************************/
/* function to swap integers, adapted from the textbook */
void
int_swap(int *x, int *y) {
	int t;
	t = *x;
	*x = *y;
	*y = t;
}

/******************************************************************************/
/* algorithms are fun */