#include "pch.hpp"

struct File {
    std::string name;
    usize       size;

    File(std::string name, usize size) : name{std::move(name)}, size{size} {}

    usize Size() const { return size; }
};

struct Directory {
    Directory*                       parent = nullptr;
    std::string                      name;
    std::map<std::string, Directory> directories;
    std::map<std::string, File>      files;
    usize                            size{};

    Directory(Directory* parent, std::string name) : parent{parent}, name{std::move(name)} {}

    void UpdateSize() {
        ranges::for_each(directories | views::values, &Directory::UpdateSize);
        size = ranges::accumulate(directories | views::values, 0_sz, std::plus{}, &Directory::Size) +
               ranges::accumulate(files | views::values, 0_sz, std::plus{}, &File::Size);
    }

    usize Size() const { return size; }

    usize Part1() const {
        return ranges::accumulate(directories | views::values, 0_sz, std::plus{}, &Directory::Part1) +
               (size < 100000_sz ? size : 0);
    }

    const Directory* SmallestMoreThan(usize min) const {
        if (size < min) {
            return nullptr;
        }
        const Directory* result   = this;
        usize            smallest = size;
        for (const auto& [name, sub_dir] : directories) {
            if (auto* small_dir = sub_dir.SmallestMoreThan(min); small_dir && small_dir->Size() < smallest) {
                result   = small_dir;
                smallest = result->Size();
            }
        }
        return result;
    }
};

int main() {
    std::ifstream input_file{"input.txt", std::ios::binary};
    std::string   raw_input{std::istreambuf_iterator{input_file}, {}};
    auto          input = raw_input | views::split('\n') | ranges::to<std::vector<std::string>>;

    Directory  root{nullptr, ""};
    Directory* cwd = &root;

    for (const auto& line : input) {
        if (line.front() == '$' && line[2] == 'c') {
            std::string dir_name;
            scn::scan(line, "$ cd {}", dir_name);
            if (dir_name == "/"sv) {
                cwd = &root;
            } else if (dir_name == ".."sv) {
                cwd = cwd->parent;
                assert(cwd);
            } else {
                auto [it, inserted] = cwd->directories.try_emplace(dir_name, cwd, dir_name);
                cwd                 = &it->second;
            }
        } else if (std::isdigit(line.front())) {
            usize       file_size{};
            std::string file_name;
            scn::scan(line, "{} {}", file_size, file_name);
            cwd->files.try_emplace(file_name, file_name, file_size);
        } else {
            std::string dir_name;
            scn::scan(line, "dir {}", dir_name);
            auto [it, inserted] = cwd->directories.try_emplace(dir_name, cwd, dir_name);
        }
    }

    root.UpdateSize();

    fmt::print("Total Size:  {}\n", root.Size());
    fmt::print("Part 1:  {}\n", root.Part1());
    fmt::print("Part 2:  {}\n", root.SmallestMoreThan(root.Size() - (70000000_sz - 30000000_sz))->Size());
}
