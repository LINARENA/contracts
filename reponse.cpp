#include <eosiolib/eosio.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/asset.hpp>


using namespace eosio;
using std::string;

class reponse : public contract {
    private:
        //@abi table
        struct account{
            uint64_t pkey;

            account_name name;
            uint32_t point;
            
            auto primary_key()const {return pkey;}
            account_name get_name()const {return name;}
            EOSLIB_SERIALIZE( account, (pkey)(name)(point) )
        };

        
        //@abi table
        struct ques{
            uint64_t pkey;

            account_name name;
            uint32_t rewards;
            string hash;
            uint32_t stat;

            auto primary_key()const {return pkey;}
            EOSLIB_SERIALIZE( ques, (pkey)(name)(rewards)(hash)(stat) )
        };

        //@abi table
        struct answ{
            uint64_t pkey;

            account_name name;
            string hash;
            uint64_t question_id;

            auto primary_key()const {return pkey;}
            EOSLIB_SERIALIZE( answ, (pkey)(name)(hash)(question_id) )
        };

        //@abi table
        struct adop{
          uint64_t pkey;
          
          account_name quser;
          uint64_t qid;

          account_name auser;
          uint64_t aid;

          uint128_t hash;

          auto primary_key()const {return pkey;}
          EOSLIB_SERIALIZE( adop, (pkey)(quser)(qid)(auser)(aid)(hash) )
        };

        //@abi table
        struct voted{
          uint64_t pkey;

          account_name voter;
          string table_name;
          uint64_t content_id;
          uint128_t hash;

          auto primary_key()const {return pkey;}
          uint128_t get_hash()const {return hash;}
        };

        //@abi table
        struct badged{
          uint64_t pkey;
          
          account_name name;
          uint32_t point;
          string challenge;
          uint128_t hash;

          auto primary_key()const {return pkey;}
          uint128_t get_hash()const {return hash;}
        };

  typedef eosio::multi_index< N(account), account,
            eosio::indexed_by< N(byname), eosio::const_mem_fun<account, account_name, &account::get_name> > 
          > account_index;
  typedef eosio::multi_index< N(ques), ques 
          > question_index;
  typedef eosio::multi_index< N(answ), answ 
          > answer_index;
  typedef eosio::multi_index< N(adop), adop 
          > adopt_index;
  typedef eosio::multi_index< N(voted), voted,
            eosio::indexed_by< N(byhash), eosio::const_mem_fun<voted, uint128_t, &voted::get_hash> >
          > vote_index;
  typedef eosio::multi_index< N(badged), badged,
            eosio::indexed_by< N(byhash), eosio::const_mem_fun<badged, uint128_t, &badged::get_hash> >
          > badge_index;

        account_index accounts;
        question_index questions;
        answer_index answers; 
        adopt_index adopts;
        vote_index votes;
        badge_index badges;

        auto create_reponse_user(const account_name newb){
            auto itr = accounts.emplace( _self, [&](auto& nb ){
              nb.pkey = accounts.available_primary_key();
              nb.name = newb;
              nb.point = 0;
            });
            return itr;
        }

        asset calc_point_to_token(uint32_t point){
          // auto a = asset::from_string("1000.0000 RES");
          // 1000 point = 1$ = 20RES
          uint64_t amount = point * 10000 * 0.02;
          asset a(amount, S(4, RES));
          return a;
        }

    public:
        reponse(account_name self)
          :contract(self),
           accounts(_self, _self),
           questions(_self, _self),
           answers(_self, _self),
           adopts(_self, _self),
           votes(_self, _self),
           badges(_self, _self)
          {}

        //@abi action
        void newuser(const account_name newb){
            require_auth(newb);

            auto acnt_index = accounts.get_index<N(byname)>();
            auto itr = acnt_index.find(newb);
            eosio_assert(itr==acnt_index.end(), "user already registered");

            create_reponse_user(newb);
        }

        //@abi action
        void question(const account_name from, const uint32_t rewards, const string hash, const uint64_t question_id){
            require_auth(from);
            
            auto acnt_index = accounts.get_index<N(byname)>();
            auto itr = acnt_index.find(from);
            eosio_assert(itr != acnt_index.end(), "not registered user");

            eosio_assert(rewards >= 0, "rewards must be >0");
            eosio_assert(itr->point >= rewards, "not enough points");

            questions.emplace( _self, [&](auto & qu){
              qu.pkey = question_id;
              qu.name = from;
              qu.rewards = rewards;
              qu.hash = hash;
              qu.stat = 0;
            });
            acnt_index.modify(itr, 0, [&](auto& acnt){
              acnt.point -= rewards;
            });
        }

        //TODO: implement this
        void freeze_question(const string hash, const uint64_t question_id){
          auto q_itr = questions.find(question_id);
          eosio_assert(q_itr != questions.end(), "not registered question_id");
        }

        //@abi action
        void adopt(const account_name quser, const uint64_t question_id, const uint64_t answer_id){
            require_auth(quser);

            auto acnt_index = accounts.get_index<N(byname)>();
            auto acc_itr = acnt_index.find(quser);
            eosio_assert(acc_itr != acnt_index.end(), "quser account not found");

            auto ques_itr = questions.find(question_id);
            eosio_assert(ques_itr != questions.end(), "question id not found");
            eosio_assert(ques_itr->name == quser, "it's not your own question");
            eosio_assert(ques_itr->stat == 0, "question is already closed");

            auto answer_itr = answers.find(answer_id);
            eosio_assert(answer_itr != answers.end(), "answer id not found");
            eosio_assert(answer_itr->name != quser, "can't adopt own answer");

            acc_itr = acnt_index.find(answer_itr->name);
            eosio_assert(acc_itr != acnt_index.end(), "auser account not found (internal error)");

            adopts.emplace(_self, [&](auto &ado){
                ado.pkey = adopts.available_primary_key();
                ado.quser = quser;
                ado.qid = question_id;
                ado.auser = answer_itr->name;
                ado.aid = answer_id;
                });

            acnt_index.modify(acc_itr, 0, [&](auto &acnt){
              acnt.point += ques_itr->rewards;
            });

            questions.modify(ques_itr, 0, [&](auto &q){
              q.stat = 1;
            });
        }

        //@abi action
        void answer(const account_name auser, const string hash, const uint64_t question_id, const uint64_t answer_id){
            require_auth(_self);

            auto ques_itr = questions.find(question_id);
            eosio_assert(ques_itr != questions.end(), "fail to find that question id");

            auto acnt_index = accounts.get_index<N(byname)>();
            auto acc_itr = acnt_index.find(auser);
            eosio_assert(acc_itr != acnt_index.end(), "account is not found");

            answers.emplace(_self, [&](auto &a){
              //a.pkey = answers.available_primary_key();
              a.pkey = answer_id;
              a.name = auser;
              a.question_id = question_id;
              a.hash = hash;
            });
            
        }

        //@abi action
        void exchangep(const account_name acnt, uint32_t point){
          require_auth(_self); // only reponse account can execute

          // check valid user
          auto acnt_index = accounts.get_index<N(byname)>();
          auto itr = acnt_index.find(acnt);
          eosio_assert(itr != acnt_index.end(), "not registerd user");

          // check valid point
          eosio_assert(point >= 1, "point must greater than 1"); // TODO: set minimum exchange point
          eosio_assert(point <= itr->point, "point must less than user own");

          // exchange point
          auto quantity = calc_point_to_token(point);
          action(
              permission_level{_self, N(active)},
              N(eosio.token), N(transfer),
              std::make_tuple(_self, acnt, quantity, std::string("exchange"))
          ).send();

          acnt_index.modify(itr, 0, [&](auto& acnt){
              acnt.point -= point;
              });
        }

        //@abi action
        void vote(const account_name voter, const uint32_t point, string table_name, uint64_t content_id, string hash){
          require_auth(_self);
          auto acnt_index = accounts.get_index<N(byname)>();
          auto voter_itr = acnt_index.find(voter);
          eosio_assert(voter_itr != acnt_index.end(), "voter account not found");
          
          eosio_assert(point >= 1, "point must greater than 1");
          eosio_assert(table_name=="ques" || table_name=="answ", "table name must be 'ques' or 'answ'");

          if (table_name=="ques"){
            auto itr = questions.find(content_id);
            eosio_assert(itr != questions.end(), "not registered question_id");

            auto taker_itr = acnt_index.find(itr->name);
            eosio_assert(taker_itr != acnt_index.end(), "wrong taker name(internal error)");
            
            acnt_index.modify(taker_itr, 0, [&](auto& acnt){
              acnt.point += point;
            });
          }else if(table_name=="answ"){
            auto itr = answers.find(content_id);
            eosio_assert(itr != answers.end(), "not registered answer_id");
            
            auto taker_itr = acnt_index.find(itr->name);
            eosio_assert(taker_itr != acnt_index.end(), "wrong taker name(internal error)");
            
            acnt_index.modify(taker_itr, 0, [&](auto& acnt){
              acnt.point += point;
            });
          }

          //uint64_t hash = (uint64_t) voter + eosio::string_to_name(table_name.c_str()) + content_id;

          uint128_t hash_int = *(uint128_t *)hash.c_str();

          auto vote_index = votes.get_index<N(byhash)>();
          auto vote_itr = vote_index.find(hash_int);
          eosio_assert(vote_itr == vote_index.end(), "already voted");

          votes.emplace(_self, [&](auto &v){
              v.pkey = votes.available_primary_key();
              v.voter = voter;
              v.table_name = table_name;
              v.content_id = content_id;
              v.hash = hash_int;
              });
        }

        //@abi action
        void badge(const account_name acnt, const uint32_t point, const string challenge, string hash){
          require_auth(_self);

          uint128_t hash_int = *(uint128_t*) hash.c_str();
          auto badge_index = badges.get_index<N(byhash)>();
          auto badge_itr = badge_index.find(hash_int);
          eosio_assert(badge_itr==badge_index.end(), "already has same badge");

          auto acnt_index = accounts.get_index<N(byname)>();
          auto acnt_itr = acnt_index.find(acnt);
          eosio_assert(acnt_itr != acnt_index.end(), "unknown user");

          acnt_index.modify(acnt_itr, 0, [&](auto& acnt){
              acnt.point += point;
              });

          badges.emplace(_self, [&](auto &b){
              b.pkey = badges.available_primary_key();
              b.name = acnt;
              b.point = point;
              b.challenge = challenge;
              b.hash = hash_int;
              });

        }

};

EOSIO_ABI(reponse, (newuser)(question)(adopt)(answer)(exchangep)(vote)(badge) )
