#include <optional>
#include <vector>
#include <filesystem>
#include "mcwg.h"

const std::filesystem::path k_animals_path = "../res/animals";
const std::filesystem::path k_adjectives_path = "../res/adjectives";
const std::filesystem::path k_corpus_name = "corpus.txt";

std::vector<Model> generate_models (const int order,
                                    const int gain,
                                    const std::filesystem::path &path,
                                    const std::vector<std::string> &corpus) {
    auto ret = std::vector<Model>();
    for(int i = order; i > 0; --i) {
        auto res_path = path / std::format("model{}.bin", i);
        auto model = Model(i, gain);
        auto tmp = Parser::load_model(res_path);
        if(!tmp) {
            std::cout << "Load: " << res_path << std::endl;
            model.deserialize(tmp.value());
        } else {
            std::cout << "Save: " << res_path << std::endl;
            model.generate_model(corpus);
            Parser::save_model(res_path, model.serialize());
        }
        ret.push_back(model);
    }
    return ret;
}

int main (int argc, char *argv[]) {
    constexpr int k_min_word_len = 6;
    constexpr int k_max_word_len = 12;
    constexpr int k_num_of_words = 10;
    srand(time(NULL));

    if(argc == 3) {
        int order = std::stoi(std::string(argv[1]));
        int gain = std::stoi(std::string(argv[2]));
        try {
            auto corpus = Parser::parse_file(k_adjectives_path / k_corpus_name);
            if(corpus) {
                auto adj_models = generate_models(order, gain, k_adjectives_path, corpus.value());
                corpus = Parser::parse_file(k_animals_path / k_corpus_name);
                if(corpus) {
                    auto anm_models = generate_models(order, gain, k_animals_path, corpus.value());
                    auto adj_gen = Generator(order, adj_models);
                    auto anm_gen = Generator(order, anm_models);
                    std::cout << adj_gen.get_printable();
                    for(int i = 0; i < k_num_of_words; ++i) {
                        auto adj_word = adj_gen.generate_word(k_min_word_len, k_max_word_len);
                        auto anm_word = anm_gen.generate_word(k_min_word_len, k_max_word_len);
                        std::cout << adj_word << " " << anm_word << std::endl;
                    }
                } else {
                    std::cout << std::format("Can not open {} file\n", (k_adjectives_path / k_corpus_name).c_str());
                }
            } else {
                std::cout << std::format("Can not open {} file\n", (k_adjectives_path / k_corpus_name).c_str());
            }

        } catch(const std::invalid_argument &e) {
            std::cout << e.what() << std::endl;
        } catch(const std::exception &e) {
            std::cout << e.what() << std::endl;
        }
    } else {
        std::cout << "Wrong arguments!" << std::endl;
    }
}
