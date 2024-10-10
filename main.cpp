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

constexpr char corpus_path[] = "/Users/weedcuper/Documents/Projects/markov_name_gen/corpus3.txt";

class Parser
{
   public:
    static std::vector<std::string> parse_file (const std::string &path) {
        std::cout << "File path: " << path << std::endl;
        auto ret = std::vector<std::string>();
        std::ifstream file_stream(path, std::ios_base::in);

        for(std::string line; getline(file_stream, line);) {
            ret.push_back(line);
        }
        return ret;
    }

    static void print_corpus (const std::vector<std::string> &corpus) {
        for(auto const &n : corpus) {
            std::cout << n << std::endl;
        }
    }
};

class Model
{
    static constexpr char _start_mark[] = "#";
    static constexpr char _end_mark[] = "@";
    std::map<std::string, std::vector<char>> _chain;
    std::map<std::string, std::vector<float>> _weights;
    std::vector<std::string> _corpus;
    int _order;

   public:
    Model (int order, const std::vector<std::string> &corpus) : _corpus(corpus), _order(order) {
        _chain = std::map<std::string, std::vector<char>>();
        _weights = std::map<std::string, std::vector<float>>();
        generate_model();
    }

    void generate_model () {
        _chain[_start_mark] = std::vector<char>();
        for(const auto &n : _corpus) {
            std::string tmp_str = n + _end_mark;
            _chain[_start_mark].push_back(tmp_str[0]);
            //std::cout << "tmp_str: " << tmp_str << "len: " << tmp_str.length() - _order << std::endl;
            for(int i = 0; i < static_cast<int>(tmp_str.length()) - _order; ++i) {
                auto tmp = tmp_str.substr(i, _order);
                //std::cout << "tmp: " << tmp << std::endl;
                if(_chain.find(tmp) == _chain.end()) {
                    _chain[tmp] = std::vector<char>();
                }
                _chain[tmp].push_back(tmp_str[i + _order]);
            }
        }
    }

    void generate_weights () {
        for(const auto &n : _chain) {
            if(_weights.find(n.first) == _weights.end()) {
                _weights[n.first] = std::vector<float>();
            }
        }
    }

    char get_letter (std::string context) {
        char ret = 0;
        if(_chain.find(context) != _chain.end()) {
            ret = _chain[context][rand() % (_chain[context].size())];
        }
        return ret;
    }

    void print_chain () {
        for(const auto &n : _chain) {
            std::cout << n.first << ": [";
            for(const auto v : n.second) {
                std::cout << v << ", ";
            }
            std::cout << "]\n";
        }
    }
};

class Generator
{
    std::vector<Model> _models;
    int _order;

   public:
    Generator (int order, std::vector<std::string> corpus) : _order(order) {
        _models = std::vector<Model>();
        for(int i = _order; i > 0; --i) {
            auto model = Model(i, corpus);
            std::cout << "============== " << i << std::endl << std::endl;
            model.print_chain();
            _models.push_back(model);
        }
    }

    char generate_letter (std::string context) {
        char ret = 0;
        for(auto &n : _models) {
            ret = n.get_letter(context);
            if(0 != ret) {
                break;
            }
        }
        return ret;
    }

    std::string generate_word () {
        auto ret = std::string("");
        // _models[0].print_chain();
        auto letter = _models[0].get_letter("#");
        // std::cout << "letter: " << letter << std::endl;
        ret += letter;
        while(true) {
            auto tmp = ret.length() <= _order ? ret : ret.substr(ret.length() - _order, _order);
            std::cout << "tmp: " << tmp << std::endl;
            letter = generate_letter(tmp);
            if(letter == '@' || letter == 0) {
                break;
            }
            ret += letter;
        }
        return ret;
    }
};

int main (int argc, char *argv[]) {
    srand(time(NULL));
    auto corpus = std::vector<std::string>();
    corpus = Parser::parse_file(argc == 2 ? argv[1] : corpus_path);
    auto gen = Generator(3, corpus);
    for(int i = 0; i < 10;) {
        auto tmp = gen.generate_word();
        if(tmp.length() >= 6 && tmp.length() <= 10) {
            std::cout << tmp << std::endl;
            ++i;
        }
    }
}
