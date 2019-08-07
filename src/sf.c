#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bf.h"
#include "sort_file.h"
#include "sf.h"

// prints an array of integers with k elements
void printarray(int* arr , int k){
  for (int i=0;i<k;i++)
    printf("%d ", arr[i]);
  printf("\n");
}

// handles errors
SR_ErrorCode printError(SR_ErrorCode ec){
  if(ec == SR_OK)
  {
      //printf("OK: Everything is fine! (code %d)\n", ec);
      return SR_OK;
  }
  else if(ec == SR_FILE_ALREADY_EXISTS)
  {
    printf("ERROR: File already exists! (code %d)\n", ec);
    return SR_FILE_ALREADY_EXISTS;
  }
  else if(ec == SR_FILE_DOESNT_EXIST)
  {
    printf("ERROR: The file you are trying to open doesn't exist! (code %d)\n", ec);
    return SR_FILE_DOESNT_EXIST;
  }
  else if(ec == SR_INCOMPATIBLE_FILE)
  {
    printf("ERROR: The type of file you are trying to open is incompatible! (code %d)\n", ec);
    return SR_INCOMPATIBLE_FILE;
  }
  else if(ec == SR_INCOMPATIBLE_BUFFERSIZE)
  {
    printf("ERROR: BufferSize should be bigger than 2 and smaller than BF_BUFFER_SIZE! (code %d)\n", ec);
    return SR_INCOMPATIBLE_BUFFERSIZE;
  }
}

// makes a from of a data file
void Copy_DataFile(int* from , int* to ){
  int blocksNumber;
  BF_GetBlockCounter(*from, &blocksNumber);         
  
  for (int i=1; i<blocksNumber; i++){
    BF_Block* block;
    BF_Block_Init(&block);
    BF_GetBlock(*from, i, block);             
    
    char *data;
    data = BF_Block_GetData(block);
    int numberOfRecords;
    memcpy(&numberOfRecords, data, sizeof(int));            
    int offset = sizeof(int);                                 
    int j;
    
    for(j=0; j<numberOfRecords; j++){
      Record rec;
      memcpy(&rec, data+offset, sizeof(Record));       
      offset = offset + sizeof(Record);
      SR_InsertEntry(*to,rec);
    }

    BF_UnpinBlock(block);                        
  }
}

// prints a record
void print_record(Record *rec)
{
  printf("Id: %d ",rec->id);
  printf("Name: %s ",rec->name);
  printf("Surame: %s ",rec->surname);
  printf("City: %s \n",rec->city);
}

// prints a run 
void print_run(int fd , int start , int end, int start_block , int end_block){
    int i,j,count = 0;
    for (i=start_block; i<=end_block; i++) {
      BF_Block* block;
      BF_Block_Init(&block);
      BF_GetBlock(fd, i, block);             
      char *data;
      data = BF_Block_GetData(block);
      int numberOfRecords;
      memcpy(&numberOfRecords, data, sizeof(int)); 
      int offset = sizeof(int)+ (numberOfRecords-1)*sizeof(Record);           
      
      int s, e; // the start and end of this block
      if((i == start_block) && (i == end_block)){
        s = start;
        e = end;
      }
      else if(i == start_block){
        s = start;
        e = offset;
      }
      else if(i == end_block){
        s = sizeof(int);
        e = end;
      }
      else{
        s = sizeof(int);
        e = offset;
      }

      for(j=s; j<=e; j+=sizeof(Record)){
        Record rec;
        memcpy(&rec, data+j, sizeof(Record));       
        print_record(&rec);
        count++;
      } 
      BF_UnpinBlock(block);                     
    }
    printf("count: %d\n", count);
    printf("------------------------------------------\n");
}

// prints block's data
void printdata(char* data){
  printf("Printing Data...\n");
  int i,nr;
  memcpy(&nr, data, sizeof(int));
  printf("Number Of Records : %d\n", nr);
  int offset = sizeof(int);
  for(i=0; i< nr; i++){
    Record rec;
    memcpy(&rec, data+offset, sizeof(Record));
    offset += sizeof(Record);
    printf("ID : %d   ",rec.id);
    printf("Name : %s   ",rec.name);
    printf("Surame : %s   ",rec.surname);
    printf("City: %s  \n",rec.city);
  }
}

// swap two records , in block1 with offest1 and blcok2 with offset2 , in file
void swap(int fd, int b1, int b2, int offset1, int offset2)
{
  if(b1 != b2) // case to swap in different blocks
  {
    //get first block
    BF_Block* block1;
    BF_Block_Init(&block1);
    BF_GetBlock(fd, b1, block1);
    char *data1;
    data1 = BF_Block_GetData(block1);

    //get second block
    BF_Block* block2;
    BF_Block_Init(&block2);
    BF_GetBlock(fd, b2, block2);
    char *data2;
    data2 = BF_Block_GetData(block2);

    //swap phase
    Record temp;
    
    memcpy(&temp, data1+offset1, sizeof(Record));
    memcpy(data1+offset1, data2+offset2, sizeof(Record));
    memcpy(data2+offset2, &temp, sizeof(Record));

    BF_Block_SetDirty(block1);
    BF_UnpinBlock(block1);
    BF_Block_SetDirty(block2);
    BF_UnpinBlock(block2);
  }
  else // case to swap in the same block
  {
    //get only block
    BF_Block* block;
    BF_Block_Init(&block);
    BF_GetBlock(fd, b1, block);
    char *data;
    data = BF_Block_GetData(block);

    //swap phase
    Record temp;
    
    memcpy(&temp, data+offset1, sizeof(Record));
    memcpy(data+offset1, data+offset2, sizeof(Record));
    memcpy(data+offset2, &temp, sizeof(Record));
    
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
  }
}

// quicksort's partitioning
struct R Partition(int fd, int start, int end, int start_block, int end_block,int fieldNo)
{
  //printf("-------------------PARTITION-------------------\n");
  //printf("start:%d end:%d start_block:%d end_block:%d\n", start, end, start_block, end_block);

  // set last element as pivot
  BF_Block* Eblock;
  BF_Block_Init(&Eblock);
  Record pivot;
  BF_GetBlock(fd, end_block, Eblock);    
  char* data1 = BF_Block_GetData(Eblock);
  memcpy(&pivot, data1+end, sizeof(Record));
  
  BF_UnpinBlock(Eblock);
  //print_record(&pivot); printf("pivot:(End_)Block:%d\n", end_block);

  // pIndex = the current index , helping partitioning
  int pIndex, pIndex_block;
  pIndex=start;
  pIndex_block=start_block;
  //printf("pIndex:%d , Block:%d\n", pIndex, pIndex_block);

  int i,j;
  for (i=start_block; i<=end_block; i++)
  {
    // get data of current block
    BF_Block* block;
    BF_Block_Init(&block);
    BF_GetBlock(fd, i, block);             
    char* data = BF_Block_GetData(block);

    int numberOfRecords;
    memcpy(&numberOfRecords, data, sizeof(int));
    int offset = sizeof(int)+ (numberOfRecords-1)*sizeof(Record); // get offset of last record in this block

    BF_UnpinBlock(block);

    // start and end of this block 
    int s,e;
    if((i == start_block) && (i == end_block)){
      s = start;
      e = end;
    }
    else if(i == start_block){
      s = start;
      e = offset;
    }
    else if(i == end_block){
      s = sizeof(int);
      e = end;
    }
    else{
      s = sizeof(int);
      e = offset;
    }

    //printf(" block: %d s:%d e:%d\n", i, s , e);

    for(j=s; j<=e; j=j+sizeof(Record)){
      //printf("block: %d , record: %d\n",i,j);
      Record rec;
      BF_Block* block1;
      BF_Block_Init(&block1);
      BF_GetBlock(fd, i, block1);             
      char* data1 = BF_Block_GetData(block1);
      memcpy(&rec, data1+j, sizeof(Record));  
      BF_UnpinBlock(block1);
      //print_record(&rec);
      
      int rc = 1;

      switch(fieldNo) { // partitioning/sorting by fieldNo parameter 
        case 0 :
          if(rec.id < pivot.id)
            rc = -1;
          break;
        case 1 :
          rc = strcmp(rec.name, pivot.name);
          break;
        case 2 :
          rc = strcmp(rec.surname, pivot.surname);
          break;
        case 3 :
          rc = strcmp(rec.city, pivot.city);
          break;
      default :
         printf("Invalid fieldNo\n" );
      }

      if(rc < 0){ // if we have smaller element than pivot
        
        //printf("Found record less than pivot((%d)%s < (%d)%s)\n", rec.id, rec.name, pivot.id, pivot.name);
        swap(fd,i,pIndex_block,j,pIndex);
        
        BF_Block* Pblock;
        BF_Block_Init(&Pblock);
        int Pnor;
        BF_GetBlock(fd, pIndex_block, Pblock);    
        char* pdata = BF_Block_GetData(Pblock);
        memcpy(&Pnor, pdata, sizeof(int));
        BF_UnpinBlock(Pblock);
        
        int poffset = sizeof(int) + (Pnor-1)*sizeof(Record);
        // pIndex will be incremented , so we can continue partitioning
        if(pIndex < poffset){ // same block
          pIndex = pIndex + sizeof(Record);
          //printf("New pIndex:%d\n", pIndex);
        }
        else{ // if pindex needs to change block
          pIndex= sizeof(int);
          pIndex_block++;
        }
          //printf("New pIndex:%d , New Block:%d\n", pIndex, pIndex_block);
      }
    }
  }  
  //getchar();

  // finally , swap pIndex with pivot , to end partinioning
  swap(fd, end_block, pIndex_block, end, pIndex);
  
  // return pIndex , which partitions records by sorting parameter
  struct R indexes;
  indexes.offset = pIndex;
  indexes.block = pIndex_block;

  //printf("pIndex: %d , pIndex_block: %d\n",indexes.offset,indexes.block);

  //printf("---------------END-OF-PARTITION---------------\n");
  return indexes;                    
}

// get the record next of current , limited in blocks of the run
struct R next_rec(int fd, int start_block, int end_block, int current_index, int current_block)
{
  BF_Block* cblock;
  BF_Block_Init(&cblock);
  BF_GetBlock(fd, current_block, cblock);
  char *data = BF_Block_GetData(cblock);
  int numberOfRecords;
  memcpy(&numberOfRecords, data, sizeof(int));
  BF_UnpinBlock(cblock);
  struct R nrec;

  // all the different cases of records position
  if(current_block == end_block){

    if(current_index == sizeof(int)+(numberOfRecords-1)*sizeof(Record)){
      nrec.offset = -1;
      nrec.block = -1;     
    }
    else{
      nrec.offset = current_index + sizeof(Record);
      nrec.block = current_block;
    }
  }
  else{
    if(current_index == sizeof(int)+(numberOfRecords-1)*sizeof(Record)){ 
      nrec.offset = sizeof(int);
      nrec.block = ++current_block;       
    }
    else{
      nrec.offset = current_index + sizeof(Record);
      nrec.block = current_block; 
    }
  }
  //printf("In Next Record: CurrInd:%d CurrBlock:%d NextInd:%d NextBlock%d\n", current_index,current_block,nrec[0],nrec[1]);
  return nrec;  
}

// get the record previous of current , limited in blocks of the run
struct R prev_rec(int fd, int start_block, int end_block, int current_index, int current_block)
{
  BF_Block* cblock;
  BF_Block_Init(&cblock);
  BF_GetBlock(fd, current_block, cblock);
  char *data = BF_Block_GetData(cblock);
  int numberOfRecords;
  memcpy(&numberOfRecords, data, sizeof(int));
  BF_UnpinBlock(cblock);
  struct R prec;
  if(current_block == start_block){
    if(current_index == sizeof(int)){
      prec.offset = -1;
      prec.block = -1;      
    }
    else{
      prec.offset = current_index - sizeof(Record);
      prec.block = current_block;
    }
  }
  else{
    if(current_index == sizeof(int)){
      BF_Block* block;
      BF_Block_Init(&block);
      BF_GetBlock(fd, current_block-1, block);
      char *data1 = BF_Block_GetData(block);
      int numberOfRecords1;
      memcpy(&numberOfRecords1, data1, sizeof(int));
      BF_UnpinBlock(block);     
      prec.offset = sizeof(int)+(numberOfRecords1-1)*sizeof(Record);
      prec.block = --current_block;
    }
    else{
      prec.offset =current_index - sizeof(Record);
      prec.block = current_block;
    }
  }

  //printf("In Prev Record: CurrInd:%d CurrBlock:%d PrevInd:%d PrevBlock%d\n", current_index,current_block,prec[0],prec[1]);
  return prec;  
}

// get a record from file
struct Record get_record(int fd, int block_number, int offset)
{
  Record rec;
  BF_Block* block;
  BF_Block_Init(&block);
  BF_GetBlock(fd, block_number, block);             
  char *data;
  data = BF_Block_GetData(block);
  memcpy(&rec, data+ offset, sizeof(Record));
  BF_UnpinBlock(block);
  return rec;
}

// quicksort runs recusively , to sort a run , limited by stard / end blocks
void QuickSort(int fd, int start, int end, int start_block, int end_block,int fieldNo)  { 
  // quicksort runs if there are more than 1 records
  if((start != -1 && start_block != -1 && end != -1 && end_block != -1 ) && (start_block < end_block || ((start_block == end_block) && (start < end)))) { 
    //printf("\n====================~ QUICKSORT ~=============================\n");
    //printf("start:%d end:%d start_block:%d end_block:%d\n", start, end, start_block, end_block);
    
    //printf("______QUICKSORT WILL RUN ON THESE RECORDS:_____\n");
    //print_run(fd,start,end,start_block,end_block);

    int i,count = 0;

    struct R pIndex;
    pIndex = Partition(fd, start, end, start_block, end_block,fieldNo); // partition records and get the partitioner index
    //printf("After Partition , pIndex: %d pIndex_block: %d", pIndex.offset, pIndex.block);
    //getchar();

    struct R new_end;
    new_end = prev_rec(fd, start_block, end_block, pIndex.offset, pIndex.block);

    struct R new_start;
    new_start = next_rec(fd, start_block, end_block, pIndex.offset, pIndex.block);

    // Quick Sort Left Part
    //printf("\nLEFT-QUICKSORT is called with \nstart:%d end:%d start_block:%d end_block:%d\n", start, new_end.offset, start_block, new_end.block);
    QuickSort(fd, start, new_end.offset, start_block, new_end.block,fieldNo);

    // Quick Sort Right Part
    //printf("\nRIGHT-QUICKSORT is called with \nstart:%d end:%d start_block:%d end_block:%d\n", new_start.offset, end, new_start.block, end_block);
    QuickSort(fd, new_start.offset, end, new_start.block, end_block,fieldNo);
    //printf("\n\n\n==================END~OF~QUICKSORT=================\n");
  }
}

// merge k-1 runs together
void MergeRuns(int before, int after, int runs, int step, int r, int fieldNo){
  //printf("\n=====================~ Merge ~==========================\n");
  //printf("Runs:%d step:%d r:%d\n", runs, step, r);
  int numberOfRecords1 = 0;
  int k = runs + 1;
  int blocksNumber;
  BF_GetBlockCounter(before, &blocksNumber);
  blocksNumber = blocksNumber - 1;
  //printf("%d\n", blocksNumber);
  
  struct R1 pointers[runs];
  int i;
  int numberOfRecords = 0;
  for(i=0; i<runs; i++){
    // for each run find the number of records*/
    int j;
    for(j=r; j<r+step; j++){
      if(j <= blocksNumber){
        BF_Block* block;
        BF_Block_Init(&block);
        BF_GetBlock(before, j, block);             
        char *data;
        data = BF_Block_GetData(block);
        int nor;
        memcpy(&nor, data, sizeof(int));
        //printf("numberOfRecords(each) (in block:%d):%d \n", j, nor);
        numberOfRecords = numberOfRecords + nor;
        BF_UnpinBlock(block);
      }
      else
        break;
    }
    //printf("numberOfRecords(total):%d \n", numberOfRecords);

    // fill table
    if(r > blocksNumber){
      pointers[i].offset = -1;
      pointers[i].block = -1;
      pointers[i].start_block = -1;
      pointers[i].end_block = -1;
      //printf("team of blocks:%d position:%d block:%d start_block:%d end_block:%d\n", i, pointers[i].offset, pointers[i].block, pointers[i].start_block, pointers[i]. end_block);
      //printf(">>>>step is:%d blocksNumber is:%d r is:%d(we just have to return first block)\n", step, blocksNumber, r );
    }
    else{
      pointers[i].offset = sizeof(int);
      pointers[i].block = r;
      pointers[i].start_block = r;
      if(r+step-1 > blocksNumber)
        pointers[i].end_block = blocksNumber;
      else
        pointers[i].end_block = r+step-1;
      //printf("team of blocks:%d position:%d block:%d start_block:%d end_block:%d\n", i, pointers[i].offset, pointers[i].block, pointers[i].start_block, pointers[i]. end_block);
    } 
    r = r + step;
  }
  //getchar();

  int cr = 1;
  while( cr <= numberOfRecords ){ // for every single record , insert smaller in file
    // int min_i = 0, min_offset = pointers[0].offset, min_block = pointers[0].block;
    // Record min_rec = get_record(before, min_block, min_offset);
    // find first record
    int flag = 0;
    int min_i, min_offset, min_block;
    Record min_rec;
    for(i=0; i<runs; i++){
      if (pointers[i].offset != -1 && pointers[i].block != -1){
        min_i = i;
        min_offset = pointers[i].offset;
        min_block = pointers[i].block;
        min_rec = get_record(before, min_block, min_offset);
        flag = 1;
        break;
      }
    }
    //printf("min_i:%d min_offset:%d min_block:%d\n", min_i, min_offset, min_block);
    //getchar();

    if(flag == 1){ // if there are records in this run
      //printf("found first min\n");
      //printf("min_i:%d min_offset:%d min_block:%d\n", min_i, min_offset, min_block);
      for(i=0; i<runs; i++){
        if(pointers[i].block == -1 && pointers[i].offset == -1){
          //printf("no more records in this run\n");
          //getchar();
        }
        else{
          Record temp = get_record(before, pointers[i].block, pointers[i].offset);
          //printf("Checking...\n");
          //print_record(&min_rec);
          //print_record(&temp);
          int rc = 1;
          switch(fieldNo) {
          case 0 :
            if(temp.id < min_rec.id)
              rc = -1;
            break;
          case 1 :
            rc = strcmp(temp.name, min_rec.name);
            break;
          case 2 :
            rc = strcmp(temp.surname, min_rec.surname);
            break;
          case 3 :
            rc = strcmp(temp.city, min_rec.city);
            break;
          default :
             printf("Invalid fieldNo\n" );
          }
          if(rc < 0){
            //printf("smaller\n");
            min_i = i;
            min_offset = pointers[i].offset;
            min_block = pointers[i].block;
            min_rec.id = temp.id;
            strcpy(min_rec.name,temp.name);
            strcpy(min_rec.surname,temp.surname);
            strcpy(min_rec.city,temp.city);
            //print_record(&min_rec);
          }
        }
      }

      SR_InsertEntry(after,min_rec);

      struct R next_top = next_rec(before, pointers[min_i].start_block, pointers[min_i].end_block, pointers[min_i].offset, pointers[min_i].block);

      pointers[min_i].offset = next_top.offset;
      pointers[min_i].block = next_top.block; 
      //printf("(min_i:%d)new pointer offset:%d & block:%d\n", min_i, pointers[min_i].offset, pointers[min_i].block);
  
      //SR_PrintAllEntries(after);
      //printf("Checking record:%d\n", cr);
      //getchar();
      cr++;
    }
    else
    {
      //printf("no more records in every run!\n");
      //getchar();
    }
  }
  //printf("============================~ End Of Merge ~==============================\n");
  //getchar();
}