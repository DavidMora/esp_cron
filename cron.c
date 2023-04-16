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
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cron.h"
#include "jobs.h"


static struct
{
  unsigned char running;
  TaskHandle_t handle;
  time_t seconds_until_next_execution;

} state = {
    .running = 0,
    .handle = NULL,
    .seconds_until_next_execution = -1
  };

cron_job *cron_job_create(const char *schedule, cron_job_callback callback, void *data)
{
  cron_job_list_init();// CALL THIS ON ANY CREATE
  cron_job *job = calloc(sizeof(cron_job), 1);
  if (job == NULL)
    goto end;
  job->callback = callback;
  job->data = data;
  job->id = -1;
  job->load=NULL;
  cron_job_load_expression(job, schedule);
  cron_job_schedule(job);
  goto end;

end:
  return job;
}

int cron_job_destroy(cron_job *job)
{
  int ret=0;
  if (job == NULL) {
    ret = -1;
    goto end;
  }
  ret = cron_job_unschedule(job);
  free(job);
end:
  return ret;
}

int cron_job_clear_all()
{
  // REFACTOR THIS!
  while (cron_job_list_first())
  {
   cron_job_destroy(cron_job_list_first()->job);
  }
  cron_job_list_reset_id();
  return 0;
}

int cron_stop()
{
  if (state.running == 0)
  {
    return -1;
  }
  TaskHandle_t xHandle;
  state.running = 0;
  xHandle = state.handle;
  state.handle = NULL;
  if (xHandle != NULL) {
    
    vTaskDelete(xHandle);
  }
  return 0;
}

int cron_start()
{
  BaseType_t xReturned;
  if (state.running == 1 || state.handle != NULL)
  {
    return -1;
  }

  /* Create the task, storing the handle. */
  xReturned = xTaskCreatePinnedToCore(
      cron_schedule_task,   /* Function that implements the task. */
      "cron_schedule_task", /* Text name for the task. */
      4096,                 /* Stack size in words, not bytes. */
      (void *)0,            /* Parameter passed into the task. */
      tskIDLE_PRIORITY + 2, /* Priority at which the task is created. */
      &state.handle,        /* Save handle for further task delete. */
      tskNO_AFFINITY);      /* NO SPECIFIC CORE */
  if (xReturned != pdPASS)
  {
    /* The task was created.  Use the task's handle to delete the task. */
  }
  state.running = 1;
  return 0;
}


int cron_job_schedule(cron_job *job)
{
  if (job == NULL)
  {
    return -1;
  }
  if (!cron_job_has_loaded(job))
  {
    return -1;
  }
  time_t now;
  time(&now);
  job->next_execution = cron_next(&(job->expression), now);
  int id = cron_job_list_insert(job);
  if (id < 0)
  {
    return -1;
  }
  else
  {
    return 0;
  }

  return 0; // WONT RUN
}

int cron_job_unschedule(cron_job *job)
{
  int ret = 0;
  if (job == NULL)
  {
    ret = -1;
  }
  else if (job->id >= 0)
  {
    ret = cron_job_list_remove(job->id);
  }
  return ret;
}


int cron_job_load_expression(cron_job *job, const char * schedule)
{
  const char *error = NULL;

  if (schedule != NULL)
  {
    memset(&(job->expression), 0, sizeof(job->expression));
    cron_parse_expr(schedule, &(job->expression), &error);
    job->load = &(job->expression);
  }
  return 0;
}

int cron_job_has_loaded(cron_job *job)
{
  if (&(job->expression) == job->load)
  {
    return 1;
  }
  else
  {
    return 0;
  }
  return 0; // WONT RUN
}

time_t cron_job_seconds_until_next_execution()
{
  return state.seconds_until_next_execution;
}



// CRON TASKS 


void cron_schedule_job_launcher(void * args){
  if (args==NULL) {
    goto end;
  }
  cron_job * job = (cron_job *) args;  
  job->callback(job);
  goto end;


  end:
    vTaskDelete(NULL);
    return;
}

void cron_schedule_task(void *args)
{
  time_t now;
  cron_job *job = NULL;
  int r1 = 0; // RUN ONCE!!
              // IF ARGS ARE A STRING DEFINED AS R1
  if (args != NULL)
  {
    if (strncmp(args, "R1", 2) == 0) // OK I ADMIT IT, ITS NOT THE MOST BEAUTIFUL CODE EVER, BUT I NEED IT TO BE TESTABLE... DON'T WANT TO GROW OLD WAITING FOR TIME TO PASS... :P
      r1 = 1;
  }

  while (true)
  {
    state.running = 1;
    time(&now);
    job = cron_job_list_first()->job;
    if (job == NULL)
    {
      break;// THIS IS IT!!! THIS WILL 
    }
    if (now >= job->next_execution)
    {
      /* Create the task, IT WILL KILL ITSELF AFTER THE JOB IS DONE. */
      xTaskCreatePinnedToCore(
      cron_schedule_job_launcher,   /* Function that implements the task. */
      "cron_schedule_job_launcher", /* Text name for the task. */
      4096,                 /* Stack size in BYTES, not bytes. */
      (void *)job,          /* Job is passed into the task. */
      tskIDLE_PRIORITY + 2, /* Priority at which the task is created. */
      (NULL),               /* No need for the handle */
      tskNO_AFFINITY);      /* No specific core */          
      cron_job_list_remove(job->id); // There is mutex in there that can mess with our timing, but i am not sure if we should move this to the new task. 
      cron_job_schedule(job); // There is mutex in there that can mess with our timing, but i am not sure if we should move this to the new task. 
    }
    else
    {
      state.seconds_until_next_execution = job->next_execution - now;
      vTaskDelay((state.seconds_until_next_execution * 1000) / portTICK_PERIOD_MS);
    }
    if (r1 != 0)
    {
      break;
    }
  }
  cron_stop();
  return;
}


