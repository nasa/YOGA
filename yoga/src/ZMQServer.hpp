namespace YOGA{
  template<class T>
  auto maybe_initialize(T& v,int)
  -> decltype(v.initialize(), void())
  {
      v.initialize();
  }

  template<class T>
  void maybe_initialize(T& v,long){
      // don't call initialize() if T doesn't have it
  }

  template<typename T>
  void initialize(T& v) {
      maybe_initialize(v,5);
  }

  template<typename T>
  ZMQMessager::Server<T>::Server(const T&& w, int channel) :isRunning(true),
                                                     worker(w),
                                                     serverChannel(channel),
                                                     context(),
                                                     socket(context,Socket::Pull),
                                                     server_thread([&](){run();})
  {
      initialize(worker);
      socket.bindSocketToOpenPort();
      portNumber = socket.portNumber();
      isPortNumberSet = true;
  }

  template<typename T>
  int ZMQMessager::Server<T>::retrievePortNumber(){
    while(not isPortNumberSet)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return portNumber;
  }

  template<typename T>
  T ZMQMessager::Server<T>::stop(){
      isRunning = false;
      server_thread.join();
      return worker;
  }

  template<typename T>
  void ZMQMessager::Server<T>::run(){
      while(isRunning){
          if(socket.hasMessage()){
              auto request_msg = socket.receive();
              worker.doWork(request_msg);
          }
          else{
              std::this_thread::sleep_for(std::chrono::milliseconds(1));
          }
      }
      socket.close();
  }
}
