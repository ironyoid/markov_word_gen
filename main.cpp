#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include "map"
#include "vector"
#include <ios>
#include <fstream>

static constexpr char k_alphabet[] = "@abcdefghijklmnopqrstuvwxyz";
static constexpr char k_end_mark[] = "@";
static constexpr char k_start_mark[] = "#";
class AlphabetMap
{
    std::map<char, int> _map;

   public:
    int &operator[](char key) {
        return _map[key];
    }
    AlphabetMap() {
        _map = std::map<char, int>();
        for(const auto &c : k_alphabet) {
            if(c != 0) {
                _map[c] = 1;
            }
        }
    }

    std::vector<int> get_vector () {
        auto ret = std::vector<int>();
        int acc = 0;
        for(const auto n : _map) {
            acc += n.second;
            ret.push_back(acc);
        }
        return ret;
    }

    void print_map () {
        for(const auto &n : _map) {
            std::cout << "{" << n.first << ": " << n.second << "} ";
        }
    }
};

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
    std::map<std::string, AlphabetMap> _chain;
    std::map<std::string, AlphabetMap> _weights;
    std::vector<std::string> _corpus;
    int _order;
    int _gain;

   public:
    Model(int order, int gain, const std::vector<std::string> &corpus) : _corpus(corpus), _order(order), _gain(gain) {
        _chain = std::map<std::string, AlphabetMap>();
        _weights = std::map<std::string, AlphabetMap>();
        generate_model();
    }

    void generate_model () {
        _chain[k_start_mark] = AlphabetMap();
        for(const auto &n : _corpus) {
            std::string tmp_str = n + k_end_mark;
            _chain[k_start_mark][tmp_str[0]] += _gain;
            //std::cout << "tmp_str: " << tmp_str << "len: " << tmp_str.length() - _order << std::endl;
            for(int i = 0; i < static_cast<int>(tmp_str.length()) - _order; ++i) {
                auto tmp = tmp_str.substr(i, _order);
                //std::cout << "tmp: " << tmp << std::endl;
                if(_chain.find(tmp) == _chain.end()) {
                    _chain[tmp] = AlphabetMap();
                }
                _chain[tmp][tmp_str[i + _order]] += _gain;
            }
        }
    }

    char get_letter (std::string context) {
        char ret = 0;
        if(_chain.find(context) != _chain.end()) {
            auto tmp = _chain[context].get_vector();
            auto rnd = rand() % tmp.back();
            for(int i = 0; i < static_cast<int>(tmp.size()); ++i) {
                if(rnd <= tmp[i]) {
                    ret = k_alphabet[i];
                    break;
                }
            }
            //std::cout << "ret: " << ret << std::endl;
        }
        return ret;
    }

    void print_chain () {
        for(auto &n : _chain) {
            std::cout << n.first << ": [";
            n.second.print_map();
            std::cout << "]\n";
        }
    }
};

class Generator
{
    std::vector<Model> _models;
    int _order;
    int _gain;

   public:
    Generator(int order, int gain, std::vector<std::string> corpus) : _order(order), _gain(gain) {
        _models = std::vector<Model>();
        for(int i = _order; i > 0; --i) {
            auto model = Model(i, _gain, corpus);
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
        auto letter = _models[0].get_letter(k_start_mark);
        ret += letter;
        while(true) {
            auto tmp = static_cast<int>(ret.length()) <= _order ? ret : ret.substr(ret.length() - _order, _order);
            letter = generate_letter(tmp);
            if(letter == k_end_mark[0] || letter == 0) {
                break;
            }
            ret += letter;
        }
        return ret;
    }
};

int main (int argc, char *argv[]) {
    constexpr int k_min_word_len = 6;
    constexpr int k_max_word_len = 10;
    constexpr int k_num_of_words = 10;

    srand(time(NULL));
    auto corpus = std::vector<std::string>();
    if(argc == 4) {
        char *path = argv[3];
        int order = std::stoi(std::string(argv[1]));
        int gain = std::stoi(std::string(argv[2]));
        corpus = Parser::parse_file(path);
        auto gen = Generator(order, gain, corpus);
        for(int i = 0; i < k_num_of_words;) {
            auto tmp = gen.generate_word();
            if(tmp.length() >= k_min_word_len && tmp.length() <= k_max_word_len) {
                std::cout << tmp << std::endl;
                ++i;
            }
        }
    } else {
        std::cout << "Wrong arguments!" << std::endl;
    }
}
