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
#include "freertos/semphr.h"
#include "jobs.h"
#include "cron.h"

static struct
{
  unsigned char running;
  TaskHandle_t handle;
  time_t seconds_until_next_execution;
  SemaphoreHandle_t semaphore;
} state = {
    .running = 0,
    .handle = NULL,
    .seconds_until_next_execution = -1,
    .semaphore = NULL
};

static inline int cron_job_lock(void)
{
  if (NULL == state.semaphore) {
    state.semaphore = xSemaphoreCreateMutex();
  }
  
  return ((NULL != state.semaphore) && (pdTRUE == xSemaphoreTake(state.semaphore, 100)));
}

static inline void cron_job_unlock(void)
{
  if (NULL != state.semaphore) {
    xSemaphoreGive(state.semaphore);
  }
}

void cron_job_init(){
  if(state.semaphore == NULL) state.semaphore = xSemaphoreCreateMutex();
}

int cron_job_is_running(){
  return state.running;
}

cron_job * cron_job_create(const char *schedule, cron_job_callback callback, void *data)
{
  if (cron_job_is_running())
  {
    return NULL;
  }

  cron_job_list_init(); // CALL THIS ON ANY CREATE
  cron_job *job = calloc(sizeof(cron_job), 1);
  if (job == NULL)
    goto end;
  job->callback = callback;
  job->data = data;
  job->id = -1;
  job->load = NULL;
  if(cron_job_load_expression(job, schedule) != 0) {
    free(job);
    return NULL;
  }
  cron_job_schedule(job);
  goto end;

end:
  return job;
}



enum cron_job_errors cron_job_destroy(cron_job *job)
{
  int ret = Cron_ok;
  if (cron_job_is_running())
  {
    ret = Cron_not_stopped;
    goto end;
  }
  if (job == NULL)
  {
    ret = Cron_bad_job;
    goto end;
  }
  ret = cron_job_unschedule(job);
  free(job);
  job = NULL;
  goto end;
end:
  return ret;
}

enum cron_job_errors cron_job_clear_all()
{
  int ret = Cron_ok;
  if (cron_job_is_running())
  {
    return Cron_not_stopped;
  }
  // REFACTOR THIS!
  while (cron_job_list_first())
  {
    ret = cron_job_destroy(cron_job_list_first()->job);
    if (ret != 0)
      break;
  }
  cron_job_list_reset_id();
  goto end;
  end:
    return ret;
}

enum cron_job_errors cron_stop()
{
  if (!cron_job_is_running())
  {
    return Cron_is_stopped;
  }
  vTaskDelay(20/portTICK_PERIOD_MS);
  TaskHandle_t xHandle = NULL;
  if (cron_job_lock())
  {
    xHandle = state.handle;
    state.handle = NULL;
    cron_job_unlock();
    state.running = 0;
    if (xHandle != NULL)
    {
      vTaskDelete(xHandle);
    }
  } else {
    return Cron_no_sempahore;
  }
  return Cron_ok;
}

enum cron_job_errors cron_start()
{
  if (cron_job_is_running())
  {
    return Cron_not_stopped;
  } else if ( state.handle != NULL) {
    return Cron_scheduler_task_handle_set_but_stopped;
  }
  // this vTaskDelay is used to wait for the cron 
  vTaskDelay(20/portTICK_PERIOD_MS);
  /* Create the task, storing the handle. */
  BaseType_t xReturned = xTaskCreatePinnedToCore(
      cron_schedule_task,   /* Function that implements the task. */
      "cron_schedule_task", /* Text name for the task. */
      4096,                 /* Stack size in words, not bytes. */
      (void *)0,            /* Parameter passed into the task. */
      tskIDLE_PRIORITY + 2, /* Priority at which the task is created. */
      &state.handle,        /* Save the handle */
      tskNO_AFFINITY);      /* NO SPECIFIC CORE */
  if (xReturned != pdPASS)
  {
    return Cron_unable_to_create_secheduler_task;
  }
  state.running = 1;
  return Cron_ok;
}

enum cron_job_errors cron_job_sort()
{
  if (cron_job_is_running())
  {
    return Cron_not_stopped;
  }
  cron_job *job = NULL;
  int count = cron_job_node_count();
  cron_job ** job_list = malloc(sizeof(cron_job*)*count);
  for(int i = 0; i<count; i++){
    job = cron_job_list_first()->job;
    cron_job_list_remove(job->id);
    job_list[i] = job;
  }

  for(int i=0; i<count; i++){
    cron_job_schedule(job_list[i]);
  }
  free(job_list);
  return Cron_ok;
}

enum cron_job_errors cron_job_schedule(cron_job *job)
{

  if (job == NULL)
  {
    return Cron_bad_job;
  }
  else if (cron_job_has_loaded(job) < 0)
  {
    return Cron_error_in_load_expression;
  }
  time_t now;
  time(&now);
  job->next_execution = cron_next(&(job->expression), now);
  int id = cron_job_list_insert(job);
  if (id < 0)
  {
    return Cron_bad_id;
  }

  return Cron_ok;
}

enum cron_job_errors cron_job_unschedule(cron_job *job)
{
  int ret = Cron_ok;
  if (cron_job_is_running())
  {
    ret = Cron_not_stopped;
  }
  else if (job == NULL)
  {
    ret = Cron_bad_job;
  }
  else if (job->id >= 0)
  {
    ret = cron_job_list_remove(job->id);
  }
  return ret;
}

enum cron_job_errors cron_job_load_expression(cron_job *job, const char *schedule)
{
  const char *error = NULL;
  
  if (schedule != NULL)
  {
    memset(&(job->expression), 0, sizeof(job->expression));
    cron_parse_expr(schedule, &(job->expression), &error);
    if(error != NULL){
      return Cron_parse_expresion_error;
    }
    job->load = &(job->expression);
    return Cron_ok;
  } else {
    return Cron_bad_schedule;
  }
  return Cron_fail;
}

enum cron_job_errors cron_job_has_loaded(cron_job *job)
{
  if(job == NULL){
    return Cron_bad_job;
  }
  if (&(job->expression) == job->load)
  {
    return Cron_ok;
  } else {
    return Cron_expresion_not_loaded;
  }
  return Cron_fail;
}

time_t cron_job_seconds_until_next_execution()
{
  return state.seconds_until_next_execution;
}

// CRON TASKS

void cron_schedule_job_launcher(void *args)
{
  if (args == NULL)
  {
    goto end;
  }
  cron_job *job = (cron_job *)args;
  job->callback(job);
  goto end;

end:

  vTaskDelete(NULL);
  return;// WONT RUN
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
  } else {
    cron_job_sort();
  }
  while (true)
  {
    time(&now);
    if (cron_job_list_first() == NULL)
    {

      break;
    }

    job = cron_job_list_first()->job;

    if (job == NULL)
    {
      break; // THIS IS IT!!! THIS WILL
    }
    if (now >= job->next_execution)
    {
      if (job->callback != NULL)
      {
        /* Create the task, IT WILL KILL ITSELF AFTER THE JOB IS DONE. */
        xTaskCreatePinnedToCore(
            cron_schedule_job_launcher,   /* Function that implements the task. */
            "cron_schedule_job_launcher", /* Text name for the task. */
            (4096*2),                         /* Stack size in BYTES, not bytes. */
            (void *)job,                  /* Job is passed into the task. */
            tskIDLE_PRIORITY + 2,         /* Priority at which the task is created. */
            (NULL),                       /* No need for the handle */
            tskNO_AFFINITY);              /* No specific core */
      }
      if (cron_job_lock()){
        cron_job_list_remove(job->id); // There is mutex in there that can mess with our timing, but i am not sure if we should move this to the new task.
        cron_job_schedule(job); // There is mutex in there that can mess with our timing, but i am not sure if we should move this to the new task.
        cron_job_unlock();
      }
    }
    else
    {
      state.seconds_until_next_execution = job->next_execution - now;
      vTaskDelay((state.seconds_until_next_execution * 1000) / portTICK_PERIOD_MS);
    }

    if (r1 != 0)
    {
      return;
    }
  }
  // r1 is the testing variable
 
  if (cron_job_lock()){
    state.running = 0;
    state.handle = NULL;
    cron_job_unlock(); // this give shall be placed before the vTaskDelete
  }
  if (r1!=1) {
    vTaskDelete(NULL);// THIS MEANS THIS IS RUNNING IN A DIFFERENT TASK
  }
  return;
}
