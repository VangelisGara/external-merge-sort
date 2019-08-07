#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bf.h"
#include "sort_file.h"
#include "sf.h"

SR_ErrorCode SR_Init() {
  // No need to initialize somehting
  return SR_OK;
}

SR_ErrorCode SR_CreateFile(const char *fileName) {
  //printf("\nCreate File : %s \n\n",fileName);
  BF_Block* block;
  BF_Block_Init(&block);
  int fd, errorCode = SR_OK;
  char* data;

  errorCode = BF_CreateFile(fileName); // Create the empty blocks file
  if(errorCode != SR_OK)
    return printError(SR_FILE_ALREADY_EXISTS);
  else{
    BF_OpenFile(fileName,&fd);  // Open the file we just created
    BF_AllocateBlock(fd,block); // Create our first metadata block
    data = BF_Block_GetData(block);
    char sign = 's'; // stands for sort file
    memcpy(data, &sign, sizeof(char));
    int numberOfRecords = 0;
    memcpy(data+sizeof(char), &numberOfRecords, sizeof(int)); // set number of records in file 0
    //printdata(data);
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block); //unpin
    BF_CloseFile(fd);     // Close 
    return printError(SR_OK);
  }
}

SR_ErrorCode SR_OpenFile(const char *fileName, int *fileDesc) {
  //printf("\nOpen File...%s \n",fileName);
  BF_Block* block;
  BF_Block_Init(&block);
  char* data;  
  int errorCode = SR_OK;
  errorCode = BF_OpenFile(fileName,fileDesc); 
  if(errorCode != SR_OK)
    return printError(SR_FILE_DOESNT_EXIST);
  else{
    BF_GetBlock(*fileDesc, 0, block); // Get our first block to get metadata
    data = BF_Block_GetData(block);
    char sign;
    memcpy(&sign, data, sizeof(char));
    BF_UnpinBlock(block);
    if ( sign == 's')  // Checks if it is a sort file
      return printError(SR_OK);
    else
      return printError(SR_INCOMPATIBLE_FILE);
  }
}

SR_ErrorCode SR_CloseFile(int fileDesc) {
  //printf("\nClose File... \n\n");
  int errorCode = SR_OK;
  errorCode = BF_CloseFile(fileDesc); 
  if(errorCode != SR_OK)
    return printError(SR_FILE_DOESNT_EXIST);
  else
    return printError(SR_OK);
}

SR_ErrorCode SR_InsertEntry(int fileDesc,	Record rec){
  char *data;
  int  blocksNumber, errorCode = SR_OK;
  errorCode = BF_GetBlockCounter(fileDesc, &blocksNumber);         
  
  if(errorCode != SR_OK)
    return printError(SR_FILE_DOESNT_EXIST);
  else
  {
    BF_Block* block;
    BF_Block_Init(&block);
    BF_GetBlock(fileDesc, blocksNumber-1, block);        
    int offset , numberOfRecords;
    if (blocksNumber != 1){
      data = BF_Block_GetData(block);
      memcpy(&numberOfRecords, data, sizeof(int));
      offset = sizeof(int) + numberOfRecords*sizeof(Record);
    } // if record doesn't fit in current block or we have only metadata block, create one
    if (blocksNumber == 1 || sizeof(Record) > (BF_BLOCK_SIZE - offset)) {       
      BF_UnpinBlock(block);
      BF_AllocateBlock(fileDesc, block);       
      data = BF_Block_GetData(block);
      numberOfRecords = 0;
      memcpy(data, &numberOfRecords, sizeof(int));
      offset = sizeof(int);
    }
    memcpy(data+offset, &rec, sizeof(Record)); // add it after last current record  
    numberOfRecords++;
    memcpy(data, &numberOfRecords, sizeof(int));                 
    BF_Block_SetDirty(block);                           
    BF_UnpinBlock(block);                      
    return printError(SR_OK);
  }
}

SR_ErrorCode SR_SortedFile(const char* input_filename, const char* output_filename, int fieldNo, int bufferSize){
  // printf("\nSorted File \n\n");
  if ( bufferSize < 3 || bufferSize > BF_BUFFER_SIZE)
    return printError(SR_INCOMPATIBLE_BUFFERSIZE);
  else {
    int input_fd, temp_fd, output_fd; // our files that will need for Sorted File
    int i,j,k,m,r=1;

  
    // ==================================~ Sort Phase ~=====================================
    // =====================================================================================

    // Work on a temp file
    SR_OpenFile(input_filename,&input_fd);
    SR_CreateFile("temp.db");
    SR_OpenFile("temp.db",&temp_fd);
    Copy_DataFile(&input_fd,&temp_fd); // copy input to temp
    SR_CloseFile(input_fd);

    i = 1; // current run
    BF_GetBlockCounter(temp_fd,& j); // j = the number of blocks in file
    j = j - 1; //remove metadata block
    k = bufferSize; // number of blocks
    m = (int)(ceil((double)j/(double)k)); // number of runs
    //printf("Total Blocks: %d\n", j); 
    //printf("Runs: %d\n", m);

    int start_block = 1, end_block = k; // a run is limited by a start and end block
    while(i<=m)
    {
      //printf("Run : %d\n", r);

      // Run , size of k blocks , will be sorted here
      int start = sizeof(int); // skip metadata block, offset starts here

      BF_Block* block;
      BF_Block_Init(&block);
      
      // get number of records of end block
      BF_GetBlock(temp_fd, end_block , block);
      char  *data = BF_Block_GetData(block); 
      int nor;
      memcpy(&nor, data, sizeof(int));

      BF_UnpinBlock(block);

      int end = sizeof(int) + (nor-1)*sizeof(Record); // the offset of last record, that is going to be sorted
      
      //printf("Before QuickSort function\n");
      //print_run(temp_fd,start,end,start_block,end_block);
      
      QuickSort(temp_fd, start, end, start_block, end_block , fieldNo);
     
      //printf("After QuickSort function\n");
      //print_run(temp_fd, start, end, start_block, end_block);
      //getchar();

      // Get the next run
      start_block += k;
      end_block += k;
      if(end_block > j)
      {
        end_block = j;
      }

      i++;
      r++;

    }
    
    //printf("After Sorting Runs\n");
    //SR_PrintAllEntries(temp_fd);
    //getchar();
    
    // ==================================~ Merge Phase ~=====================================
    // ======================================================================================
    
    // we will work with before and after files , to store the different runs produced , after merging
    int before,after;
    SR_CreateFile("before.db");
    SR_OpenFile("before.db",&before);
    Copy_DataFile(&temp_fd, &before);
    SR_CloseFile(temp_fd);    
    remove("temp.db"); // no need to have it anymore , better naming with before / after
    //SR_PrintAllEntries(before);
    //getchar();
    
    i = 1;
    int p = (int)ceil((double)log(m)/(double)log(k-1)); // number of phases
    j = m;
    int runs = k - 1;
    int step = k;
    
    while(i<=p){ // for every phase
      SR_CreateFile("after.db"); // merged runs will be stored here
      SR_OpenFile("after.db",&after);
      int n = 1;
      int q = (int)ceil((double)(j)/(double)(k-1)); // number of runs
      //printf("q:%d\n", q);
      int r = 1;
      while(n<=q){ // get k-1 runs and merge theme
        MergeRuns(before, after, runs, step, r , fieldNo); 
        r = r + runs*step;
        //printf(">>>new run , after merging\n");
        //SR_PrintAllEntries(after);
        //getchar();
        n++;
      }

      // After file , becomes before , so we can merge the newly produced runs
      SR_CloseFile(before);
      SR_CloseFile(after);

      remove("before.db");
      rename("after.db","before.db");
      SR_OpenFile("before.db",&before);

      step = runs * step; // get next runs
      j = q;
      i++;
      //getchar();
    }

    // =================================~ Final Phase ~===================================
    // ===================================================================================
    SR_CreateFile(output_filename);
    SR_OpenFile(output_filename,&output_fd);
    Copy_DataFile(&before,&output_fd);
    SR_CloseFile(before);
    remove("before.db");
    //printf("After Merging Runs\n");
    //SR_PrintAllEntries(output_fd);
    //getchar();
    
    SR_CloseFile(output_fd);
    return SR_OK;
  }
}

SR_ErrorCode SR_PrintAllEntries(int fileDesc) {
  BF_Block* block;
  BF_Block_Init(&block);
  int blocksNumber;
  int errorCode = SR_OK;
  errorCode = BF_GetBlockCounter(fileDesc, &blocksNumber);         
  if(errorCode != SR_OK)
    return printError(SR_FILE_DOESNT_EXIST);
  else
  {     
    int i;
    for (i=1; i<blocksNumber; i++) 
    {
      BF_GetBlock(fileDesc, i, block);             
      char *data;
      data = BF_Block_GetData(block);
      int numberOfRecords;
      memcpy(&numberOfRecords, data, sizeof(int));  
      printf(">>>Block:%d with %d records\n", i, numberOfRecords );         
      int offset = sizeof(int);                                 
      int j;
      //printf("numberOfRecords:%d\n", numberOfRecords);
      for(j=0; j<numberOfRecords; j++)
      {
        Record rec;
        memcpy(&rec, data+offset, sizeof(Record));       
        offset = offset + sizeof(Record);
        print_record(&rec);
      }
      BF_UnpinBlock(block);                        
    }
    return printError(SR_OK);
  }
}
