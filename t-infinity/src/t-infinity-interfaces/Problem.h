#ifndef _PROBLEM_H_
#define _PROBLEM_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <complex.h>

#include "tinf_problem.h"

#include <stdio.h>
class Problem
{
  public:
    Problem(void* prob) : problem(prob) {}

    virtual ~Problem() {}

    inline virtual bool defined(const char* key, enum TINF_DATA_TYPE* datatype,
                                int32_t* rank,
                                size_t dims[TINF_PROBLEM_MAX_RANK])
    {
      int32_t err;
      bool ret = tinf_problem_defined(problem, key, strlen(key), datatype,
                                      rank, dims, &err);
      if( TINF_SUCCESS != err ) {
        std::stringstream ss;
        ss << err;
        std::string message = std::string("tinf_problem_defined returned ") +
                              ss.str();
        throw std::runtime_error(message.c_str());
      }
      return ret;
    }

    inline virtual void value(const char* key, const void* data, int32_t rank,
                              const size_t dims[])
    {
      int32_t err = tinf_problem_value(problem, key, strlen(key), data, rank, dims);

      if( TINF_SUCCESS != err ) {
        std::stringstream ss;
        ss << err;
        std::string message = std::string("tinf_problem_value returned ") +
                              ss.str();
        throw std::runtime_error(message.c_str());
      }
    }

    // Overloads

    inline virtual bool value(const char* key, int32_t* val)
    {
      enum TINF_DATA_TYPE type;
      int32_t rank;
      size_t dims[TINF_PROBLEM_MAX_RANK];
      try {
        if( defined(key, &type, &rank, dims) ) {
          if( TINF_INT32 == type && 0 == rank ) {
            value(key,(const void*)val,rank,dims);
            return true;
          }
        }
      } catch (std::runtime_error e) {
        throw;
      }
      return false;
    }

    inline virtual bool value(const char* key, int64_t* val)
    {
      enum TINF_DATA_TYPE type;
      int32_t rank;
      size_t dims[TINF_PROBLEM_MAX_RANK];
      try {
        if( defined(key, &type, &rank, dims) ) {
          if( TINF_INT64 == type && 0 == rank ) {
            value(key,(const void*)val,rank,dims);
            return true;
          }
        }
      } catch (std::runtime_error e) {
        throw;
      }
      return false;
    }

    inline virtual bool value(const char* key, float* val)
    {
      enum TINF_DATA_TYPE type;
      int32_t rank;
      size_t dims[TINF_PROBLEM_MAX_RANK];
      try {
        if( defined(key, &type, &rank, dims) ) {
          if( TINF_FLOAT == type && 0 == rank ) {
            value(key,(const void*)val,rank,dims);
            return true;
          }
        }
      } catch (std::runtime_error e) {
        throw;
      }
      return false;
    }

    inline virtual bool value(const char* key, double* val)
    {
      enum TINF_DATA_TYPE type;
      int32_t rank;
      size_t dims[TINF_PROBLEM_MAX_RANK];
      try {
        if( defined(key, &type, &rank, dims) ) {
          if( TINF_DOUBLE == type && 0 == rank ) {
            value(key,(const void*)val,rank,dims);
            return true;
          }
        }
      } catch (std::runtime_error e) {
        throw;
      }
      return false;
    }

    inline virtual bool value(const char* key, std::complex<double>* val)
    {
      enum TINF_DATA_TYPE type;
      int32_t rank;
      size_t dims[TINF_PROBLEM_MAX_RANK];
      try {
        if( defined(key, &type, &rank, dims) ) {
          if( TINF_CMPLX_DOUBLE == type && 0 == rank ) {
            value(key,(const void*)val,rank,dims);
            return true;
          }
        }
      } catch (std::runtime_error e) {
        throw;
      }
      return false;
    }

    inline virtual bool value(const char* key, bool* val)
    {
      enum TINF_DATA_TYPE type;
      int32_t rank;
      size_t dims[TINF_PROBLEM_MAX_RANK];
      try {
        if( defined(key, &type, &rank, dims) ) {
          if( TINF_BOOL == type && 0 == rank ) {
            value(key,(const void*)val,rank,dims);
            return true;
          }
        }
      } catch (std::runtime_error e) {
        throw;
      }
      return false;
    }

    inline virtual bool value(const char* key, int32_t** val, size_t* len)
    {
      enum TINF_DATA_TYPE type;
      int32_t rank;
      size_t dims[TINF_PROBLEM_MAX_RANK];
      int32_t* ptr;
      try {
        if( defined(key, &type, &rank, dims) ) {
          if( TINF_INT32 == type && 1 == rank ) {
            if( (ptr=(int32_t*)malloc(dims[0]*sizeof(int32_t))) == NULL)
              throw std::runtime_error("tinf_problem_value: allocation error");
            *len = dims[0];
            value(key,(const void*)ptr,rank,dims);
            *val = ptr;
            return true;
          }
        }
      } catch (std::runtime_error e) {
        throw;
      }
      return false;
    }

    inline virtual bool value(const char* key, int64_t** val, size_t* len)
    {
      enum TINF_DATA_TYPE type;
      int32_t rank;
      size_t dims[TINF_PROBLEM_MAX_RANK];
      int64_t* ptr;
      try {
        if( defined(key, &type, &rank, dims) ) {
          if( TINF_INT64 == type && 1 == rank ) {
            if( (ptr=(int64_t*)malloc(dims[0]*sizeof(int64_t))) == NULL)
              throw std::runtime_error("tinf_problem_value: allocation error");
            *len = dims[0];
            value(key,(const void*)ptr,rank,dims);
            *val = ptr;
            return true;
          }
        }
      } catch (std::runtime_error e) {
        throw;
      }
      return false;
    }

    inline virtual bool value(const char* key, float** val, size_t* len)
    {
      enum TINF_DATA_TYPE type;
      int32_t rank;
      size_t dims[TINF_PROBLEM_MAX_RANK];
      float* ptr;
      try {
        if( defined(key, &type, &rank, dims) ) {
          if( TINF_FLOAT == type && 1 == rank ) {
            if( (ptr=(float*)malloc(dims[0]*sizeof(float))) == NULL)
              throw std::runtime_error("tinf_problem_value: allocation error");
            *len = dims[0];
            value(key,(const void*)ptr,rank,dims);
            *val = ptr;
            return true;
          }
        }
      } catch (std::runtime_error e) {
        throw;
      }
      return false;
    }

    inline virtual bool value(const char* key, double** val, size_t* len)
    {
      enum TINF_DATA_TYPE type;
      int32_t rank;
      size_t dims[TINF_PROBLEM_MAX_RANK];
      double* ptr;
      try {
        if( defined(key, &type, &rank, dims) ) {
          if( TINF_DOUBLE == type && 1 == rank ) {
            if( (ptr=(double*)malloc(dims[0]*sizeof(double))) == NULL)
              throw std::runtime_error("tinf_problem_value: allocation error");
            *len = dims[0];
            value(key,(const void*)ptr,rank,dims);
            *val = ptr;
            return true;
          }
        }
      } catch (std::runtime_error e) {
        throw;
      }
      return false;
    }

    inline virtual bool value(const char* key, std::complex<double>** val, size_t* len)
    {
      enum TINF_DATA_TYPE type;
      int32_t rank;
      size_t dims[TINF_PROBLEM_MAX_RANK];
      std::complex<double>* ptr;
      try {
        if( defined(key, &type, &rank, dims) ) {
          if( TINF_CMPLX_DOUBLE == type && 1 == rank ) {
            if( (ptr=(std::complex<double>*)malloc(dims[0]*sizeof(std::complex<double>))) == NULL)
              throw std::runtime_error("tinf_problem_value: allocation error");
            *len = dims[0];
            value(key,(const void*)ptr,rank,dims);
            *val = ptr;
            return true;
          }
        }
      } catch (std::runtime_error e) {
        throw;
      }
      return false;
    }

    inline virtual bool value(const char* key, char** val, size_t* len)
    {
      enum TINF_DATA_TYPE type;
      int32_t rank;
      size_t dims[TINF_PROBLEM_MAX_RANK];
      char* ptr;
      try {
        if( defined(key, &type, &rank, dims) ) {
          if( TINF_CHAR == type && 1 == rank ) {
            if( (ptr=(char*)malloc(dims[0]*sizeof(char))) == NULL)
              throw std::runtime_error("tinf_problem_value: allocation error");
            *len = dims[0];
            value(key,(const void*)ptr,rank,dims);
            *val = ptr;
            return true;
          }
        }
      } catch (std::runtime_error e) {
        throw;
      }
      return false;
    }

    inline virtual bool value(const char* key, bool** val, size_t* len)
    {
      enum TINF_DATA_TYPE type;
      int32_t rank;
      size_t dims[TINF_PROBLEM_MAX_RANK];
      bool* ptr;
      try {
        if( defined(key, &type, &rank, dims) ) {
          if( TINF_BOOL == type && 1 == rank ) {
            if( (ptr=(bool*)malloc(dims[0]*sizeof(bool))) == NULL)
              throw std::runtime_error("tinf_problem_value: allocation error");
            *len = dims[0];
            value(key,(const void*)ptr,rank,dims);
            *val = ptr;
            return true;
          }
        }
      } catch (std::runtime_error e) {
        throw;
      }
      return false;
    }

  private:
    void* problem;
};

#endif
