#include "ZMQPostMan.h"
#include <Tracer.h>

namespace YOGA {

int extractPortNumberFromDestination(std::string s) {
    int pos = s.rfind(":") + 1;
    return std::stoi(s.substr(pos));
}

ZMQPostMan::ZMQPostMan(MessagePasser mp, std::set<int> message_types_callback)
    : context(),
      socket(context, ZMQMessager::Socket::Pull),
      client(std::make_shared<ZMQServerNameGenerator>(mp)),
      rank(mp.Rank()),
      callback_message_types(message_types_callback){
    socket.bindSocketToOpenPort();
    int port = socket.portNumber();
    ports_for_ranks = mp.Gather(port);

}
void ZMQPostMan::push(int target_rank, int message_type, MessagePasser::Message&& msg) {
    push(target_rank,message_type,std::make_shared<MessagePasser::Message>(msg));
}

void ZMQPostMan::push(int target_rank, int message_type, std::shared_ptr<MessagePasser::Message> msg) {
    long bytes = msg->size();
    Tracer::begin("push ("+std::to_string(bytes)+" bytes)");
    if(target_rank == rank){
        auto msg_copy = *msg;
        callbacks[message_type](std::move(msg_copy));
    }
    else{
        Guard lock(outbox_mtx);
        OutBoundMessage m;
        m.target_rank = target_rank;
        m.message_type = message_type;
        m.body = msg;
        outbox.push(m);
    }
    Tracer::end("push ("+std::to_string(bytes)+" bytes)");
}

std::shared_ptr<MessagePasser::Message> ZMQPostMan::MailBox::wait(int message_type) {
    while (true) {
        inbox_mtx.lock();
        if (not inboxes[message_type].empty()) {
            inbox_mtx.unlock();
            return retrieve(message_type);
        }
        inbox_mtx.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

std::shared_ptr<MessagePasser::Message> ZMQPostMan::MailBox::retrieve(int message_type) {
    auto ptr = inboxes[message_type].front();
    inboxes[message_type].pop();
    return ptr;
}

void ZMQPostMan::start() {
    for (int message_type : callback_message_types) {
        if (callbacks.count(message_type) != 1) throw std::logic_error("Must register all callbacks before starting");
    }
    server_thread = std::thread([&]() { listen(); });
    while(not is_running){
        short_sleep(10);
    }
}

void ZMQPostMan::stop() {
    is_running = false;
    server_thread.join();
}

void ZMQPostMan::registerCallBack(int message_type, std::function<void(MessagePasser::Message&&)> callback) {
    callbacks[message_type] = callback;
}

void ZMQPostMan::sendFirstMessage() {
    outbox_mtx.lock();
    if (not outbox.empty()) {
        auto m = outbox.front();
        outbox.pop();
        outbox_mtx.unlock();
        int target_rank = m.target_rank;
        bool forward = false;
        int forward_rank = m.target_rank;
        int target_port = ports_for_ranks[target_rank];
        Tracer::begin("connect");
        client.connectToServer(target_rank, target_port);
        Tracer::end("connect");
        Tracer::begin("make request");
        MessagePasser::Message header;
        header.pack(forward);
        header.pack(forward_rank);
        header.pack(m.message_type);
        size_t body_size = m.body->size();
        header.pack(body_size);
        client.send(header,true);
        Tracer::begin(std::to_string(target_rank));
        client.send(*m.body,false);
        Tracer::end(std::to_string(target_rank));
        Tracer::end("make request");
        Tracer::begin("disconnect");
        client.disconnectFromServer(target_rank, target_port);
        Tracer::end("disconnect");
    } else {
        outbox_mtx.unlock();
    }
}

void ZMQPostMan::MailBox::store(int message_type, MessagePasser::Message&& body) {
    Tracer::begin("store");
    Guard lock(inbox_mtx);
    inboxes[message_type].push(std::make_shared<MessagePasser::Message>(std::move(body)));
    Tracer::end("store");
}

ZMQPostMan::MailBox::MailBox(std::set<int> message_types) : message_types(message_types) {
    for (int type : message_types) {
        inboxes[type] = std::queue<std::shared_ptr<MessagePasser::Message>>();
    }
}

void ZMQPostMan::listen() {
    Tracer::begin("PostMan");
    is_running = true;
    while (is_running) {
        bool recvd = false;
        bool sent = false;
        if (socket.hasMessage()){
            recvd = true;
            auto msg = std::make_shared<MessagePasser::Message>(socket.receive());
            MessagePasser::Message body;
            bool forward = false;
            int forward_rank = 0;
            int message_type = 0;
            msg->unpack(forward);
            msg->unpack(forward_rank);
            msg->unpack(message_type);
            msg->unpack(body);
            Tracer::begin("unpacked: "+std::to_string(message_type));
            if(forward){
                forward = false;
                push(forward_rank,message_type,std::move(body));
            }
            else if (callback_message_types.count(message_type) == 1) {
                std::string event_name = "callback: " + std::to_string(message_type);
                Tracer::begin(event_name);
                callbacks[message_type](std::move(body));
                Tracer::end(event_name);
            } else {
                printf("PostMan: doesn't know how to handle messages of type %i\n", message_type);
            }
            Tracer::end("unpacked: "+std::to_string(message_type));
        }
        if (not outbox.empty()) {
            sent = true;
            sendFirstMessage();
        }
        if (not recvd and not sent) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
    socket.close();
    Tracer::end("PostMan");
}
}
