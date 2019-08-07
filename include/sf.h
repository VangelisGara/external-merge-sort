#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bf.h"
#include "sort_file.h"

//it is very helpful in cases that we want to return from a function
//the "address"(offset, block) from a record
// 
typedef struct R {
	int offset;
	int block;
} R;

typedef struct R1 {
	int offset;
	int block;
	int start_block;
	int end_block;
} R1;

SR_ErrorCode printError(SR_ErrorCode ec);

void printarray(int* arr , int k);

void Copy_DataFile(int* copy , int* copied );

void print_record(Record *rec);

void print_run(int fd , int start , int end, int start_block , int end_block);

void printdata(char* data);

void swap(int fd, int b1, int b2, int offset1, int offset2);

struct R Partition(int fd, int start, int end, int start_block, int end_block,int fieldNo);

struct R next_rec(int fd, int start_block, int end_block, int current_index, int current_block);

struct R prev_rec(int fd, int start_block, int end_block, int current_index, int current_block);

struct Record get_record(int fd, int block_number, int offset);

void QuickSort(int fd, int start, int end, int start_block, int end_block,int fieldNo);

void MergeRuns(int before, int after, int runs, int step, int r, int fieldNo);