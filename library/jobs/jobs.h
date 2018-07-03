// Copyright 2018 Insite SAS 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//
//
// Author: David Mora Rodriguez dmorar (at) insite.com.co
//

#ifndef _ESP_CRON_JOBS_LINKED_LIST
#define _ESP_CRON_JOBS_LINKED_LIST
#include <time.h>
#include "cron.h"

struct cron_job_node {
  struct cron_job_node * next;
  cron_job * job;
};

/*
*  SUMMARY: Returns the first element in the linkedlist. 
*
*  PARAMS: Time input (timestamp) 
*
*  RETURNS:  hour 
*/

struct cron_job_node * cron_job_list_first();
/*
*  SUMMARY: Adds a job to the list in execution order. 
*
*  PARAMS: job
*
*  RETURNS:  id or -1 on error 
*/
int cron_job_list_insert(cron_job * job);
/*
*  SUMMARY: Removes a node from the list. 
*  Note that for a new node the id must be -1 and this method will give you a new id.
*  Ids start at zero and grow from there, so job->id must be set to -1.
*
*  PARAMS: id for the node
*
*  RETURNS: 0 on success, -1 on not found
*/
int cron_job_list_remove(int id);


/*
*  SUMMARY:Counts elements on list o(N). 
*
*  PARAMS: NONE
*
*  RETURNS: number of nodes on list
*/
int cron_job_node_count();
/*
*  SUMMARY: initializes the needed structures for the module to work (like mutex). It is safe to call it multiple times.
*
*  PARAMS: NONE
*
*  RETURNS:none
*/
void cron_job_list_init();
/*
*  SUMMARY: If the node count is zero then make id 0 to start over.
*
*  PARAMS: NONE
*
*  RETURNS: 0 on success, -1 on no action
*/

int cron_job_list_reset_id();

#endif 