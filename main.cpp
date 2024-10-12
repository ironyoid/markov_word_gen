#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "map"
#include "vector"
#include <ios>
#include <fstream>
#include <stdint.h>

static constexpr char k_animals_path[] = "/Users/weedcuper/Documents/Projects/markov_name_gen/res/animals";
static constexpr char k_adjectives_path[] = "/Users/weedcuper/Documents/Projects/markov_name_gen/res/adjectives";
static constexpr char k_other_path[] = "/Users/weedcuper/Documents/Projects/markov_name_gen/res/other";
static constexpr char k_alphabet[] = "@abcdefghijklmnopqrstuvwxyz";
static constexpr char k_end_mark[] = "@";
static constexpr char k_start_mark[] = "#";

namespace HelperFunctions {
    static void push_int (std::vector<uint8_t> &vec, uint32_t val) {
        vec.push_back(static_cast<uint8_t>(val));
        vec.push_back(static_cast<uint8_t>(val >> 8));
        vec.push_back(static_cast<uint8_t>(val >> 16));
        vec.push_back(static_cast<uint8_t>(val >> 24));
    }

    static uint32_t pull_int (std::vector<uint8_t> &vec, uint32_t pos) {
        assert(pos <= vec.size() - sizeof(uint32_t) && "position must be less than vec size");
        uint32_t ret = 0;
        ret |= vec[pos++];
        ret |= vec[pos++] << 8;
        ret |= vec[pos++] << 16;
        ret |= vec[pos] << 24;
        return ret;
    }

    void push_string (std::vector<uint8_t> &vec, const std::string &str) {
        push_int(vec, static_cast<uint32_t>(str.length()));
        for(const auto n : str) {
            vec.push_back(n);
        }
    }

    std::string pull_string (const std::vector<uint8_t> &vec, int pos, int len) {
        auto ret = std::string();
        for(int i = pos; i < pos + len; ++i) {
            ret += vec[i];
        }
        return ret;
    }

    std::string start (const int order) {
        auto ret = std::string("");
        for(int i = 0; i < order; ++i) {
            ret += k_start_mark;
        }
        return ret;
    }
}; // namespace HelperFunctions

class AlphabetMap
{
    std::map<char, uint32_t> _map;

   public:
    uint32_t &operator[](char key) {
        return _map[key];
    }
    AlphabetMap () {
        _map = std::map<char, uint32_t>();
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

    void deserialize (std::vector<uint8_t> vec, int pos, int len) {
        assert(vec.size() != 0 && "vec must contain at least one element");
        int letter_cnt = 0;
        for(int i = pos; i < pos + len; i += 4) {
            auto tmp = HelperFunctions::pull_int(vec, i);
            _map[k_alphabet[letter_cnt]] = tmp;
            ++letter_cnt;
        }
    }

    std::vector<uint8_t> serialize () {
        auto ret = std::vector<uint8_t>();
        HelperFunctions::push_int(ret, _map.size() * sizeof(int));
        for(const auto n : _map) {
            HelperFunctions::push_int(ret, n.second);
        }
        return ret;
    }
};

class Parser
{
   public:
    static std::vector<std::string> parse_file (const std::string &path) {
        auto ret = std::vector<std::string>();
        std::ifstream file_stream(path, std::ios_base::in);
        if(file_stream) {
            for(std::string line; getline(file_stream, line);) {
                ret.push_back(line);
            }
        } else {
            std::cout << "Can not open file" << path << std::endl;
        }
        return ret;
    }

    static std::vector<uint8_t> load_model (const std::string &path) {
        std::ifstream file_stream(path, std::ios::binary);
        auto ret = std::vector<uint8_t>();
        if(file_stream) {
            ret = std::vector<uint8_t>((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
        } else {
            std::cout << "Can not open file" << path << std::endl;
        }
        return ret;
    }

    static void save_model (const std::string &path, const std::vector<uint8_t> &vec) {
        auto ret = std::vector<uint8_t>();
        std::ofstream file_stream(path, std::ios_base::binary | std::ios::app);
        if(file_stream) {
            file_stream.write((char *) &vec[0], vec.size() * sizeof(uint8_t));
        } else {
            std::cout << "Can not open file" << path << std::endl;
        }
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

    int _order;
    int _gain;

   public:
    Model (int order, int gain) : _order(order), _gain(gain) {
        _chain = std::map<std::string, AlphabetMap>();
    }

    void generate_model (const std::vector<std::string> &corpus) {
        //_chain[start()] = AlphabetMap();
        for(const auto &n : corpus) {
            std::string tmp_str = HelperFunctions::start(_order) + n + k_end_mark;
            //_chain[start()][tmp_str[0]] += _gain;
            for(int i = 0; i < static_cast<int>(tmp_str.length()) - _order; ++i) {
                auto tmp = tmp_str.substr(i, _order);
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

    std::vector<uint8_t> serialize () {
        auto ret = std::vector<uint8_t>();
        for(auto &n : _chain) {
            HelperFunctions::push_string(ret, n.first);
            auto tmp = n.second.serialize();
            ret.insert(ret.end(), tmp.begin(), tmp.end());
        }
        return ret;
    }
    void deserialize (std::vector<uint8_t> vec) {
        assert(vec.size() != 0 && "vec must contain at least one element");
        long unsigned int pos = 0;
        while(pos < vec.size()) {
            auto str_len = HelperFunctions::pull_int(vec, pos);
            pos += 4;
            auto str = HelperFunctions::pull_string(vec, pos, str_len);
            pos += str_len;
            auto alp_len = HelperFunctions::pull_int(vec, pos);
            pos += 4;
            auto alp = AlphabetMap();
            alp.deserialize(vec, pos, alp_len);
            pos += alp_len;
            _chain[str] = alp;
        }
    }
};

class Generator
{
    std::vector<Model> _models;
    std::string _path;
    int _order;
    int _gain;

   public:
    Generator (int order, int gain, std::string path, std::vector<std::string> corpus) :
        _path(path),
        _order(order),
        _gain(gain) {
        _models = std::vector<Model>();
        if(!corpus.empty()) {
            generate_models(corpus);
        } else {
            throw std::invalid_argument("reveived empty corpus");
        }
    }

    void generate_models (std::vector<std::string> &corpus) {
        for(int i = _order; i > 0; --i) {
            std::string path = _path + "/" + "model" + std::to_string(i) + ".bin";
            auto model = Model(i, _gain);
            auto tmp = Parser::load_model(path);
            if(!tmp.empty()) {
                std::cout << "Load: " << path << std::endl;
                model.deserialize(tmp);
            } else {
                std::cout << "Save: " << path << std::endl;
                model.generate_model(corpus);
                Parser::save_model(path, model.serialize());
            }
            _models.push_back(model);
        }
    }

    char generate_letter (std::string context) {
        char ret = 0;
        auto tmp = context.substr(context.length() - _order, _order);
        for(auto &n : _models) {
            ret = n.get_letter(tmp);
            if(0 == ret || k_end_mark[0] == ret) {
                tmp = tmp.substr(1, tmp.length() - 1);
            } else {
                break;
            }
        }
        return ret;
    }

    std::string generate_word () {
        auto ret = HelperFunctions::start(_order);
        auto letter = generate_letter(ret);
        while(k_end_mark[0] != letter && 0 != letter) {
            if(0 != letter) {
                ret += letter;
            }
            letter = generate_letter(ret);
        }
        return ret.substr(_order, ret.length() - _order);
    }

    std::string generate_word (int min, int max) {
        while(true) {
            auto tmp = generate_word();
            if(tmp.length() >= min && tmp.length() <= max) {
                return tmp;
            }
        }
    }

    void print_models () {
        for(auto &n : _models) {
            n.print_chain();
            std::cout << std::endl;
        }
    }
};

int main (int argc, char *argv[]) {
    constexpr int k_min_word_len = 6;
    constexpr int k_max_word_len = 12;
    constexpr int k_num_of_words = 10;
    srand(time(NULL));

    if(argc == 3) {
        auto anm = Generator(3, 200, k_animals_path, Parser::parse_file(std::string(k_animals_path) + "/corpus.txt"));
        auto adj
            = Generator(3, 500, k_adjectives_path, Parser::parse_file(std::string(k_adjectives_path) + "/corpus.txt"));
        //gen.print_models();
        for(int i = 0; i < k_num_of_words; ++i) {
            std::cout << adj.generate_word(k_min_word_len, k_max_word_len) << " "
                      << anm.generate_word(k_min_word_len, k_max_word_len) << std::endl;
        }
        // int order = std::stoi(std::string(argv[1]));
        // int gain = std::stoi(std::string(argv[2]));
        // auto adj_gen
        //     = Generator(order, gain, k_other_path, Parser::parse_file(std::string(k_other_path) + "/corpus.txt"));
        // //adj_gen.print_models();
        // std::cout << adj_gen.generate_word(k_min_word_len, k_max_word_len) << std::endl;

    } else {
        std::cout << "Wrong arguments!" << std::endl;
    }
}
