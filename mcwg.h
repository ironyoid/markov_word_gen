#ifndef __MCWG_H_
#define __MCWG_H_

#include "map"
#include "vector"
#include <cassert>
#include <cstdint>
#include <ctime>
#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <format>

class McwgBase
{
   public:
    McwgBase () = default;

   protected:
    static constexpr char k_alphabet[] = "@abcdefghijklmnopqrstuvwxyz";
    static constexpr char k_end_mark[] = "@";
    static constexpr char k_start_mark[] = "#";
    void push_int (std::vector<uint8_t> &vec, uint32_t val) {
        vec.push_back(static_cast<uint8_t>(val));
        vec.push_back(static_cast<uint8_t>(val >> 8));
        vec.push_back(static_cast<uint8_t>(val >> 16));
        vec.push_back(static_cast<uint8_t>(val >> 24));
    }

    uint32_t pull_int (const std::vector<uint8_t> &vec, uint32_t pos) {
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

    std::string pull_string (const std::vector<uint8_t> &vec, const int pos, const int len) {
        auto ret = std::string();
        for(int i = pos; i < pos + len; ++i) {
            ret += vec[i];
        }
        return ret;
    }

    std::string gen_start_seq (const int order) {
        auto ret = std::string("");
        for(int i = 0; i < order; ++i) {
            ret += k_start_mark;
        }
        return ret;
    }
};

class AlphabetMap : McwgBase
{
    std::map<char, uint32_t> _map;

   public:
    uint32_t &operator[](const char key) {
        return _map[key];
    }
    AlphabetMap () {
        _map = std::map<char, uint32_t>();
        for(const auto &c : McwgBase::k_alphabet) {
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

    std::string get_printable () const {
        auto ret = std::string();
        for(const auto &[key, val] : _map) {
            ret += std::format("{}: {} ", key, val);
        }
        return ret;
    }

    void deserialize (const std::vector<uint8_t> &vec, int pos, int len) {
        assert(vec.size() != 0 && "vec must contain at least one element");
        int letter_cnt = 0;
        for(int i = pos; i < pos + len; i += 4) {
            auto tmp = pull_int(vec, i);
            _map[McwgBase::k_alphabet[letter_cnt]] = tmp;
            ++letter_cnt;
        }
    }

    std::vector<uint8_t> serialize () {
        auto ret = std::vector<uint8_t>();
        push_int(ret, _map.size() * sizeof(int));
        for(const auto n : _map) {
            push_int(ret, n.second);
        }
        return ret;
    }
};

class Model : McwgBase
{
    std::map<std::string, AlphabetMap> _chain;
    int _order;
    int _gain;

   public:
    Model (const int order, const int gain) : _chain(), _order(order), _gain(gain) {
    }

    void generate_model (const std::vector<std::string> &corpus) {
        for(const auto &n : corpus) {
            std::string tmp_str = gen_start_seq(_order) + n + McwgBase::k_end_mark;
            for(int i = 0; i < static_cast<int>(tmp_str.length()) - _order; ++i) {
                auto tmp = tmp_str.substr(i, _order);
                if(_chain.find(tmp) == _chain.end()) {
                    _chain[tmp] = AlphabetMap();
                }
                _chain[tmp][tmp_str[i + _order]] += _gain;
            }
        }
    }

    char get_letter (const std::string &context) {
        char ret = 0;
        if(_chain.find(context) != _chain.end()) {
            auto tmp = _chain[context].get_vector();
            auto rnd = rand() % tmp.back();
            for(int i = 0; i < static_cast<int>(tmp.size()); ++i) {
                if(rnd <= tmp[i]) {
                    ret = McwgBase::k_alphabet[i];
                    break;
                }
            }
        }
        return ret;
    }

    std::string get_printable () const {
        auto ret = std::string();
        for(const auto &[key, val] : _chain) {
            ret += std::format("{}: [{}]\n", key, val.get_printable());
        }
        return ret;
    }

    std::vector<uint8_t> serialize () {
        auto ret = std::vector<uint8_t>();
        for(auto &n : _chain) {
            push_string(ret, n.first);
            auto tmp = n.second.serialize();
            ret.insert(ret.end(), tmp.begin(), tmp.end());
        }
        return ret;
    }
    void deserialize (const std::vector<uint8_t> &vec) {
        assert(vec.size() != 0 && "vec must contain at least one element");
        long unsigned int pos = 0;
        while(pos < vec.size()) {
            auto str_len = pull_int(vec, pos);
            pos += 4;
            auto str = pull_string(vec, pos, str_len);
            pos += str_len;
            auto alp_len = pull_int(vec, pos);
            pos += 4;
            auto alp = AlphabetMap();
            alp.deserialize(vec, pos, alp_len);
            pos += alp_len;
            _chain[str] = alp;
        }
    }
};

class Generator : McwgBase
{
    std::vector<Model> _models;
    int _order;

   public:
    Generator (const int order, const std::vector<Model> &models) : _models(models), _order(order) {
    }

    char generate_letter (const std::string &context) {
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
        auto ret = gen_start_seq(_order);
        auto letter = generate_letter(ret);
        while(k_end_mark[0] != letter && 0 != letter) {
            if(0 != letter) {
                ret += letter;
            }
            letter = generate_letter(ret);
        }
        return ret.substr(_order, ret.length() - _order);
    }

    std::string generate_word (const int min, const int max) {
        while(true) {
            auto tmp = generate_word();
            if(tmp.length() >= min && tmp.length() <= max) {
                return tmp;
            }
        }
    }

    std::string get_printable () const {
        auto ret = std::string();
        for(const auto &n : _models) {
            ret += std::format("{}\n", n.get_printable());
        }
        return ret;
    }
};

#endif /* __MCWG_H_ */
