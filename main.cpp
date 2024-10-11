#include <cassert>
#include <cstddef>
#include <cstdint>
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
#include <stdint.h>

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
}; // namespace HelperFunctions

class AlphabetMap
{
    std::map<char, uint32_t> _map;

   public:
    uint32_t &operator[](char key) {
        return _map[key];
    }
    AlphabetMap() {
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
        std::cout << "File path: " << path << std::endl;
        auto ret = std::vector<std::string>();
        std::ifstream file_stream(path, std::ios_base::in);

        for(std::string line; getline(file_stream, line);) {
            ret.push_back(line);
        }
        return ret;
    }

    static std::vector<uint8_t> load_model (const std::string &path) {
        std::cout << "File path: " << path << std::endl;
        //auto ret = std::vector<uint8_t>();
        std::ifstream file_stream(path, std::ios::binary);
        std::vector<uint8_t> ret((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
        return ret;
    }

    static void save_model (const std::string &path, std::vector<uint8_t> &vec) {
        std::cout << "File path: " << path << std::endl;
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
    Model(int order, int gain) : _order(order), _gain(gain) {
        _chain = std::map<std::string, AlphabetMap>();
        auto m1 = AlphabetMap();
        auto m2 = AlphabetMap();
        m1['z'] = 55;
        m2['z'] = 77;
        _chain["abcd"] = m1;
        _chain["efgh"] = m2;
        //generate_model(corpus);
    }

    void generate_model (const std::vector<std::string> &corpus) {
        _chain[k_start_mark] = AlphabetMap();
        for(const auto &n : corpus) {
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
    Generator(int order, int gain, std::string path, std::vector<std::string> corpus) :
        _path(path),
        _order(order),
        _gain(gain) {
        _models = std::vector<Model>();
        generate_models(corpus);
    }

    void generate_models (std::vector<std::string> &corpus) {
        for(int i = _order; i > 0; --i) {
            auto model = Model(i, _gain);
            auto tmp = Parser::load_model(_path + "/" + "model" + std::to_string(i) + ".bin");
            if(!tmp.empty()) {
                std::cout << "load\n";
                model.deserialize(tmp);
            } else {
                std::cout << "gen\n";
                model.generate_model(corpus);
            }
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

    void print_models () {
        for(auto &n : _models) {
            n.print_chain();
            std::cout << std::endl;
        }
    }
};

int main (int argc, char *argv[]) {
    auto corpus = std::vector<std::string>();
    auto model = Model(1, 2);
    auto ser = model.serialize();
    model.print_chain();
    Parser::save_model("/home/nikitapichugin/Documents/Projects/markov_word_gen/res/model1.bin", ser);

    auto gen = Generator(1, 1, "/home/nikitapichugin/Documents/Projects/markov_word_gen/res", corpus);
    gen.print_models();
    // auto restore = Parser::load_model("/home/nikitapichugin/Documents/Projects/markov_word_gen/res/model0.bin");
    // model.deserialize(ser);
    // model.print_chain();
    // auto map1 = AlphabetMap();
    // auto map2 = AlphabetMap();
    // int cnt = 5;
    // for(auto n : k_alphabet) {
    //     if(n != 0) {
    //         map1[n] = cnt += 5;
    //     }
    // }
    // auto ser = map1.serialize();
    // map1.print_map();
    // std::cout << std::endl;
    // map2.deserialize(ser);
    // map2.print_map();

    // constexpr int k_min_word_len = 6;
    // constexpr int k_max_word_len = 10;
    // constexpr int k_num_of_words = 10;

    // srand(time(NULL));
    // auto corpus = std::vector<std::string>();
    // if(argc == 4) {
    //     char *path = argv[3];
    //     int order = std::stoi(std::string(argv[1]));
    //     int gain = std::stoi(std::string(argv[2]));
    //     corpus = Parser::parse_file(path);
    //     auto gen = Generator(order, gain, corpus);
    //     for(int i = 0; i < k_num_of_words;) {
    //         auto tmp = gen.generate_word();
    //         if(tmp.length() >= k_min_word_len && tmp.length() <= k_max_word_len) {
    //             std::cout << tmp << std::endl;
    //             ++i;
    //         }
    //     }
    // } else {
    //     std::cout << "Wrong arguments!" << std::endl;
    // }
}
