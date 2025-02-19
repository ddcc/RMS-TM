// IMPORTANT:  READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
// By downloading, copying, installing or using the software you agree
// to this license.  If you do not agree to this license, do not download,
// install, copy or use the software.
// 
// 
// Copyright (c) 2008 Northwestern University
// All rights reserved.
// 
// Redistribution of the software in source and binary forms,
// with or without modification, is permitted provided that the
// following conditions are met: 
// 
// 1       Redistributions of source code must retain the above copyright
//         notice, this list of conditions and the following disclaimer. 
// 
// 2       Redistributions in binary form must reproduce the above copyright
//         notice, this list of conditions and the following disclaimer in the
//         documentation and/or other materials provided with the distribution. 
// 
// 3       Neither the name of Northwestern University nor the names of its
//         contributors may be used to endorse or promote products derived
//         from this software without specific prior written permission.
//  
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
// IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// NORTHWESTERN UNIVERSITY OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.



// Compiler options:-
// -DBALT     = make trees balanced
#include <omp.h>
#include <iostream>
#include <fcntl.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>
#include <string.h>
#include <strings.h>

#include "Itemset.h"
#include "ListItemset.h"
#include "HashTree.h"
#include "Database.h"
#include "pardhp.h"

FILE *tmp;
int **lsupcnt;
int nthreads;

struct timeval tp;
#define MAXITER 30
#define MAXTRANSZ 100


int MINSUPPORT;
double MINSUP_PER = 0.25;
int NUM_INSERT;
int NUM_ACTUAL_INSERT;

HashTree *Candidate = NULL;
ListItemset *Largelist = NULL;

char infile[500], outfile[500], off_file[100];

FILE *summary;
uint64_t ts_uint, te_uint, *t_io_uint;
uint64_t max_io_time;

int **start, **enda;
int **hash_pos;
int *max_trans_sz;

int threshold=2;
int more;

int *hash_indx = NULL;
int tot_cand = 0;
int *local_tot_cand;


transaction *data_set;

atomic_section * first_atomic_block;
atomic_section * second_atomic_block;
atomic_section * third_atomic_block;
all_atomic_time * atomic_sections_time;

void parse_args(int argc, char **argv)
{
   extern char * optarg;
   int c;
   
   if (argc < 5){
      cout << "usage: -i<infile> -f<offset_file> -s<support> -n<number of threads>\n";
      cout << "-t<threshold>\n";
      exit(3);
   }
   else{      
      while ((c=getopt(argc,argv,"b:i:f:o:s:t:n:"))!=-1){
         switch(c){
         case 'b':
            DBASE_BUFFER_SIZE = atoi(optarg);
            break;
         case 'i':
            strcpy(infile,optarg);
            break;
         case 'f':
            strcpy(off_file, optarg);
            break; 
         case 'o':
            strcpy(outfile, optarg);
            break;
         case 's':
            MINSUP_PER = atof(optarg);
            break;
         case 't':
            threshold = atoi(optarg);
            break;
         case 'n':
            nthreads = atoi(optarg);
            break;
         }
      }
   }
}

#ifdef BALT
void form_hash_indx(int hash_function)
{
   int i, cnt;
   i=0;

   printf("HASH_FUNCTION = %d\n", hash_function);
   if (hash_function == 1)
   {
      return;
   }

   while(i < DBASE_MAXITEM){
      for(cnt = 0; i < DBASE_MAXITEM && cnt < hash_function; i++)
         if (hash_indx[i] == 0){
            hash_indx[i] = cnt;
            cnt++;
         }
      for(cnt = hash_function-1;i < DBASE_MAXITEM && cnt >= 0; i++)
         if (hash_indx[i] == 0){
            hash_indx[i] = cnt;
            cnt--;
         }
   }
}
#endif

int init_subsets(int *starts, int *endas, int num_item, int k_item)
{
   int i;
   
   if (num_item < k_item) return 0;
   
   for (i=0; i < k_item; i++){
      starts[i] = i;
      endas[i] = num_item - k_item + 1 + i;
   }
   return 1;
}

int get_next_subset(int *starts, int *endas, int k_item)
{
   int i,j;
   
   for (i=k_item-1; i >= 0; i--){
      starts[i]++;
      if (starts[i] < endas[i]){
         for (j=i+1; j < k_item; j++)
            starts[j] = starts[j-1]+1;
         return 1;
      }
   }
   return 0;
}

ListElement * find_in_list(Itemset *item, int sz, ListElement *head, int pid)
{
   for(;head; head = head->next()){
      Itemset *curr = head->item();
      for(int i=0; i < sz; i++){
         int it = item->item((start[pid])[i]);
         if (curr->item(i) < it)
            break;
         else if (curr->item(i) > it)
            return NULL;
         else if (i==sz-1) return head;
      }
   }
   return NULL;
}


int not_prune(Itemset *curr, int k, ListElement *beg, int pid)
{
   if (k+1 == 3){
      start[pid][0] = 1;
      start[pid][1] = 2;
      if ((beg = find_in_list(curr, k, beg, pid)) == NULL) return 0;
   }
   else{
      int res = init_subsets(start[pid], enda[pid], curr->numitems(), k);
      start[pid][k-2] = curr->numitems()-2;
      start[pid][k-1] = curr->numitems()-1;
      while (res){
         if ((beg = find_in_list(curr, k, beg, pid)) == NULL) return 0;
         res = get_next_subset(start[pid], enda[pid], k);
      }
   }
   return 1;
}

int choose(int n, int k)
{
   int i;
   int val = 1;

   if (k >= 0 && k <= n){
      for (i=n; i > n-k; i--)
         val *= i;
      for (i=2; i <= k; i++)
         val /= i;
   }
   return val;
}

int get_hash_function(int num, int k){
   int hash = (int)ceil(pow(num/threshold, 1.0/k));
   if (hash < 1) hash = 1;
   return hash;
}


void assign_lsupcnt_offset(HashTree *node, int &val)
{
   if (node->is_leaf()){
      ListItemset *list = node->list();
      if(list && list->first()){
         ListElement *iter = list->first();
         for(;iter;iter = iter->next()){
            iter->item()->set_sup(val++);
         }
      }
   }
   else{
      for(int i=0; i < node->hash_function(); i++)
         if (node->hash_table(i))
            assign_lsupcnt_offset(node->hash_table(i), val);
   } 
}

int apriori_gen(int k, int pid, int *max_nested_depth){
   int tt;
   long blk = (Largelist->numitems()+nthreads-1)/nthreads;
   int lb = pid*blk;
   int ub = min((pid+1)*blk, Largelist->numitems());
   
   int * nestedLock_depth;
   nestedLock_depth = (int*) calloc(nthreads, sizeof(int));

   ListElement *L1iter = Largelist->node(lb);
   printf("Apriori_gen pid= %d, lb= %d, ub= %d, Largelist->numitems= %d\n", pid, lb, ub, Largelist->numitems());

   
   for (int i=lb; i < ub && L1iter; i++, L1iter = L1iter->next())
   {
      Itemset *temp = L1iter->item();
      ListElement *L2iter = L1iter->next();
      for(;L2iter; L2iter = L2iter->next()){
         Itemset *temp2 = L2iter->item();
         if (temp->compare(*temp2,k-2) < 0) break;
         else{
           int k1 = temp->item(k-2);
           int k2 = temp2->item(k-2);
            if (k1 < k2)
            {
               Itemset *it = new Itemset(k);
               it->set_numitems(k);
               for (int l=0; l < temp->numitems(); l++)
                  it->add_item(l,temp->item(l));
               it->add_item(k-1, k2);
               ListElement *beg = Largelist->first();
               if(k==2 || not_prune(it, k-1, beg, pid))
               {
                 for (int m=0; m<nthreads; m++)
 			nestedLock_depth[m] = 0;

                 tt = Candidate->add_element(*it,first_atomic_block, second_atomic_block, third_atomic_block,atomic_sections_time,nestedLock_depth,max_nested_depth, pid);
                 local_tot_cand[pid]++;
               }
            }
         }
      }
   }
#pragma omp barrier
   tt= 0;
   assign_lsupcnt_offset(Candidate,tt);
   free (nestedLock_depth);
   return tt;
}


void increment(Itemset *trans, ListItemset *Clist, int pid, char *tbitvec)
{
   if (Clist->first()){
      ListElement *head = Clist->first();
      for(;head; head = head->next()){
         Itemset *temp = head->item();
         if (temp->subsequence(tbitvec, trans->numitems())){
            lsupcnt[pid][temp->sup()]++;
         }
      }
   }
}

void subset(Itemset *trans, int st, int en, int final,
            HashTree* node, int k, int level, int pid, 
            char *tbitvec, int *vist, int hash_function)
{
   int i;

   (*vist)++;
   int myvist = *vist;

   if (node == Candidate 
       && node->is_leaf() && node->list())
      increment(trans, node->list(), pid, tbitvec);
   else{
      for(i=st; i < en; i++){
#ifdef BALT
         int val = trans->item(i);
         int hashval = hash_indx[val];
         if (hashval == -1) continue;
#else
         int val = trans->item(i);
         if (hash_indx[val] == -1) continue;
         int hashval = node->hash(val);
#endif
         if ((hash_pos[pid])[level*hash_function+hashval] != myvist){
            (hash_pos[pid])[level*hash_function+hashval] = myvist;
            if (node->hash_table_exists() && node->hash_table(hashval)){
               if (node->hash_table(hashval)->is_leaf() &&
                   node->hash_table(hashval)->list())
                  increment(trans, node->hash_table(hashval)->list()
                            ,pid, tbitvec);
               else if (en+1 <= final)
                  subset(trans, i+1, en+1, final,node->hash_table(hashval),
                         k, level+1, pid, tbitvec, vist, hash_function);
            }
         }
      }
   }
}


void form_large(HashTree *node,int k,int &cnt, int *cntary)
{
   if (node->is_leaf()){
      ListItemset *list = node->list();
      if(list && list->first()){
         ListElement *iter = list->first();
         for(;iter;iter = iter->next()){
            iter->item()->set_sup(cntary[cnt++]); 
            if (iter->item()->sup() >= MINSUPPORT){
               Largelist->sortedInsert(iter->item());
               for (int j=0; j < iter->item()->numitems(); j++)
                   hash_indx[iter->item()->item(j)] = 0;
            }      
         }
      }
   }
   else{
      for(int i=0; i < node->hash_function(); i++)
         if (node->hash_table(i))
            form_large(node->hash_table(i), k, cnt, cntary);
   } 
}

void alloc_mem_for_var(int pid)
{
   start[pid] = new int [MAXITER];
   enda[pid] = new int [MAXITER];
}

void sum_counts(int *destary, int **srcary, int pid, int ub)
{
   int i,j;
   
   for(i=1; i < nthreads; i++){
      for(j=0; j < ub; j++){
         destary[j] += srcary[i][j];
      }
   }
}

void *main_proc(void *)
{
   int vist, hash_function;
   Dbase_Ctrl_Blk DCB;  
   HashTree *oldCand=NULL;
   int k, i, j,pid;
   int *buf, tid, numitem, idx;
   int *cnt_ary;
   char *trans_bitvec;
   int blk, lb, ub;
   int *offsets;
   int *file_offset;
   FILE *off_fp;
   
   //Gokcen
   uint64_t parallel_sec_start_uint, parallel_sec_end_uint;

   int * max_nested_depth;
   int max_depth;
   int pid_max_depth;

   t_io_uint = (uint64_t *) calloc(nthreads, sizeof(uint64_t));

   first_atomic_block = (atomic_section*) calloc(nthreads, sizeof(atomic_section));
   second_atomic_block = (atomic_section*) calloc(nthreads, sizeof(atomic_section));
   third_atomic_block = (atomic_section*) calloc(nthreads, sizeof(atomic_section));
   atomic_sections_time = (all_atomic_time*) calloc (nthreads, sizeof(all_atomic_time));

   max_nested_depth = (int*) calloc(nthreads, sizeof(int));
   //Gokcen_end

   local_tot_cand=(int *) calloc(nthreads, sizeof(int));
   file_offset = (int *) calloc(nthreads+1, sizeof(int));
   off_fp = fopen(off_file, "r");

   for (i=0; i<nthreads+1; i++)
     fscanf(off_fp,"%d\n", &(file_offset[i]));

  ts_uint = rdtsc();
  parallel_sec_start_uint = rdtsc();
  omp_set_num_threads(nthreads);
  
  #pragma omp parallel private(DCB, k,vist, lb, ub, pid, buf, numitem, tid, \
               i, j, trans_bitvec)
 
{
   pid = omp_get_thread_num();
   max_trans_sz[pid] = Database_readfrom(infile, lsupcnt[pid], offsets, pid, file_offset[pid], file_offset[pid+1]- file_offset[pid]);

   alloc_mem_for_var(pid);
#pragma omp barrier
   if (pid==0)
   { 
      hash_indx = new int [1000];
      cnt_ary = new int[DBASE_MAXITEM * (DBASE_MAXITEM -1)/2];
      for (i=0; i<(DBASE_MAXITEM * (DBASE_MAXITEM -1)/2); i++)
        cnt_ary[i]=lsupcnt[0][i];
      for (j=1; j<nthreads; j++)
          for (i=0; i<(DBASE_MAXITEM * (DBASE_MAXITEM -1)/2); i++)
              cnt_ary[i] += lsupcnt[j][i];
      
      offsets = new int [DBASE_MAXITEM];
      int offt = 0;
      for (i=DBASE_MAXITEM-1; i >= 0; i--){
          offsets[DBASE_MAXITEM-i-1] = offt;
          offt += i;
      }
      
      for(i=0; i < DBASE_MAXITEM; i++)
         hash_indx[i] = -1;
      for(i=0; i < DBASE_MAXITEM-1; i++){
         idx = offsets[i]-i-1;
         for (j=i+1; j < DBASE_MAXITEM; j++){
             if (cnt_ary[idx+j] >= MINSUPPORT){
                hash_indx[i] = 0;
                hash_indx[j] = 0;
                Itemset *it = new Itemset(2);
                it->set_numitems(2);
                it->add_item(0,i);
                it->add_item(1,j);
                it->set_sup(cnt_ary[idx+j]);
                Largelist->append(*it);
                local_tot_cand[pid]++;
             }
         }
      }
      NUM_INSERT = choose(Largelist->numitems(),2);

      //fprintf(summary, "(2.ITER)it= %d\n", Largelist->numitems());
      printf("(2.ITER)it= %d\n", Largelist->numitems());
      hash_function = get_hash_function(NUM_INSERT,2);
      Candidate = new HashTree(0, hash_function, threshold);
#ifdef BALT
      more = (NUM_INSERT > 0);
      form_hash_indx(hash_function);
#endif

   }

#pragma omp barrier
   blk = (DBASE_NUM_TRANS+nthreads - 1)/nthreads;
  
   Itemset *trans;
   trans = new Itemset(max_trans_sz[pid]);
   trans_bitvec = new char[DBASE_MAXITEM];
   for (i=0; i < DBASE_MAXITEM; i++){
       trans_bitvec[i] = 0;
   }
   lb = pid*blk;
   ub = min((pid+1)*blk, DBASE_NUM_TRANS);
   printf ("pid= %d, NUM_INSERT= %d\n",pid, NUM_INSERT);

   for(k=3;more;k++) 
   {
      
      if (pid==0) 
        {
          printf("*****************ITER %d\n",k);
          printf("*****************MORE %d\n",more);
        }
      if (hash_pos[pid]) delete [] hash_pos[pid];
      hash_pos[pid] = new int [(k+1)*hash_function];
      for (i=0; i<(k+1)*hash_function; i++)
          hash_pos[pid][i]=0;

      if (pid==0)
      {
         NUM_ACTUAL_INSERT = apriori_gen(k, pid,max_nested_depth);
      }
      else
      {
         apriori_gen(k, pid,max_nested_depth);
      }
#pragma omp barrier
      if (pid==0) 
      { 
        #ifndef EQ
        Largelist->clearlist();
        #endif
        if (oldCand) delete oldCand;
      }
#pragma omp barrier

      if (lsupcnt[pid]) delete [] lsupcnt[pid];
      lsupcnt[pid] = new int [NUM_ACTUAL_INSERT+1];
      for (i=0; i<NUM_ACTUAL_INSERT+1; i++)
        lsupcnt[pid][i] = 0;
      vist = 1;
      if (more)
      {
        for(i=lb; i < ub;i++)
        {

        make_Itemset(trans, data_set[i].item_list, data_set[i].numitem, data_set[i].tid);
        for (j=0; j < trans->numitems(); j++)
          trans_bitvec[trans->item(j)] = 1;

        subset(trans, 0, trans->numitems()-k+1, trans->numitems(), 
                   Candidate,
                   k, 0, pid, trans_bitvec, &vist, hash_function);
        for (j=0; j < trans->numitems(); j++)
          trans_bitvec[trans->item(j)] = 0;  

        }
      }
#pragma omp barrier
      if (pid==0)
      { 
        for (i=0; i < DBASE_MAXITEM; i++) hash_indx[i] = -1;
      
        delete [] cnt_ary;      
        cnt_ary = new int[NUM_ACTUAL_INSERT+1];
        for (i=0; i<NUM_ACTUAL_INSERT+1; i++)
        cnt_ary[i]=lsupcnt[0][i];
        for (j=1; j<nthreads; j++)
          for (i=0; i<NUM_ACTUAL_INSERT+1; i++)
            cnt_ary[i] += lsupcnt[j][i];
        int ccnntt = 0;
        form_large(Candidate,k, ccnntt, cnt_ary);

        //fprintf(summary, "(%d .ITER)it= %d\n", k, Largelist->numitems());
        printf("(%d .ITER)it= %d\n", k, Largelist->numitems());
        more = (Largelist->numitems() > 1);
        NUM_INSERT = choose(Largelist->numitems(),2);
#ifdef BALT
        form_hash_indx(get_hash_function(NUM_INSERT,k+1));
#endif
        oldCand = Candidate;
        Candidate = new HashTree(0, get_hash_function(NUM_INSERT,k+1), threshold);
        hash_function = get_hash_function(NUM_INSERT,k+1);

      }
#pragma omp barrier
    }
}
  // delete [] trans_bitvec;
//   close_DCB(DCB);
   parallel_sec_end_uint = rdtsc();
   te_uint = rdtsc();

   for (int i=0; i<nthreads; i++)
   {
     printf("*****************************\n");
     printf("THREAD_ID= %d\tFIRST_AS_ENTRANCE_NUMBER= %d\n", i,first_atomic_block[i].entrance);
     printf("THREAD_ID= %d\tFIRST_AS_MAX_WAIT= %lu\n",i, first_atomic_block[i].max_housekeeping_time);
     printf("THREAD_ID= %d\tFIRST_AS_MIN_WAIT= %lu\n",i,first_atomic_block[i].min_housekeeping_time);
     printf("THREAD_ID= %d\tFIRST_AS_MAX_COMMIT= %lu\n",i,first_atomic_block[i].max_executing_time);
     printf("THREAD_ID= %d\tFIRST_AS_MIN_COMMIT= %lu\n",i,first_atomic_block[i].min_executing_time);
     printf("THREAD_ID= %d\tFIRST_AS_TOTALTIME= %lu\n\n",i,first_atomic_block[i].total_atomicsec_time);
     
 
     printf("THREAD_ID= %d\tSECOND_AS_ENTRANCE_NUMBER= %d\n", i,second_atomic_block[i].entrance);
     printf("THREAD_ID= %d\tSECOND_AS_MAX_WAIT= %lu\n",i, second_atomic_block[i].max_housekeeping_time);
     printf("THREAD_ID= %d\tSECOND_AS_MIN_WAIT= %lu\n",i,second_atomic_block[i].min_housekeeping_time);
     printf("THREAD_ID= %d\tSECOND_AS_MAX_COMMIT= %lu\n",i,second_atomic_block[i].max_executing_time);
     printf("THREAD_ID= %d\tSECOND_AS_MIN_COMMIT= %lu\n",i,second_atomic_block[i].min_executing_time);
     printf("THREAD_ID= %d\tSECOND_AS_TOTALTIME= %lu\n\n",i,second_atomic_block[i].total_atomicsec_time);
     

     printf("THREAD_ID= %d\tTHIRD_AS_ENTRANCE_NUMBER= %d\n", i,third_atomic_block[i].entrance);
     printf("THREAD_ID= %d\tTHIRD_AS_MAX_WAIT= %lu\n",i, third_atomic_block[i].max_housekeeping_time);
     printf("THREAD_ID= %d\tTHIRD_AS_MIN_WAIT= %lu\n",i,third_atomic_block[i].min_housekeeping_time);
     printf("THREAD_ID= %d\tTHIRD_AS_MAX_COMMIT= %lu\n",i,third_atomic_block[i].max_executing_time);
     printf("THREAD_ID= %d\tTHIRD_AS_MIN_COMMIT= %lu\n",i,third_atomic_block[i].min_executing_time);
     printf("THREAD_ID= %d\tTHIRD_AS_TOTALTIME= %lu\n\n",i,third_atomic_block[i].total_atomicsec_time);
     

     printf("THREAD_ID= %d\tTOTAL_TIME_FOR_ALL_SECTIONS= %lu\n",i,first_atomic_block[i].total_atomicsec_time+second_atomic_block[i].total_atomicsec_time+third_atomic_block[i].total_atomicsec_time); 
     printf("*****************************\n\n");
   }

   uint64_t max_critical_section_time = 0; 
   uint64_t total_critical_section_time_AS = 0;

   for (int i=0; i<nthreads; i++)
   {
     printf("THREAD_ID= %d\tTOTAL_WAITING_TIME_FOR_ACQURING_LOCK= %lu\n", i, atomic_sections_time[i].total_housekeeping_time);
     printf("THREAD_ID= %d\tTOTAL_EXECUTING_TIME= %lu\n", i, atomic_sections_time[i].total_executing_time);
     printf("\t\tTHREAD_ID= %d\tTOTAL_TIME_WITH_ALL_WAITING_TIME= %lu\n", i,atomic_sections_time[i].total_atomic_time);
  
    max_critical_section_time = max(atomic_sections_time[i].total_atomic_time,max_critical_section_time);
   }

   printf("\n\n************NESTED_DEPTH_VALUES*****************\n");
   printf(" %d\tTHREAD_ID\t%d\tNESTED_DEPTH_VALUE\n", 0,max_nested_depth[0]);
   max_depth = max_nested_depth[0];
   pid_max_depth = 0;
   for (i=1; i<nthreads; i++)
   {
     printf(" %d\tTHREAD_ID\t%d\tNESTED_DEPTH_VALUE\n", i,max_nested_depth[i]);
     if (max_nested_depth[i] > max_depth)
     {
       max_depth = max_nested_depth[i];
       pid_max_depth = i;
     }
   }

   printf ("NUM_ACTUAL_INSERT= %d\n",NUM_ACTUAL_INSERT);
   for (i=0; i<nthreads; i++) 
      tot_cand +=local_tot_cand[i];

   printf("PARALLEL_SECTION(rdtsc)= %lu\n", parallel_sec_end_uint - parallel_sec_start_uint);
   printf("MAX_ATOMIC_SECTION_TIME= %lu\n",max_critical_section_time);
   printf("ATOMIC_SECTION_RATE= %f\n",(double(max_critical_section_time*100)/double(te_uint-ts_uint-max_io_time)));
   printf("MAX_NESTED_DEPTH= %d\n",max_depth);
 
   free (t_io_uint);
   free (first_atomic_block);
   free (second_atomic_block);
   free (third_atomic_block);
   free (max_nested_depth);
   free (local_tot_cand);
   free (file_offset);
   return NULL;
}

void init_var()
{
   int i;
   Largelist = new ListItemset;

   start = new int *[nthreads];
   enda = new int *[nthreads];
   hash_pos = new int *[nthreads];
   for (i=0; i < nthreads; i++) hash_pos[i] = NULL;

   max_trans_sz = new int[nthreads];
   for (i=0; i < nthreads; i++) max_trans_sz[i] = 0;

   lsupcnt = new int *[nthreads];
   for (i=0; i < nthreads; i++) lsupcnt[i] = NULL;
   more = 1;
}

void clean_up(){
   int i;

#pragma omp parallel
{
   TM_THREAD_EXIT();
#pragma omp barrier
}

   delete Candidate;
   if (Largelist) delete Largelist;
   delete [] hash_indx;

   for (i=0; i < nthreads; i++){
      delete start[i];
      delete enda[i];
      delete hash_pos[i];
   }
   delete [] start;
   delete [] enda;
   delete [] hash_pos;
   for (i=0; i < nthreads; i++) delete lsupcnt[i];
   delete [] lsupcnt;
}

int main(int argc, char **argv)
{
   char sumfile[100];
   parse_args(argc, argv);
   

   sprintf(sumfile, "temp");
   if ((summary = fopen (sumfile, "a+")) == NULL){
      printf("can't open %s\n", sumfile);
      exit(-1);
   }

  
   init_var();
   fprintf(summary, "Database %s sup %f\n",
           infile, MINSUP_PER);
   printf("Database %s sup %f\n",
           infile, MINSUP_PER);
   fprintf(summary, "nthreads= %d\n ", nthreads);
   printf("nthreads= %d\n", nthreads);

   if (more) main_proc(NULL);
  
 
   max_io_time = t_io_uint[0];
   for (int j=1; j<nthreads; j++)
     if (t_io_uint[j]>max_io_time) max_io_time = t_io_uint[j];
 
   printf("Cands= %d\n",tot_cand);
   printf("TOTAL_EXECUTION_TIME(rdtsc)= %lu\n",te_uint-ts_uint);
   printf("IO_TIME(rdtsc)= %lu\n",max_io_time);
   printf("COMPUTATION_TIME(rdtsc)= %lu\n",te_uint-ts_uint-max_io_time);
   fflush(summary);
   fflush(stdout);
   clean_up();
   fclose(summary);
   exit(0);
}



























