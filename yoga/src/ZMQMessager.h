#pragma once
#include <future>
#include <zmq.h>
#include "MessageTypes.h"
#include "ZMQServerNameGenerator.h"

namespace YOGA {

class ZMQMessager {
  public:

    class Context{
      public:
        Context(): context(zmq_ctx_new()){}
        void close(){
            zmq_ctx_destroy(context);
        }
        void* get(){return context;}
      private:
        void* context;
    };

    class Socket{
      public:
        enum Type {Push,Pull};
        Socket(Context& c, Type type): socket(zmq_socket(c.get(),zmqType(type))){}
        void close() {
            zmq_close(socket);
        }
        void connect(const std::string s){
            zmq_connect(socket,s.c_str());
        }
        void disconnect(const std::string s){
            zmq_disconnect(socket,s.c_str());
        }
        void send(const MessagePasser::Message& msg,int is_more_coming){
            int flag = is_more_coming ? ZMQ_SNDMORE : 0;
            int rc = zmq_send(socket,msg.data(),msg.size(),flag);
            if(rc != int(msg.size()))
                printf("ZMQ failed to send msg\n");
        }
        bool hasMessage(){
            zmq_pollitem_t  items[1];
            items[0].socket = socket;
            items[0].events = ZMQ_POLLIN;
            int status = zmq_poll(items,1,300);
            if(status == 0)
                return false;
            return (bool)(items[0].revents & ZMQ_POLLIN);
        }
        int portNumber(){
            size_t opt_size = 256;
            std::string buffer;
            buffer.resize(opt_size);
            zmq_getsockopt(socket,ZMQ_LAST_ENDPOINT,(void*)buffer.data(),&opt_size);
            return extractPortNumberFromDestination(buffer);
        }
        MessagePasser::Message receive() {
            while(not hasMessage()){
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            MessagePasser::Message msg;
            do {
                zmq_msg_t m;
                zmq_msg_init(&m);
                size_t message_length = zmq_recvmsg(socket,&m,0);
                char* msg_data = (char*)zmq_msg_data(&m);
                msg.pack(msg_data,message_length);
                zmq_msg_close(&m);
            } while(isThereMore());
            return msg;
        }
        bool isThereMore(){
            int more;
            size_t size(sizeof(more));
            zmq_getsockopt(socket,ZMQ_RCVMORE,&more,&size);
            return more;
        }

        void bindSocketToOpenPort() {
            std::string bindString = "tcp://*:*";
            try {
                int zero = 0;
                zmq_setsockopt(socket,ZMQ_LINGER, (void*)&zero, sizeof(int));
                zmq_bind(socket,bindString.c_str());
            } catch (...) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                throw std::logic_error("Caught zmq error");
            }
        }
      private:
        void* socket;

        int zmqType(Type type){
            switch (type) {
                case Push: return ZMQ_PUSH;
                case Pull: return ZMQ_PULL;
            }
            throw std::logic_error("Only push/pull sockets supported in yoga");
        }

        int extractPortNumberFromDestination(std::string s){
            int pos = s.rfind(":")+1;
            return std::stoi(s.substr(pos));
        }
    };

    template <typename Worker>
    class Server {
      public:
        Server(const Worker&& w, int channel);
        ~Server(){
            isRunning = false;
            try{
                server_thread.join();
            }
            catch(...){
                server_thread = {};
            }
            context.close();
        }
        Worker stop();
        int retrievePortNumber();

      private:
        bool isRunning;
        bool isPortNumberSet = false;
        Worker worker;
        int serverChannel;
        int portNumber;
        Context context;
        Socket socket;
        std::thread server_thread;

        void run();
    };


    class Client {
      public:
        Client(std::shared_ptr<ZMQServerNameGenerator> server_names);
        ~Client();
        void connectToServer(int serverRank, int serverPort);
        void disconnectFromServer(int id, int serverPort);
        void send(const MessagePasser::Message& msg,bool is_more_coming = false);

      private:
        std::shared_ptr<ZMQServerNameGenerator> server_names;
        Context context;
        Socket socket;
    };
};
}

#include "ZMQServer.hpp"
