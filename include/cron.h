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

#ifndef ESP_CRON_JOB
#define ESP_CRON_JOB
#include "freertos/FreeRTOS.h"
#include <string.h>
#include "ccronexpr.h"



/*
*  STRUCT INFORMATION: Holds the information needed to run the cron job.
*  
*  - callback: Function pointer to be called on execution
*  - schedule: Cron syntax for the schedule 
*            ┌────────────── second (0 - 59)  
*            | ┌───────────── minute (0 - 59)
*            | │ ┌───────────── hour (0 - 23)
*            | │ │ ┌───────────── day of month (1 - 31)
*            | │ │ │ ┌───────────── month (1 - 12)
*            | │ │ │ │ ┌───────────── day of week (0 - 6) (Sunday to Saturday;
*            | │ │ │ │ │                                       7 is also Sunday on some systems)
*            | │ │ │ │ │
*            | │ │ │ │ │
*            * * * * * *  
*  - data: pointer to arbitrary data needed by the cron_job
*  - id: identifier in the module for this cron job, this allows to unschedule similar tasks
*  - next execution: this information holds the time when it will run next, is managed by the cron module
*  - see https://github.com/staticlibs/ccronexpr
*/

// FORWARD DECLARATIONS ARE NEEDED FOR CALLBACK DECLARATION INSIDE THE STRUCT
struct cron_job_struct; 
typedef struct cron_job_struct cron_job;

struct cron_job_struct 
{
  void (* callback)(cron_job *);
  cron_expr expression;
  void * data;
  int id;
  void * load;
  time_t next_execution;
};


// FUNCTION POINTER TO CALLBACKS
typedef void (*cron_job_callback)(cron_job *);


/*
*  SUMARY: Allocates a cron job on the heap with supplied parameters
*  
*  PARAMS: CRON SYNTAX SCHEDULE, CALLBACK (JOB), DATA FOR THE CALLBACK
*
*  RETURNS: heap allocated cron_job
*/

cron_job * cron_job_create(const char * schedule,cron_job_callback callback, void * data);

/*
*  SUMARY: Deallocates, and remove from scheduling 
*  
*  PARAMS: Cron job to deallocate
*
*  RETURNS: heap allocated cron_job
*/

int cron_job_destroy(cron_job * job);


/*
*  SUMMARY: Removes all cron_job from the module
*
*  PARAMS: removes all cron_jobs (no deallocation is performed by the call, memory must be handled out of this module)
*
*  RETURNS:  0 on success
*/

int cron_job_clear_all();

/*
*  SUMMARY: Starts the schedule module (creates a new task on the operating system)
*
*  RETURNS:  0 on success
*/

int cron_start();

/*
*  SUMMARY: Stops the schedule module (deletes the cron task on the operating system)
*
*  RETURNS:  0 on success
*/

int cron_stop();


/*
*  SUMMARY: Schedule a new cron_job
*
*  PARAMS: cron_job to be scheduled (no allocation is performed by the call, memory must be handled out of this module)
*
*  RETURNS:  0 on success
*/


int cron_job_schedule(cron_job * job);

/*
*  SUMMARY: Removes a cron_job from the schedule, this is ID based
*
*  PARAMS: cron_job to be removed (no deallocation is performed by the call, memory must be handled out of this module)
*
*  RETURNS:  0 on success, -1 on job NULL, -2 on JOB not loaded
*/

int cron_job_unschedule(cron_job * job);




/*
*  SUMMARY: Returns seconds until next execution
*  RETURNS:  seconds until next execution
*/

time_t cron_job_seconds_until_next_execution();

/*
*  SUMARY: TASK SCHEDULER WITH DELAYS AND CALL TO CALLBACKS DO NOT RUN THIS TAST
*  
*  PARAMS: ARGS MUST BE NULL IN REGULAR USE, IF YOU WANT TO RUN JUST ONCE ARGS MUST BE A CHAR POINTER TO "R1" LITERAL
*
*  RETURNS: NO RETURN
*/

void cron_schedule_task(void *args);
/*
*  SUMARY: Parses cron expression and sets the state structure
*
*  PARAMS: Cron job structure
*
*  RETURNS: NO RETURN
*/
int cron_job_load_expression(cron_job *job, const char * schedule);
/*
*  SUMARY: Checks if load expression has loaded
*  
*  PARAMS: Cron job structure
*
*  RETURNS: 1 if it has loaded, 0 on error
*/
int cron_job_has_loaded(cron_job *job);
/*
*  SUMARY: Returns seconds until next execution
*  
*  PARAMS: NONE
*
*  RETURNS: seconds until next execution
*/
time_t cron_job_seconds_until_next_execution();



#endif
