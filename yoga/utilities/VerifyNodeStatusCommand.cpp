#include <stdio.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <parfait/JsonParser.h>
#include <parfait/FileTools.h>

class VerifyStatusCountsCommand : public inf::SubCommand {
  public:
    std::string description() const override { return "Verify node status counts against golden file"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag(inf::Alias::help(), inf::Help::help());
        m.addParameter({"file"},"output to verify", true);
        m.addParameter({"golden"}, "`golden` reference file", true);
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto current = importJson(m.get("file"));
        auto golden = importJson(m.get("golden"));
        if(not haveMatchingCounts(current, golden))
            exit(1);
    }

  private:

    Parfait::Dictionary importJson(const std::string& json_filename){
        auto contents = Parfait::FileTools::loadFileToString(json_filename);
        return Parfait::JsonParser::parse(contents);
    }

    bool haveMatchingCounts(const Parfait::Dictionary& actual, const Parfait::Dictionary& expected){
        auto& a = actual.at("node-statuses");
        auto& b = expected.at("node-statuses");
        return isMatch(a,b,"in") and isMatch(a,b,"out") and isMatch(a,b,"receptor") and isMatch(a,b,"orphan");
    }

    bool isMatch(const Parfait::Dictionary& a, const Parfait::Dictionary& b,const std::string& key){
        int ex = a.at(key).asInt();
        int ac = b.at(key).asInt();
        if(ex != ac){
            printf("Expected (%s): %i   Actual: %i\n",key.c_str(),ex,ac);
            return false;
        }
        return true;
    }
};

CREATE_INF_SUBCOMMAND(VerifyStatusCountsCommand)
