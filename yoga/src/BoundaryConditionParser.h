#pragma once
#include <string>
#include <queue>
#include <vector>
#include <set>
#include <parfait/Lexer.h>
#include <parfait/StringTools.h>


namespace YOGA{

    struct DomainBoundaryInfo{
        std::string name;
        int importance = 0;
        std::vector<int> solid_wall_tags;
        std::vector<int> interpolation_tags;
        std::vector<int> x_symmetry_tags;
        std::vector<int> y_symmetry_tags;
        std::vector<int> z_symmetry_tags;
    };

    class BoundaryInfoParser{
    public:

      static std::vector<DomainBoundaryInfo> parse(const std::string& s){
          auto tokens = getTokensAsQueue(s);
          std::vector<DomainBoundaryInfo> domains;
          while(not tokens.empty()){
              auto next = tokens.front();
              tokens.pop();
              if(domain() == next.to_string())
                  extractDomain(domains, tokens);
              else
                  throwParseError(next);
          }
          return domains;
      }

    private:

      static void extractDomain(std::vector<DomainBoundaryInfo>& domains, std::queue<Parfait::Lexer::Token>& tokens){
          std::vector<std::string> domain_names_to_add;
          while(not tokens.empty() and isDomainName(tokens.front())) {
              domain_names_to_add.push_back(extractName(tokens.front()));
              tokens.pop();
          }
          DomainBoundaryInfo d;
          while(not tokens.empty()){
              auto next = tokens.front();
              if(solid() == next.to_string()){
                  tokens.pop();
                  d.solid_wall_tags = extractIntegers(tokens);
              }
              else if(interpolation() == next.to_string()){
                  tokens.pop();
                  d.interpolation_tags = extractIntegers(tokens);
              }
              else if(x_symmetry() == next.to_string()){
                  tokens.pop();
                  d.x_symmetry_tags = extractIntegers(tokens);
              }
              else if(y_symmetry() == next.to_string()){
                  tokens.pop();
                  d.y_symmetry_tags = extractIntegers(tokens);
              }
              else if(z_symmetry() == next.to_string()){
                  tokens.pop();
                  d.z_symmetry_tags = extractIntegers(tokens);
              }
              else if(domain() == next.to_string()){
                  break;
              }
              else if(importance() == next.to_string()){
                  tokens.pop();
                  d.importance = extractInteger(tokens.front());
                  tokens.pop();
              }
              else{
                  throwParseError(next);
              }
          }

          for(auto name:domain_names_to_add) {
              domains.push_back(d);
              domains.back().name = name;
          }
      }

      static void throwParseError(const Parfait::Lexer::Token& token){
          printf("Unexpected token: %s\n",token.to_string().c_str());
          printf("on line %i col %i\n",token.lineNumber(),token.column());
          fflush(stdout);
          throw std::logic_error("BoundaryConditionParser Error");
      }

        static std::vector<int> extractIntegers(std::queue<Parfait::Lexer::Token>& tokens){
          std::vector<int> tags;
          while(not tokens.empty()){
              if(isInteger(tokens.front())) {
                  tags.push_back(extractInteger(tokens.front()));
                  tokens.pop();
              }
              else if(range() == tokens.front().to_string()){
                  tokens.pop();
                  int first_tag_in_range = tags.back();
                  int last_tag_in_range = extractInteger(tokens.front());
                  for(int i=first_tag_in_range+1;i<=last_tag_in_range;i++)
                      tags.push_back(i);
                  tokens.pop();
              }
              else{
                  break;
              }
          }
          return tags;
      }


        inline static Parfait::Lexer::TokenMap bcSyntax() {
            Parfait::Lexer::TokenMap token_map;
            token_map.push_back({Parfait::Lexer::Matchers::whitespace(), ""});
            token_map.push_back({Parfait::Lexer::Matchers::comment("#"),""});
            token_map.push_back({"domain", domain()});
            token_map.push_back({"x-symmetry", x_symmetry()});
            token_map.push_back({"y-symmetry", y_symmetry()});
            token_map.push_back({"z-symmetry", z_symmetry()});
            token_map.push_back({"interpolation", interpolation()});
            token_map.push_back({"importance", importance()});
            token_map.push_back({"solid", solid()});
            token_map.push_back({":", range()});
            token_map.push_back({Parfait::Lexer::Matchers::dashed_word(), "#R"});
            token_map.push_back({Parfait::Lexer::Matchers::c_variable(), "#R"});
            token_map.push_back({Parfait::Lexer::Matchers::integer(), "IR"});
            return token_map;
        }

        static bool isDomainName(const Parfait::Lexer::Token& token){
            auto s = token.to_string();
            return s.front() == '#';
        }
        static bool isInteger(const Parfait::Lexer::Token& token){
            auto s = token.to_string();
            return s.front() == 'I' and s != interpolation();
        }

        static std::string domain(){return "DO";}
        static std::string importance(){return "IM";}
        static std::string x_symmetry(){return "XS";}
        static std::string y_symmetry(){return "YS";}
        static std::string z_symmetry(){return "ZS";}
        static std::string interpolation(){return "IN";}
        static std::string solid(){return "SO";}
        static std::string range(){return "CO";}

        static std::queue<Parfait::Lexer::Token> getTokensAsQueue(const std::string& s){
            auto lexer = Parfait::Lexer(bcSyntax());
            auto tokens = lexer.tokenize(s);
            std::queue<Parfait::Lexer::Token> q;
            for(auto token:tokens)
                q.push(token);
            return q;
        }

        static int extractInteger(const Parfait::Lexer::Token& token){
            auto s = token.to_string();
            s.erase(0,1);
            return Parfait::StringTools::toInt(s);
        }

        static std::string extractName(const Parfait::Lexer::Token& token){
            auto s = token.to_string();
            s.erase(0,1);
            return s;
        }
    };
}
