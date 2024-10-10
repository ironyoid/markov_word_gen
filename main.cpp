#include <cstdlib>
#include <ctime>
#include <new>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include "map"
#include "vector"
#include <ios>
#include <fstream>

constexpr char corpus_path[] = "/home/nikitapichugin/Documents/Projects/markov_name_gen/corpus0.txt";

class Parser {
    public:
     static std::vector<std::string> parse_file(const std::string &path) {
         auto ret = std::vector<std::string>();
         std::ifstream file_stream(path, std::ios_base::in);
         
         for( std::string line; getline( file_stream, line ); ) {
             ret.push_back(line);
         }
         return ret;
     }

     static void print_corpus(const std::vector<std::string> &corpus) {
        for(auto const &n : corpus) {
            std::cout << n << std::endl;
        }
     }
};

class Model {
    static constexpr char _start_mark[] = "#";
    static constexpr char _end_mark[] = "@";
    std::map<std::string, std::vector<char>> _chain;
    std::vector<std::string> _corpus;
    int _order;
    public:
     Model(int order, const std::vector<std::string> &corpus) : _order(order), _corpus(corpus) {
         _chain = std::map<std::string, std::vector<char>>();
         generate_model();
     }
 
     void generate_model() {
            _chain[_start_mark] = std::vector<char>();
         for(const auto &n : _corpus){
             std::string tmp_str = n + _end_mark;
             _chain[_start_mark].push_back(tmp_str[0]);
             /* std::cout << "tmp_str[0]: " << tmp_str[0] << std::endl; */
             for(int i = 0; i < tmp_str.length() - _order; ++i){
                 auto tmp = tmp_str.substr(i, _order);
                 /* std::cout << "tmp: " << tmp << std::endl; */
                 if(_chain.find(tmp) == _chain.end()) {
                     _chain[tmp] = std::vector<char>();
                 }
                 _chain[tmp].push_back(tmp_str[i+_order]);
             }
         }
     }

     char get_letter(std::string context) {
        char ret = 0;
        if(_chain.find(context) != _chain.end()) {
            ret = _chain[context][rand() % (_chain[context].size())];
        }
        return ret;
     }

     std::string get_word() {
        auto ret = std::string("");
        auto start = _chain[_start_mark][rand() % (_chain[_start_mark].size()-1)];
        ret += start;
        int i = 0;
        while(true) {
            auto tmp = ret.substr(i, _order);
            if(_chain.find(tmp) != _chain.end()) {
                auto c = _chain[tmp][rand() % (_chain[tmp].size()-1)];
                if(c != _end_mark[0]) {
                    ret += c;
                } else {
                    break;
                }
            }
        }
        return ret;
     }

     void print_chain() {
        for(const auto &n : _chain) {
            std::cout << n.first << ": [";
            for(const auto v : n.second) {
                std::cout << v << ", ";
            }
            std::cout << "]\n";
        }
     }
};

class Generator {
    std::vector<Model> _models;
    int _order;
    public:
     Generator(int order, std::vector<std::string> corpus) : _order(order) {
        _models = std::vector<Model>();
        for(int i = _order; i > 0; --i) {
            _models.push_back(Model(i, corpus));
        }
     }

     char generate_letter(std::string context) {
        char ret = 0;
        for(auto &n : _models) {
            ret = n.get_letter(context);
            if(0 != ret) {
                break;
            } 
        }
        return ret;
     }

     std::string generate_word() {
        auto ret = std::string("");
        // _models[0].print_chain();
        auto letter = _models[0].get_letter("#");
        // std::cout << "letter: " << letter << std::endl;
        ret += letter;
        while(true) {
            auto tmp = ret.length() <= _order ? ret : ret.substr(ret.length() - _order, _order);
            letter = generate_letter(tmp);
            if(letter == '@' || letter == 0){
                break;
            }
            ret += letter;
        }
        return ret;
     }
};

int main() {
    srand(time(NULL));
    auto corpus = Parser::parse_file(corpus_path);
    auto gen = Generator(5, corpus);
    std::cout << gen.generate_word() << std::endl;
}   
