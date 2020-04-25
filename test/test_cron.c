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
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "unity.h"
#include "cron.h"
#include "jobs.h"
void reset_cron(){
  cron_job_init();
  cron_stop();
}
TEST_CASE("**CRON_JOB - INFO -- INIT TEST, THIS INITIALIZES THE TIME DATA MAY LEAK SOME MEMORY", "[cron_job]") {
  struct timeval tv;
  tv.tv_sec = 1530000000; // SOMEWHERE IN JUNE 2018
  settimeofday(&tv, NULL);
  cron_job_init();
}



TEST_CASE("**CRON_JOB - cron_job_schedule and cron_job_remove IS IT WORKING? ", "[cron_job]")
{
  reset_cron();  

  int cnt = 0, cnt2 = 0, ans = 0;
  cnt = cron_job_node_count();
  cron_job * job=cron_job_create("* * * * * *",NULL,NULL);
  cnt2 = cron_job_node_count();
  TEST_ASSERT_MESSAGE(job != NULL, "INSERTION FAILED WITH ERRORS");
  TEST_ASSERT_EQUAL_INT_MESSAGE(cnt + 1, cnt2, "LIST DIDNT GROW");
  ans = cron_job_destroy(job);
  cnt2 = cron_job_node_count();  
  TEST_ASSERT_EQUAL_INT_MESSAGE( 0, ans, "DESTROY FAILED WITH ERRORS");
  TEST_ASSERT_EQUAL_INT_MESSAGE(cnt, cnt2, "LIST DIDNT REDUCE");
  // cron_stop();
  vTaskDelay((1 * 1000) / portTICK_PERIOD_MS); 
}

TEST_CASE("**CRON_JOB - cron_job_schedule CRON PARSER AND SCHEDULER IS AS EXPECTED ", "[cron_job]")
{
  reset_cron();
  time_t now;
  struct timeval tv;
  struct tm timeinfo;
  char buffer[256], buffer2[256];
  int buffer_len = 256;
  int ans = 0;
  tv.tv_sec = 1530000000; // SOMEWHERE IN JUNE 2018
  settimeofday(&tv, NULL);
  cron_job * job=cron_job_create("* * * * * *",NULL,NULL);
  cron_stop();
  time(&now);
  localtime_r(&(job->next_execution), &timeinfo);
  strftime(buffer, buffer_len, "%c", &timeinfo);
  localtime_r(&now, &timeinfo);
  strftime(buffer2, buffer_len, "%c", &timeinfo);
  printf("now: %s next execution: %s\n", buffer2, buffer);
  printf("now: %d next_execution: %d\n", (int)now, (int)job->next_execution);
  TEST_ASSERT_EQUAL_INT_MESSAGE(now + 1, job->next_execution, "SCHEDULE IS NOT FOR THE NEXT SECOND AS IT SHOULD BE");
  ans = cron_job_destroy(job);
  TEST_ASSERT_EQUAL_INT_MESSAGE(0, ans, "UNABLE TO DESTROY A JOB");
  cron_stop();
  // vTaskDelay((1 * 1000) / portTICK_PERIOD_MS);

}

TEST_CASE("**CRON_JOB - cron_clear_all TEST CLEAR ALL JOBS ", "[cron_job]")
{
  reset_cron();
  int cnt_init=0,cnt=0,i=0;
  cron_job * jobs[10];
  cnt_init = cron_job_node_count();
  for (i =0;i<10;i++) {
    jobs[i]=cron_job_create("* * * * * *",NULL,NULL);
  }
  cron_stop();
  cnt = cron_job_node_count();
  TEST_ASSERT_EQUAL_INT_MESSAGE(i, cnt-cnt_init,"Creation count");
  cron_job_destroy(jobs[0]);
  cnt = cron_job_node_count();
  TEST_ASSERT_EQUAL_INT_MESSAGE(i-1, cnt-cnt_init,"Single destroy call");
  cron_job_clear_all();
  cnt = cron_job_node_count();
  TEST_ASSERT_EQUAL_INT_MESSAGE(0, cnt-cnt_init,"Destroy all call");
}

void test_cron_job_sample_callback(cron_job *job)
{
  time_t now;
  struct tm timeinfo;
  char buffer[256],buffer2[256];
  int buffer_len = 256;
  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(buffer, buffer_len, "%c", &timeinfo);
  localtime_r(&(job->next_execution), &timeinfo);
  strftime(buffer2, buffer_len, "%c", &timeinfo);
  job->data = job->data+5;
  printf("CALLBACK RUNNED AT TIME: %s SHOULD RUN AT: %s WITH DATA: %d \n", buffer,buffer2,(unsigned int)job->data);
  return;
}

TEST_CASE("**CRON_JOB - cron_schedule_task TEST IF THE SCHEDULER RUNS ONCE AND WITH EXPECTED RESULTS", "[cron_job]")
{
  reset_cron();
  time_t begin=1530000000;
  const int seconds_to_run=10*2; //whatever you want to run must be twice because one for the delay and one for the callback :P 
  cron_job * jobs[2];
  struct timeval tv;
  tv.tv_sec = begin; // SOMEWHERE IN JUNE 2018
  settimeofday(&tv, NULL);
  jobs[0]=cron_job_create("* * * * * *",test_cron_job_sample_callback,(void *)0);
  jobs[1]=cron_job_create("*/5 * * * * *",test_cron_job_sample_callback,(void *)10000);
  for (int i =0;i<seconds_to_run;i++) {
   cron_schedule_task((void *)"R1"); // R1 = RUN ONCE SYNCRHONOUS MODE*/
  }
  vTaskDelay((2 * 1000) / portTICK_PERIOD_MS); 
  TEST_ASSERT_MESSAGE((int)jobs[0]->data >= 5*8, "Unexpected delay");//IT DEPENDS ON TIME MOST OF THE TIME WILL DO 45, SOME 40 AND OTHERS 50
  TEST_ASSERT_MESSAGE((int)jobs[1]->data >= 10005, "Unexpected delay"); // SOMETIMES IT RUNS TWICE BECAUSE OF BEGIN TIME 
  cron_job_destroy(jobs[0]);
  cron_job_destroy(jobs[1]);
  //cron_job_clear_all();
}

TEST_CASE("**CRON_JOB - cron_schedule_task TEST IF THE SCHEDULER RUNS WHILE STARTED", "[cron_job]")
{
  reset_cron();
  time_t begin=1530000000;
  cron_job * job;
  struct timeval tv;
  tv.tv_sec = begin; // SOMEWHERE IN JUNE 2018
  settimeofday(&tv, NULL);
  job=cron_job_create("* * * * * *",test_cron_job_sample_callback,(void *)0);
  vTaskDelay((1 * 1000) / portTICK_PERIOD_MS); // WAIT FOR THE TIME TO SETTLE
  int res = cron_start();
  TEST_ASSERT_EQUAL_INT_MESSAGE(Cron_ok,res, "Cron did not start");//IT DEPENDS ON TIME MOST OF THE TIME WILL DO 45, SOME 40 AND OTHERS 50
  vTaskDelay((6 * 1000) / portTICK_PERIOD_MS); // WAIT FOR THE TASK TO RUN - WE DO 6 SECONDS BUT CHECK AGAINST 5 ASSUMING SOME ERRORS ON THE DELAY BETWEEN A TASK CREATION AND STUFF
  cron_stop();
  TEST_ASSERT_MESSAGE((int)job->data > 5*5, "Unexpected delay");//IT DEPENDS ON TIME 
  cron_job_destroy(job);
  //cron_job_clear_all();
}

TEST_CASE("**CRON_JOB - cron_job_create() should return a job with the data 1 2 3 * * *", "[cron_job]")
{
  reset_cron();
  cron_job * job = cron_job_create("1 2 3 * * *",NULL,(char *)"the data");
  TEST_ASSERT_MESSAGE(job!=NULL, "JOB IS NULL");
  TEST_ASSERT_EQUAL_INT_MESSAGE(strlen(job->data), 8, "the data len is not 11");
  TEST_ASSERT_EQUAL_INT_MESSAGE(strncmp(job->data,"the data",8), 0, "the job data is not 1 2 3 * * *");
  vTaskDelay((1 * 1000) / portTICK_PERIOD_MS); 
  cron_job_clear_all();
}

TEST_CASE("**CRON_JOB - cron_job_create() NULL if the schedule is wrong", "[cron_job]")
{
  reset_cron();
  cron_job * job = cron_job_create("1 2 3",NULL,(char *)"the data");
  TEST_ASSERT_MESSAGE(job == NULL, "the job is not null");
}

TEST_CASE("**CRON_JOB - cron_job_destroy() return 0 if job is a cron_job", "[cron_job]")
{
  reset_cron();
  cron_job * job = cron_job_create("* * * * * *",NULL,(char *)"the data");
  vTaskDelay((1 * 1000) / portTICK_PERIOD_MS); 
  int res = cron_job_destroy(job);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 0, "the return isnt 0");
  cron_job_clear_all();
}

TEST_CASE("**CRON_JOB - cron_job_destroy() return 0 if after 2 jobs creation but just one is destroyed", "[cron_job]")
{
  reset_cron();
  (void) cron_job_create("1 2 3 * * *",NULL,(char *)"the data");
  cron_job * job2 = cron_job_create("1 2 5 * * *",NULL,(char *)"the data");
  cron_stop();
  int res = cron_job_destroy(job2);
  TEST_ASSERT_EQUAL_INT_MESSAGE(0, res, "the return isnt 0");
  vTaskDelay((1 * 1000) / portTICK_PERIOD_MS); 
  cron_job_clear_all();
}

TEST_CASE("**CRON_JOB - cron_job_destroy() return 0 if after 4 jobs creation but just one is destroyed", "[cron_job]")
{
  reset_cron();
  (void) cron_job_create("1 2 3 * * *",NULL,(char *)"the data");
  (void) cron_job_create("1 2 5 * * *",NULL,(char *)"the data");
  cron_job * job3 = cron_job_create("3 2 5 * * *",NULL,(char *)"the data");
  (void) cron_job_create("4 2 5 * * *",NULL,(char *)"the data");
  cron_stop();
  int res = cron_job_destroy(job3);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 0, "the return isnt 0");
  cron_job_clear_all();
}

TEST_CASE("**CRON_JOB - cron_job_destroy() return -1 if job is null", "[cron_job]")
{
  reset_cron();
  int res = cron_job_destroy(NULL);
  TEST_ASSERT_EQUAL_INT_MESSAGE(Cron_bad_job, res, "the return isnt -1");
}


TEST_CASE("**CRON_JOB - cron_job_clear_all() anfter execute cron_job_clear_all the job is null", "[cron_job]")
{
  reset_cron();
  (void) cron_job_create("1 2 3 * * *",NULL,(char *)"the data");
  int res = cron_job_clear_all();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 0, "return of clear is not zero");
}


TEST_CASE("**CRON_JOB - cron_job_clear_all() called with an empty list", "[cron_job]")
{
  reset_cron();
  (void) cron_job_clear_all();
  int res = cron_job_clear_all();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 0, "return of clear is not zero");
}

TEST_CASE("**CRON_JOB - cron_stop() return 0 if cron is started", "[cron_job]")
{
  reset_cron();
  cron_start();
  // (void) cron_job_create("1 2 3 * * *",NULL,"the data");
  int res = cron_stop();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 0, "return of cron_stop is not zero");
}


TEST_CASE("**CRON_JOB - cron_stop() return -1 if cron is not running and method is called again", "[cron_job]")
{
  reset_cron();
  (void) cron_stop();
  int res = cron_stop();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, Cron_is_stopped, "return of cron_stop is not -1");
}

TEST_CASE("**CRON_JOB - cron_start() return -1 if cron is already started", "[cron_job]")
{
  reset_cron();
  cron_start();
  (void) cron_job_create("1 2 3 * * *",NULL,(char *)"the data");
  int res = cron_start();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, Cron_not_stopped, "return of cron_start is not -1");
  vTaskDelay((1 * 1000) / portTICK_PERIOD_MS); 
  (void) cron_job_clear_all();
}
TEST_CASE("**CRON_JOB - cron_job_schedule() return -1 if job is null", "[cron_job]")
{
  reset_cron();  
  int res = cron_job_schedule(NULL);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, Cron_bad_job, "res is not -1");
}

TEST_CASE("**CRON_JOB - cron_job_unschedule() return -1 if job is null", "[cron_job]")
{
  reset_cron();  
  int res = cron_job_unschedule(NULL);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, Cron_bad_job, "res is not -1");
}

TEST_CASE("**CRON_JOB - cron_job_unschedule() return 0 if job is ok", "[cron_job]")
{
  reset_cron();  
  cron_job * job = cron_job_create("3 2 5 * * *",NULL,(char *)"the data");
  cron_stop();
  int res = cron_job_unschedule(job);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 0, "res is not 0");
}

TEST_CASE("**CRON_JOB - cron_job_load_expression() return 0 if job and schedule are correct", "[cron_job]")
{
  reset_cron();  
  cron_job_list_init(); // CALL THIS ON ANY CREATE
  cron_job *job = calloc(sizeof(cron_job), 1);
  job->callback = NULL;
  job->data = (char *)"3 2 5 * * *";
  job->id = -1;
  job->load = NULL;
  int res = cron_job_load_expression(job, "3 2 5 * * *");
  cron_stop();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 0, "res is not 0");
}

TEST_CASE("**CRON_JOB - cron_job_load_expression() return -1 if schedule is null", "[cron_job]")
{
  reset_cron();  
  cron_job_list_init(); // CALL THIS ON ANY CREATE
  cron_job *job = calloc(sizeof(cron_job), 1);
  job->callback = NULL;
  job->data = (char *)"3 2 5 * * *";
  job->id = -1;
  job->load = NULL;
  int res = cron_job_load_expression(job, NULL);
  cron_stop();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, Cron_bad_schedule, "res is not -1");
}

TEST_CASE("**CRON_JOB - cron_job_load_expression() return -2 if schedule is wrong 3 2 5", "[cron_job]")
{
  reset_cron();  
  cron_job_list_init(); // CALL THIS ON ANY CREATE
  cron_job *job = calloc(sizeof(cron_job), 1);
  job->callback = NULL;
  job->data = (char *)"3 2 5";
  job->id = -1;
  job->load = NULL;
  int res = cron_job_load_expression(job, "3 2 5");
  cron_stop();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, Cron_parse_expresion_error, "res is not -1");
}

TEST_CASE("**CRON_JOB - cron_job_load_expression() return -2 if schedule is wrong *****", "[cron_job]")
{
  reset_cron();  
  cron_job_list_init(); // CALL THIS ON ANY CREATE
  cron_job *job = calloc(sizeof(cron_job), 1);
  job->callback = NULL;
  job->data = (char *)"3 2 5";
  job->id = -1;
  job->load = NULL;
  int res = cron_job_load_expression(job, "*****");
  cron_stop();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, Cron_parse_expresion_error, "res is not -1");
}

TEST_CASE("**CRON_JOB - cron_job_load_expression() return -2 if schedule is wrong 123456789", "[cron_job]")
{
  reset_cron();  
  cron_job_list_init(); // CALL THIS ON ANY CREATE
  cron_job *job = calloc(sizeof(cron_job), 1);
  job->callback = NULL;
  job->data = (char *)"3 2 5";
  job->id = -1;
  job->load = NULL;
  int res = cron_job_load_expression(job, "123456789");
  cron_stop();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, Cron_parse_expresion_error, "res is not -1");
}

TEST_CASE("**CRON_JOB - cron_job_load_expression() return -2 if schedule is wrong asdfghjkl", "[cron_job]")
{
  reset_cron();  
  cron_job_list_init(); // CALL THIS ON ANY CREATE
  cron_job *job = calloc(sizeof(cron_job), 1);
  job->callback = NULL;
  job->data = (char *)"3 2 5";
  job->id = -1;
  job->load = NULL;
  int res = cron_job_load_expression(job, "asdfghjkl");
  cron_stop();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, Cron_parse_expresion_error, "res is not -1");
  free(job);
}


TEST_CASE("**CRON_JOB - cron_job_has_loaded() return 0 if job is scheduled", "[cron_job]")
{
  reset_cron();  
  cron_job * job = cron_job_create("3 2 5 * * *",NULL,(char *)"the data");
  int res = cron_job_has_loaded(job);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 0, "res is not 0");
  cron_stop();
}

TEST_CASE("**CRON_JOB - cron_job_has_loaded() return -2 if job is scheduled", "[cron_job]")
{
  reset_cron();  
  int res = cron_job_has_loaded(NULL);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, Cron_bad_job, "res is not -2");
  cron_stop();
}

TEST_CASE("**CRON_JOB - cron_job_has_loaded() return Cron_expresion_not_loaded if job is not scheduled", "[cron_job]")
{
  reset_cron();  
  cron_job_list_init(); // CALL THIS ON ANY CREATE
  cron_job *job = calloc(sizeof(cron_job), 1);
  job->callback = NULL;
  job->data = (char *)"3 2 5";
  job->id = -1;
  job->load = NULL;
  int res = cron_job_has_loaded(job);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, Cron_expresion_not_loaded, "res is not -2");
  cron_stop();
  free(job);
}