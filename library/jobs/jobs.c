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
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "jobs.h"

// STATIC FUNCTION DECLARATIONS
struct cron_job_node *_cron_job_list_insert(struct cron_job_node *next_node, struct cron_job_node *new_node);
// STATIC STRUCTS
static struct
{
  int next_id;
  struct cron_job_node *first;
  SemaphoreHandle_t semaphore;
  int init;

} linked_link_state = {
    .next_id = 0,
    .first = NULL,
    .semaphore = NULL,
    .init = 0
    };

void cron_job_list_init()
{
  if (linked_link_state.init == 0) {
    linked_link_state.semaphore = xSemaphoreCreateMutex();
    linked_link_state.init=1;
  }
}

struct cron_job_node *cron_job_list_first()
{
  return linked_link_state.first;
}

/* RECURSIVE BUT STACK EXHAUSTION? */
struct cron_job_node *_cron_job_list_insert(struct cron_job_node *next_node, struct cron_job_node *new_node)
{
  if (next_node == NULL || new_node->job->next_execution < next_node->job->next_execution)
  {
    new_node->next = next_node;
    return new_node;
  }
  else
  {
    next_node->next = _cron_job_list_insert(next_node->next, new_node);
  }
  return next_node;
}

int cron_job_list_insert(cron_job *job)
{
  if (linked_link_state.semaphore == NULL)
    cron_job_list_init();
  if (job == NULL)
  {
    return -1;
  }
  struct cron_job_node *new_node = calloc(sizeof(struct cron_job_node),1);
  if (new_node == NULL)
  {
    return -1;
  }
  new_node->job = job;
  if (xSemaphoreTake(linked_link_state.semaphore, (TickType_t)10) == pdTRUE)
  {
    linked_link_state.first = _cron_job_list_insert(linked_link_state.first, new_node);
    xSemaphoreGive(linked_link_state.semaphore);
  }
  else
  {
    free(new_node);
    return -1;
  }
  if (new_node->job->id == -1) // NOT INITIALIZED ON -1
    new_node->job->id = linked_link_state.next_id++;
  return new_node->job->id;
}

int cron_job_list_remove(int id)
{
  int ret = -1;
  struct cron_job_node *node = linked_link_state.first, *prev_node = NULL;
  if (xSemaphoreTake(linked_link_state.semaphore, (TickType_t)10) == pdTRUE) 
  {
    do
    {
      if (node->job->id == id)
      {
        if (node == linked_link_state.first)
        {
          linked_link_state.first = node->next;
        }
        else
        {
          prev_node->next = node->next;
        }
        free(node);
        node = NULL;

        ret = 0;
        break;
      }
      else
      {
        prev_node = node;
        node = node->next;
      }

    } while (node->next);
    xSemaphoreGive(linked_link_state.semaphore);
  }
  else
  {
    ret = -1;
  }
  return ret;
}

int cron_job_node_count()
{
  int cnt = 0;
  struct cron_job_node *node = cron_job_list_first();
  if (node == NULL)
  {
    cnt = 0;
  }
  else
  {
    do
    {
      cnt++;
      node = node->next;
    } while (node);
  }
  return cnt;
}


int cron_job_list_reset_id(){
  if (cron_job_node_count()==0) {
    linked_link_state.next_id=0;
    return 0;
    }
  return -1;
}