# CRON like component for the ESP-IDF framework

This is a cron-like clone for the esp-idf framework. It uses cron-like sytanx and time libraries included in newlib (esp-idf framework) for task scheduling.

## How to use

We tried to keep module functions interface at minimum there is a creator, a destroyer a cron module starter and a cron module stopper. The workflow would be to define at least one job and then start the module. Then create and destroy jobs as desired. Keep in mind that if there are no jobs to be scheduled the cron module will stop itself, this is by design as we don't want to waste cpu time.

Please remember that this module relies heavilly on the time.h library. **Time has to be initialized before any job creation.** The library time.h can be set manually or with another component like sntp, but it must have started before to this module is in use. This component will not perform any checks to idetify if time has been set.

### Create

Usage is pretty simple, we provided a component factory for cron-job creation. 


```C
cron_job *cron_job_create(const char *schedule, cron_job_callback callback, void *data)
```

* Where schedule is a cron-like string with seconds resolution. 

            ┌────────────── second (0 - 59)  
            | ┌───────────── minute (0 - 59)
            | │ ┌───────────── hour (0 - 23)
            | │ │ ┌───────────── day of month (1 - 31)
            | │ │ │ ┌───────────── month (1 - 12)
            | │ │ │ │ ┌───────────── day of week (0 - 6) (Sunday to Saturday;
            | │ │ │ │ │                                       7 is also Sunday on some systems)
            | │ │ │ │ │
            | │ │ │ │ │
            * * * * * *  
            
Thank you alex at staticlibs.net for the good work on the parser!!. 

* The callback is just a function pointer for the job that will be scheduled with the running cron_job as an argument, defined as:

```C
typedef void (*cron_job_callback)(cron_job *);
```

Please note that the callback is a simple function, no need for infinite loops or vTask calls, the cron module will handle this for you

* And data is a non managed, non typed  pointer that will be stored in the cron_job structure that can be used as the user needs.

### Destroy



If you want to stop a previously created cron job simply call the destroy method with the returned cron_job from the creator. 



```C
int cron_job_destroy(cron_job * job);
```


### Starting the module

You can start the module with at least one defined job by calling 

```C
int cron_start();
```

### Stopping the module

You can stop the module by calling 

```C
int cron_stop();
```

### Clearing all jobs a.k.a destroying all jobs

We defined a helper to stop all cron jobs, we think it might be useful in some situations

```C
int cron_job_clear_all();
```


## Example

The first thing you need to pay attention to is to initialize the time module of newlib, you can do this in several ways, one good example is to use sntp for this. But if you want to do it manually the following code will work. 

```C
  /* YOU MUST SET THE TIME FIRST BE CAREFUL ABOUT THIS - NOT PART OF THE MODULE*/
  struct timeval tv;
  time_t begin=1530000000;
  tv.tv_sec = begin; // SOMEWHERE IN JUNE 2018
  settimeofday(&tv, NULL);
  /* END TIME SET */
```

The header for this is just `cron.h`.

The code below is all you need to run the code notice that we are running a sample callback function which you can find after this code. 

```C
  cron_job * jobs[2];
  jobs[0]=cron_job_create("* * * * * *",test_cron_job_sample_callback,(void *)0);
  jobs[1]=cron_job_create("*/5 * * * * *",test_cron_job_sample_callback,(void *)10000);
  cron_start();
  vTaskDelay((running_seconds * 1000) / portTICK_PERIOD_MS); // This is just to emulate a delay between the calls
  cron_stop();
  cron_job_clear_all();
```

Sample callback:

```C
void test_cron_job_sample_callback(cron_job *job)
{
  /* DO YOUR WORK IN HERE */
return;
}
```
