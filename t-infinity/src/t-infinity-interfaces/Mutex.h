#ifdef MULTI_THREAD
# ifdef WINNT
#  include <windows.h>
# else
#  include <pthread.h>
# endif
#endif

#include <exception>
#ifdef HAVE_STDEXCEPT
# include <stdexcept>
#endif
#include <string>
#include <sstream>

class MutexBusy : public std::exception {
public:
  const char* what() const throw () {
    return "Mutex Busy";
  }
};

class Mutex {
public:

  typedef void (*LockFuncPtr)(void);
  typedef void (*UnlockFuncPtr)(void);

#ifdef MULTI_THREAD
#ifdef WINNT
  typedef HANDLE mutex_t;
#else
  typedef pthread_mutex_t mutex_t;
#endif
#else
  typedef char mutex_t;
#endif

  /**
   * Construct a Mutual Exclusion (Mutex) lock.
   */
  Mutex(void) : lockFunc(NULL), unlockFunc(NULL) {
//    std::cout << "Mutex constructor" << std::endl;
#ifdef MULTI_THREAD
#ifdef WINNT
    mutex = CreateMutex (NULL,      /* default security attributes */
                         0,         /* not initially locked */
                         NULL);     /* no name */
    if( !mutex )
      throw std::runtime_error("Failed to create mutex");
#else
    if( pthread_mutex_init(&mutex,NULL) != 0 )
      throw std::runtime_error("Failed to create mutex");
#endif
#endif
  }

  /**
   * Destroy a Mutual Exclusion (Mutex) lock.
   */
  virtual ~Mutex() {
#ifdef MULTI_THREAD
#ifdef WINNT
    if( mutex ) {
      if ( !CloseHandle(mutex) )
        throw std::runtime_error("Failed to close mutex");
    }
#else
    int ret;
    if( (ret=pthread_mutex_destroy(&mutex)) != 0 ) {
      std::stringstream ss;
      ss << ret;
      std::string message = std::string("Failed to destroy mutex with: ") +
                            ss.str();
      throw std::runtime_error(message.c_str());
    }
#endif
#endif
  }

  /**
   * Obtain the Mutual Exclusion (Mutex) lock.
   *
   * @throws MutexBusy if busy or std::runtime_error on failure
   **/
  void tryLock() {
#ifdef MULTI_THREAD
# ifdef WINNT
    DWORD rc = WaitForSingleObject(mutex,INFINITE);

    if ( rc == WAIT_ABANDONED )
    {
      // the previous caller died without releasing the mutex, but now we can
      // really lock it
      //
      // use 0 timeout, normally we should always get it
      rc = WaitForSingleObject(mutex, 0);
    }

    switch ( rc )
    {
      case WAIT_OBJECT_0:
        // ok
        break;

      case WAIT_TIMEOUT:
        throw MutexBusy();

      case WAIT_ABANDONED:        // checked for above

      default:
        // impossible return value, fall through

      case WAIT_FAILED:
        throw std::runtime_error("Mutex::lock, WaitForSingleObject() failed");
    }
# else
    int ret;
    if( (ret=pthread_mutex_trylock(&mutex)) != 0 ) {
      if( EBUSY == ret ) {
        throw MutexBusy();
      } else {
        std::stringstream ss;
        ss << ret;
        std::string message =
                   std::string("Mutex::lock, pthread_mutex_lock() returned: ") +
                   ss.str();
        throw std::runtime_error(message.c_str());
      }
    }
# endif
#else
    if( lockFunc )
      lockFunc();
#endif
  }


  /**
   * Release the Mutual Exclusion (Mutex) lock.
   *
   * @throws std::runtime_error on failure
   **/
  void unlock() {
#ifdef MULTI_THREAD
# ifdef WINNT
    if ( !ReleaseMutex(mutex) )
      throw std::runtime_error("Mutex::lock, ReleaseMutex() failed");
# else
    int ret;
    if( (ret=pthread_mutex_unlock(&mutex)) != 0 ) {
      std::stringstream ss;
      ss << ret;
      std::string message =
                  std::string("Mutex::lock, pthread_mutex_unlock() returned: ")+
                  ss.str();
      throw std::runtime_error(message.c_str());
    }
# endif
#else
    if( unlockFunc )
      unlockFunc();
#endif
  }

  void setCustomLockFunctions(LockFuncPtr lock, UnlockFuncPtr unlock) {
    lockFunc = lock;
    unlockFunc = unlock;
  }

private:
  mutex_t       mutex;
  LockFuncPtr   lockFunc;
  UnlockFuncPtr unlockFunc;
};

