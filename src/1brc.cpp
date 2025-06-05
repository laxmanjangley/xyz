#include <iostream>
#include <unordered_map>
#include <vector>
#include <string_view>
#include <algorithm>
#include <limits>
#include <iomanip>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>

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

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    struct stat st;
    if (fstat(fd, &st) != 0) {
        perror("fstat");
        close(fd);
        return 1;
    }
    size_t size = st.st_size;
    char* data = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    close(fd);

    std::unordered_map<std::string_view, Stats> stats;
    const char* ptr = data;
    const char* end = data + size;

    while (ptr < end) {
        const char* name_start = ptr;
        while (ptr < end && *ptr != ';') ++ptr;
        if (ptr >= end) break;
        std::string_view name(name_start, ptr - name_start);
        ++ptr; // skip ';'
        char* num_end;
        double temp = strtod(ptr, &num_end);
        ptr = num_end;
        while (ptr < end && (*ptr == '\n' || *ptr == '\r')) ++ptr;

        Stats& s = stats[name];
        s.count++;
        s.sum += temp;
        if (temp < s.min) s.min = temp;
        if (temp > s.max) s.max = temp;
    }

    std::vector<std::string_view> names;
    names.reserve(stats.size());
    for (auto const& kv : stats) names.push_back(kv.first);
    std::sort(names.begin(), names.end());

    std::cout.setf(std::ios::fixed);
    std::cout << std::setprecision(1);

    bool first = true;
    for (auto const& name : names) {
        const Stats& s = stats[name];
        if (!first) std::cout << ", ";
        first = false;
        std::cout << name << "=" << s.min << "/" << (s.sum / s.count) << "/" << s.max;
    }
    std::cout << std::endl;
    munmap(data, size);
    return 0;
}
