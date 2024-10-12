#include <vector>
#include "mcwg.h"

static constexpr char k_animals_path[] = "../res/animals";
static constexpr char k_adjectives_path[] = "../res/adjectives";
static constexpr char k_other_path[] = "../res/other";

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

std::vector<Model> generate_models (const int order,
                                    const int gain,
                                    const std::string &path,
                                    const std::vector<std::string> &corpus) {
    auto ret = std::vector<Model>();
    for(int i = order; i > 0; --i) {
        std::string res_path = path + "/" + "model" + std::to_string(i) + ".bin";
        auto model = Model(i, gain);
        auto tmp = Parser::load_model(res_path);
        if(!tmp.empty()) {
            std::cout << "Load: " << res_path << std::endl;
            model.deserialize(tmp);
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
            auto adj_models = generate_models(order,
                                              gain,
                                              k_adjectives_path,
                                              Parser::parse_file(std::string(k_adjectives_path) + "/corpus.txt"));
            auto anm_models = generate_models(order,
                                              gain,
                                              k_animals_path,
                                              Parser::parse_file(std::string(k_animals_path) + "/corpus.txt"));
            auto adj_gen = Generator(order, adj_models);
            auto anm_gen = Generator(order, anm_models);

            for(int i = 0; i < k_num_of_words; ++i) {
                auto adj_word = adj_gen.generate_word(k_min_word_len, k_max_word_len);
                auto anm_word = anm_gen.generate_word(k_min_word_len, k_max_word_len);
                std::cout << adj_word << " " << anm_word << std::endl;
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
