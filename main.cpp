#include <optional>
#include <vector>
#include <filesystem>
#include "mcwg.h"

const std::filesystem::path k_animals_path = "../res/animals";
const std::filesystem::path k_adjectives_path = "../res/adjectives";
const std::filesystem::path k_corpus_name = "corpus.txt";

class Parser
{
   public:
    static std::optional<std::vector<std::string>> parse_file (const std::filesystem::path &path) {
        auto ret = std::optional<std::vector<std::string>>();
        auto str = std::vector<std::string>();
        std::ifstream file_stream(path, std::ios_base::in);
        if(file_stream) {
            for(std::string line; getline(file_stream, line);) {
                str.push_back(line);
            }
            ret = str;
        }
        return ret;
    }

    static std::optional<std::vector<uint8_t>> load_model (const std::filesystem::path &path) {
        std::ifstream file_stream(path, std::ios::binary);
        auto vec = std::vector<uint8_t>();
        auto ret = std::optional<std::vector<uint8_t>>();
        if(file_stream) {
            vec = std::vector<uint8_t>((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
            ret = vec;
        }
        return ret;
    }

    static bool save_model (const std::filesystem::path &path, const std::vector<uint8_t> &vec) {
        std::ofstream file_stream(path, std::ios_base::binary | std::ios::app);
        bool ret = false;
        if(file_stream) {
            file_stream.write((char *) &vec[0], vec.size() * sizeof(uint8_t));
            ret = true;
        }
        return ret;
    }

    static void print_corpus (const std::vector<std::string> &corpus) {
        for(auto const &n : corpus) {
            std::cout << n << std::endl;
        }
    }
};

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
