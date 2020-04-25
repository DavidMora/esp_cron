#include <string.h>
#include "unity.h"
#include "freertos/FreeRTOS.h"

#include "jobs.h"
#include "cron.h"

cron_job * test_create_job(char * data, int id){
  cron_job *job = calloc(sizeof(cron_job), 1);
  job->callback = NULL;
  job->data = data;
  job->id = id;
  job->load = NULL;
  return job;
}

TEST_CASE("**CRON_JOB - cron_job_list_init() return 0 if module is not initialized", "[cron_job_list]")
{
  cron_job_list_dinit();
  int res = cron_job_list_init();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 0, "res is not 0");
  cron_job_list_dinit();
}

TEST_CASE("**CRON_JOB - cron_job_has_loaded() return -1 if module is already initialized", "[cron_job_list]")
{
  cron_job_list_dinit();
  (void) cron_job_list_init();
  int res = cron_job_list_init();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, -1, "res is not -1");
  cron_job_list_dinit();
}
TEST_CASE("**CRON_JOB - cron_job_list_insert() return -1 if job is NULL", "[cron_job_list]")
{
  cron_job_list_dinit();
  (void) cron_job_list_init();
  int res = cron_job_list_insert(NULL);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, -1, "response is not -1");
  cron_job_list_dinit();
}
TEST_CASE("**CRON_JOB - cron_job_list_insert() return the new node id if the node is added to list", "[cron_job_list]")
{
  cron_job_list_dinit();
  (void) cron_job_list_init();
  cron_job * job = test_create_job("test1", 8);
  int res = cron_job_list_insert(job);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 8, "response is not 8");
  free(job);
  cron_job_list_dinit();
}
TEST_CASE("**CRON_JOB - cron_job_list_insert() return the new node id if the node is added to list but id is -1", "[cron_job_list]")
{
  cron_job_list_dinit();
  (void) cron_job_list_init();
  cron_job * job = test_create_job("test1", -1);
  int res = cron_job_list_insert(job);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 0, "response is not 0");
  free(job);
  cron_job_list_dinit();
}
TEST_CASE("**CRON_JOB - cron_job_list_insert() return the new node id if the node is added to list while inserting 2 nodes", "[cron_job_list]")
{
  cron_job_list_dinit();
  (void) cron_job_list_init();
  cron_job * job = test_create_job("test1", 8);
  int res = cron_job_list_insert(job);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 8, "response is not 8");
  cron_job * job2 = test_create_job("test1", 1);
  res = cron_job_list_insert(job2);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 1, "response is not 1");
  free(job);
  free(job2);
  cron_job_list_dinit();
}
TEST_CASE("**CRON_JOB - cron_job_list_insert() add two nodes width the same id", "[cron_job_list]")
{
  cron_job_list_dinit();
  (void) cron_job_list_init();
  cron_job * job = test_create_job("test1", 1);
  int res = cron_job_list_insert(job);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 1, "response is not 1");
  cron_job * job2 = test_create_job("test1", 1);
  res = cron_job_list_insert(job2);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 1, "response is not 1");
  free(job);
  free(job2);
  cron_job_list_dinit();
}

TEST_CASE("**CRON_JOB - cron_job_list_remove() return 0 if the node exist", "[cron_job_list]")
{
  cron_job_list_dinit();
  (void) cron_job_list_init();
  cron_job * job = test_create_job("test", -1);
  (void) cron_job_list_insert(job);
  int res = cron_job_list_remove(0);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 0, "response is not 0");
  free(job);
  cron_job_list_dinit();
}

TEST_CASE("**CRON_JOB - cron_job_list_remove() return -5 if the node exist", "[cron_job_list]")
{
  cron_job_list_dinit();
  (void) cron_job_list_init();
  cron_job * job = test_create_job("test", -1);
  (void) cron_job_list_insert(job);
  cron_job * job2 = test_create_job("test", -1);
  (void) cron_job_list_insert(job2);
  int res = cron_job_list_remove(10);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, -5, "response is not -5");
  free(job);
  free(job2);
  cron_job_list_dinit();
}

TEST_CASE("**CRON_JOB - cron_job_list_remove() return 0 when remove the last element", "[cron_job_list]")
{
  cron_job_list_dinit();
  (void) cron_job_list_init();
  cron_job * job = test_create_job("test", 1);
  (void) cron_job_list_insert(job);
  cron_job * job2 = test_create_job("test", 2);
  (void) cron_job_list_insert(job2);
  int res = cron_job_list_remove(2);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 0, "response is not 0");
  free(job);
  free(job2);
  cron_job_list_dinit();
}

TEST_CASE("**CRON_JOB - cron_job_list_remove() return 0 when remove the first element", "[cron_job_list]")
{
  cron_job_list_dinit();
  (void) cron_job_list_init();
  cron_job * job = test_create_job("test", 1);
  (void) cron_job_list_insert(job);
  cron_job * job2 = test_create_job("test", 2);
  (void) cron_job_list_insert(job2);
  int res = cron_job_list_remove(1);
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 0, "response is not 0");
  free(job);
  free(job2);
  cron_job_list_dinit();
}

TEST_CASE("**CRON_JOB - cron_job_node_count() return 0 if there is no nodes", "[cron_job_list]")
{
  cron_job_list_dinit();
  int res = cron_job_node_count();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 0, "res is not 0");
  cron_job_list_dinit();
}

TEST_CASE("**CRON_JOB - cron_job_node_count() return 2 if there is two nodes", "[cron_job_list]")
{
  cron_job_list_dinit();
  (void) cron_job_list_init();
  cron_job * job = test_create_job("test", 1);
  (void) cron_job_list_insert(job);
  cron_job * job2 = test_create_job("test", 2);
  (void) cron_job_list_insert(job2);
  int res = cron_job_node_count();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 2, "res is not 2");
  free(job);
  free(job2);
  cron_job_list_dinit();
}

TEST_CASE("**CRON_JOB - cron_job_node_count() return 5 if there is five nodes", "[cron_job_list]")
{
  cron_job_list_dinit();
  (void) cron_job_list_init();
  cron_job * job = test_create_job("test", 1);
  (void) cron_job_list_insert(job);
  cron_job * job2 = test_create_job("test", 2);
  (void) cron_job_list_insert(job2);
  cron_job * job3 = test_create_job("test", 3);
  (void) cron_job_list_insert(job3);
  cron_job * job4 = test_create_job("test", 4);
  (void) cron_job_list_insert(job4);
  cron_job * job5 = test_create_job("test", 5);
  (void) cron_job_list_insert(job5);
  int res = cron_job_node_count();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 5, "res is not 5");
  free(job);
  free(job2);
  free(job3);
  free(job4);
  free(job5);
  cron_job_list_dinit();
}

TEST_CASE("**CRON_JOB - cron_job_node_count() return 3 if there is five nodes but removes 2", "[cron_job_list]")
{
  cron_job_list_dinit();
  (void) cron_job_list_init();
  cron_job * job = test_create_job("test", 1);
  (void) cron_job_list_insert(job);
  cron_job * job2 = test_create_job("test", 2);
  (void) cron_job_list_insert(job2);
  cron_job * job3 = test_create_job("test", 3);
  (void) cron_job_list_insert(job3);
  cron_job * job4 = test_create_job("test", 4);
  (void) cron_job_list_insert(job4);
  cron_job * job5 = test_create_job("test", 5);
  (void) cron_job_list_insert(job5);
  (void) cron_job_list_remove(2);
  (void) cron_job_list_remove(4);
  int res = cron_job_node_count();
  TEST_ASSERT_EQUAL_INT_MESSAGE(res, 3, "res is not 3");
  free(job);
  free(job2);
  free(job3);
  free(job4);
  free(job5);
  cron_job_list_dinit();
}