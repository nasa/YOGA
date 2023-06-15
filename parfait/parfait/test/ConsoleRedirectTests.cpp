#include <RingAssertions.h>
#include <parfait/ConsoleRedirect.h>
#include <MessagePasser/MessagePasser.h>

TEST_CASE("Console redirect works with printf") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 1) return;
    Parfait::Console console;
    console.redirect("shhh.log");
    printf("This goes to shhh.log\n");
    console.reset();
    printf("this goes to the normal console\n");

    auto contents = Parfait::FileTools::loadFileToString("shhh.log");
    REQUIRE(contents == "This goes to shhh.log\n");
    REQUIRE(0 == remove("shhh.log"));
}