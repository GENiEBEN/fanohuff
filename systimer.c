
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)

 /* Windows */
 #include <windows.h>
 #define WINDOWS_TIMER
 typedef struct {
   double  period;
   __int64 t0;
 } systimer_t;

#elif defined(__DJGPP__)

 /* DOS/DJGPP */
 #include <time.h>
 #define DOS_TIMER
 typedef struct {
   uclock_t t0;
 } systimer_t;

#else

 /* Unix (Linux etc) */
 #include <stdlib.h>
 #include <sys/time.h>
 #define GTOD_TIMER
 typedef struct {
   struct timeval t0;
 } systimer_t;

#endif

/* Global timer resource */
static systimer_t global_timer;


void InitTimer( void )
{
#if defined(WINDOWS_TIMER)

  __int64 freq;

  global_timer.period = 0.0;
  if( QueryPerformanceFrequency( (LARGE_INTEGER *)&freq ) )
  {
    global_timer.period = 1.0 / (double) freq;
    QueryPerformanceCounter( (LARGE_INTEGER *)&global_timer.t0 );
  }
  else
    global_timer.t0 = (__int64) GetTickCount();

#elif defined(DOS_TIMER)

  global_timer.t0 = uclock();

#elif defined(GTOD_TIMER)

  gettimeofday( &global_timer.t0, NULL );

#endif
}


double GetTime( void )
{
#if defined(WINDOWS_TIMER)

  __int64 t;

  if( global_timer.period > 0.0 )
  {
    QueryPerformanceCounter( (LARGE_INTEGER *)&t );
    return global_timer.period * (double) (t - global_timer.t0);
  }
  else
    return 0.001 * (double) (GetTickCount() - (int) global_timer.t0);

#elif defined(DOS_TIMER)

  return (1.0 / UCLOCKS_PER_SEC) * (double) (uclock() - global_timer.t0);

#elif defined(GTOD_TIMER)

  struct timeval tv;

  gettimeofday( &tv, NULL );

  tv.tv_sec -= global_timer.t0.tv_sec;
  tv.tv_usec -= global_timer.t0.tv_usec;
  if( tv.tv_usec < 0 )
  {
    --tv.tv_sec;
    tv.tv_usec += 1000000;
  }

  return (double) tv.tv_sec + 0.000001 * (double) tv.tv_usec;

#else

  return 0.0;

#endif
}
