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

TEST_CASE("**CRON_JOB - INFO -- INIT TEST, THIS INITIALIZES THE TIME DATA MAY LEAK SOME MEMORY", "[cron_job]") {
  struct timeval tv;
  tv.tv_sec = 1530000000; // SOMEWHERE IN JUNE 2018
  settimeofday(&tv, NULL);

}



TEST_CASE("**CRON_JOB - cron_job_schedule and cron_job_remove IS IT WORKING? ", "[cron_job]")
{  

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
}

TEST_CASE("**CRON_JOB - cron_job_schedule CRON PARSER AND SCHEDULER IS AS EXPECTED ", "[cron_job]")
{
  time_t now;
  struct timeval tv;
  struct tm timeinfo;
  char buffer[256], buffer2[256];
  int buffer_len = 256;
  int ans = 0;
  tv.tv_sec = 1530000000; // SOMEWHERE IN JUNE 2018
  settimeofday(&tv, NULL);
  cron_job * job=cron_job_create("* * * * * *",NULL,NULL);
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
}

TEST_CASE("**CRON_JOB - cron_clear_all TEST CLEAR ALL JOBS ", "[cron_job]")
{
  int cnt_init=0,cnt=0,i=0;
  cron_job * jobs[10];
  cnt_init = cron_job_node_count();

  for (i =0;i<10;i++) {
    jobs[i]=cron_job_create("* * * * * *",NULL,NULL);
  }
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
  TEST_ASSERT_MESSAGE((int)jobs[0]->data > 5*8, "Unexpected delay");//IT DEPENDS ON TIME MOST OF THE TIME WILL DO 45, SOME 40 AND OTHERS 50
  TEST_ASSERT_MESSAGE((int)jobs[1]->data > 10005, "Unexpected delay"); // SOMETIMES IT RUNS TWICE BECAUSE OF BEGIN TIME 
  cron_job_destroy(jobs[0]);
  cron_job_destroy(jobs[1]);
  //cron_job_clear_all();
}





