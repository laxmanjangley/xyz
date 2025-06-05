#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <iomanip>

struct Stats {
    long count = 0;
    double sum = 0.0;
    double min = std::numeric_limits<double>::max();
    double max = -std::numeric_limits<double>::max();
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: 1brc <input-file>" << std::endl;
        return 1;
    }
    std::ifstream in(argv[1]);
    if (!in) {
        std::cerr << "failed to open input file" << std::endl;
        return 1;
    }
    std::unordered_map<std::string, Stats> data;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::size_t pos = line.find(';');
        if (pos == std::string::npos) continue;
        std::string name = line.substr(0, pos);
        double temp = std::stod(line.substr(pos + 1));
        Stats &s = data[name];
        s.count++;
        s.sum += temp;
        if (temp < s.min) s.min = temp;
        if (temp > s.max) s.max = temp;
    }
    std::vector<std::string> names;
    names.reserve(data.size());
    for (auto const& kv : data) names.push_back(kv.first);
    std::sort(names.begin(), names.end());

    std::cout.setf(std::ios::fixed);
    std::cout << std::setprecision(1);

    bool first = true;
    for (auto const& name : names) {
        const Stats& s = data[name];
        if (!first) std::cout << ", ";
        first = false;
        std::cout << name << "=" << s.min << "/" << (s.sum / s.count) << "/" << s.max;
    }
    std::cout << std::endl;
    return 0;
}
