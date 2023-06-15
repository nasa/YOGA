#pragma once

class MessageStatus {
  public:
    inline MessageStatus() : r(std::make_shared<MPI_Request>()){}
    inline MPI_Request* request() { return r.get(); }
    inline void wait() {
        MPI_Status no_one_cares;
        MPI_Wait(request(), &no_one_cares);
    }
    inline bool isComplete() {
        int flag = 0;
        MPI_Test(request(), &flag, MPI_STATUS_IGNORE);
        return flag;
    }
    inline bool waitFor(double remaining_time_to_wait_in_seconds){
        int flag = 0;
        double five_milliseconds  = 5e-3;
        while(remaining_time_to_wait_in_seconds > 0.0){
            MPI_Test(request(), &flag, MPI_STATUS_IGNORE);
            if(flag) return flag;
            remaining_time_to_wait_in_seconds -= five_milliseconds;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return flag;
    }
    inline virtual ~MessageStatus() {}

  protected:
    std::shared_ptr<MPI_Request> r;
};

