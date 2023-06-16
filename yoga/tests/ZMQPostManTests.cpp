#include <RingAssertions.h>
#include "ZMQPostMan.h"

using namespace YOGA;

void addThreeAndForwardToSender(ZMQPostMan& post_man,MessagePasser::Message body){

    int sending_rank = 0;
    body.unpack(sending_rank);
    int reply_message_type = 0;
    int n = 0;
    body.unpack(n);
    n += 3;
    MessagePasser::Message reply;
    reply.pack(n);
    post_man.push(sending_rank,reply_message_type,std::move(reply));
}

TEST_CASE("Make sure callbacks are registered before starting"){
    MessagePasser mp(MPI_COMM_WORLD);
    ZMQPostMan postman(mp,{0,42,98});

    REQUIRE_THROWS(postman.start());

}

TEST_CASE("Create postman"){
    MessagePasser mp(MPI_COMM_WORLD);
    ZMQPostMan postman(mp,{0,42,98});

    ZMQPostMan::MailBox mailbox({0,42});

    postman.registerCallBack(98,[&](MessagePasser::Message&& m){addThreeAndForwardToSender(postman,m);});
    postman.registerCallBack(0,[&](MessagePasser::Message&& m){mailbox.store(0,std::move(m));});
    postman.registerCallBack(42,[&](MessagePasser::Message&& m){mailbox.store(42,std::move(m));});
    postman.start();

    MessagePasser::Message msg_1,msg_2;
    msg_1.pack(mp.Rank());
    msg_1.pack(int(7));
    int target_rank = mp.Rank();
    int message_type = 98;
    postman.push(target_rank,message_type,std::move(msg_1));

    message_type = 42;
    msg_2.pack(double(.987));
    postman.push(target_rank,message_type,std::move(msg_2));

    while(not mailbox.isThereAMessage(42)){
        short_sleep(1);
    }
    mailbox.lock();
    if(mailbox.isThereAMessage(42)){
        auto reply_2 = mailbox.retrieve(42);
        double x;
        reply_2->unpack(x);
        REQUIRE(.987 == Approx(x));
    }
    mailbox.unlock();


    auto msg = mailbox.wait(0);

    int n = 0;
    msg->unpack(n);
    REQUIRE(10 == n);
    postman.stop();

}
