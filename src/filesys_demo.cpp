#include "filesys.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

void print_help() {
    std::cout << "Commands:\n"
              << "  mkdir <path>\n"
              << "  touch <path>\n"
              << "  ls [path]\n"
              << "  cd <path>\n"
              << "  pwd\n"
              << "  rm <path>        (remove file)\n"
              << "  rmdir <path>     (remove empty dir)\n"
              << "  write <path> <text>  (overwrite file contents)\n"
              << "  cat <path>\n"
              << "  tree [path]\n"
              << "  help\n"
              << "  exit\n";
}

int main() {
    FileSystem fs;
    std::string line;
    print_help();
    while (true) {
        std::cout << fs.pwd() << " $ ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        if (cmd == "exit") break;
        else if (cmd == "help") { print_help(); continue; }
        else if (cmd == "mkdir") {
            std::string path; iss >> path;
            if (path.empty()) { std::cout << "mkdir: missing path\n"; continue; }
            if (!fs.mkdir(path)) std::cout << "mkdir: failed (exists or invalid path)\n";
        }
        else if (cmd == "touch") {
            std::string path; iss >> path;
            if (path.empty()) { std::cout << "touch: missing path\n"; continue; }
            if (!fs.touch(path)) std::cout << "touch: failed\n";
        }
        else if (cmd == "ls") {
            std::string path; iss >> path;
            auto list = fs.ls(path);
            for (auto &n : list) std::cout << n << "  ";
            std::cout << "\n";
        }
        else if (cmd == "cd") {
            std::string path; iss >> path;
            if (!fs.cd(path)) std::cout << "cd: no such dir\n";
        }
        else if (cmd == "pwd") {
            std::cout << fs.pwd() << "\n";
        }
        else if (cmd == "rm") {
            std::string path; iss >> path;
            if (!fs.remove_file(path)) std::cout << "rm: failed\n";
        }
        else if (cmd == "rmdir") {
            std::string path; iss >> path;
            if (!fs.remove_dir(path)) std::cout << "rmdir: failed (not empty or not exist)\n";
        }
        else if (cmd == "write") {
            std::string path; iss >> path;
            std::string rest;
            std::getline(iss, rest);
            if (path.empty()) { std::cout << "write: missing path\n"; continue; }
            // rest may start with space
            if (!rest.empty() && rest[0] == ' ') rest.erase(rest.begin());
            if (!fs.write_file(path, rest)) std::cout << "write: failed\n";
        }
        else if (cmd == "cat") {
            std::string path; iss >> path;
            std::string out;
            if (path.empty()) { std::cout << "cat: missing path\n"; continue; }
            if (!fs.cat(path, out)) { std::cout << "cat: failed\n"; continue; }
            std::cout << out << "\n";
        }
        else if (cmd == "tree") {
            std::string path; iss >> path;
            fs.tree(path);
        }
        else {
            std::cout << "unknown command: " << cmd << "\n";
        }
    }
    return 0;
}
