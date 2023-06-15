#pragma once
#include <queue>
#include "Sleep.h"
#include "ZMQMessager.h"

namespace YOGA {
class ZMQPostMan {
  public:
    ZMQPostMan(MessagePasser mp, std::set<int> message_types_callback);
    void push(int target_rank, int message_type, MessagePasser::Message&& msg);
    void push(int target_rank, int message_type, std::shared_ptr<MessagePasser::Message> msg);
    void registerCallBack(int message_type,std::function<void(MessagePasser::Message&&)> callback);
    void start();
    void stop();




    class MailBox{
      public:
        MailBox(std::set<int> message_types);
        void store(int message_type,MessagePasser::Message&& m);
        std::shared_ptr<MessagePasser::Message> wait(int message_type);
        std::shared_ptr<MessagePasser::Message> retrieve(int inbox_id);
        bool isThereAMessage(int inbox_id) const{
            return not inboxes.at(inbox_id).empty();
        }
        void lock(){
            inbox_mtx.lock();
        }

        void unlock(){
            inbox_mtx.unlock();
        }

      private:
        std::mutex inbox_mtx;
        std::set<int> message_types;
        std::map<int, std::queue<std::shared_ptr<MessagePasser::Message>>> inboxes;
    };
  private:
    struct OutBoundMessage {
        int target_rank;
        int message_type;
        std::shared_ptr<MessagePasser::Message> body;
    };
    typedef std::lock_guard<std::mutex> Guard;
    typedef std::function<void(MessagePasser::Message&&)> CallBack;
    bool is_running = false;
    ZMQMessager::Context context;
    ZMQMessager::Socket socket;
    ZMQMessager::Client client;
    int rank;
    std::thread server_thread;
    std::mutex outbox_mtx;
    std::queue<OutBoundMessage> outbox;
    std::set<int> callback_message_types;
    std::map<int, CallBack> callbacks;
    std::vector<int> ports_for_ranks;

    void listen();
    void sendFirstMessage();
};
}
