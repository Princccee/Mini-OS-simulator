#include "filesys.h"
#include <sstream>
#include <iostream>
#include <algorithm>

FSNode::FSNode(const std::string &name_, NodeType t, FSNode* parent_)
    : name(name_), type(t), parent(parent_) {}

FileSystem::FileSystem() {
    root = std::make_unique<FSNode>("/", NodeType::DIR_NODE, nullptr);
    cwd = root.get();
}

std::vector<std::string> FileSystem::split_path(const std::string &path) const {
    std::vector<std::string> parts;
    std::string token;
    std::istringstream iss(path);
    // split by '/'
    size_t i = 0;
    std::string p = path;
    // remove leading/trailing spaces
    while (!p.empty() && p.front()==' ') p.erase(p.begin());
    while (!p.empty() && p.back()==' ') p.pop_back();
    if (p.empty()) return parts;
    // allow repeated slashes: use manual parse
    std::string cur;
    for (char c : p) {
        if (c == '/') {
            if (!cur.empty()) { parts.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) parts.push_back(cur);
    return parts;
}

FSNode* FileSystem::resolve_path(const std::string &path) const {
    if (path.empty()) return cwd;
    FSNode* node = nullptr;
    std::vector<std::string> parts = split_path(path);
    if (path.size() > 0 && path[0] == '/') {
        node = root.get();
    } else {
        node = cwd;
    }
    for (const auto &part : parts) {
        if (part == ".") continue;
        if (part == "..") {
            if (node->parent) node = node->parent;
            continue;
        }
        auto it = node->children.find(part);
        if (it == node->children.end()) return nullptr;
        node = it->second.get();
    }
    return node;
}

FSNode* FileSystem::resolve_parent_of(const std::string &path, std::string &basename) const {
    // returns parent node pointer and sets basename (last component)
    basename.clear();
    if (path.empty()) return nullptr;
    // handle trailing slashes: remove them
    std::string p = path;
    while (!p.empty() && p.back() == '/') p.pop_back();
    if (p.empty()) return root.get();
    // find last '/'
    size_t pos = p.find_last_of('/');
    if (pos == std::string::npos) {
        basename = p;
        return cwd;
    } else if (pos == 0) {
        // like "/name"
        basename = p.substr(1);
        return root.get();
    } else {
        basename = p.substr(pos + 1);
        std::string parent_path = p.substr(0, pos);
        return resolve_path(parent_path);
    }
}

bool FileSystem::mkdir(const std::string &path) {
    std::string name;
    FSNode* parent = resolve_parent_of(path, name);
    if (!parent || !parent->is_dir()) return false;
    if (name.empty()) return false;
    if (parent->children.find(name) != parent->children.end()) return false; // exists
    parent->children[name] = std::make_unique<FSNode>(name, NodeType::DIR_NODE, parent);
    return true;
}

bool FileSystem::touch(const std::string &path) {
    std::string name;
    FSNode* parent = resolve_parent_of(path, name);
    if (!parent || !parent->is_dir()) return false;
    if (name.empty()) return false;
    if (parent->children.find(name) != parent->children.end()) {
        // if exists and is file -> update timestamp (ignored here)
        return parent->children[name]->is_file();
    }
    parent->children[name] = std::make_unique<FSNode>(name, NodeType::FILE_NODE, parent);
    return true;
}

bool FileSystem::remove_file(const std::string &path) {
    std::string name;
    FSNode* parent = resolve_parent_of(path, name);
    if (!parent) return false;
    auto it = parent->children.find(name);
    if (it == parent->children.end()) return false;
    if (!it->second->is_file()) return false;
    parent->children.erase(it);
    return true;
}

bool FileSystem::remove_dir(const std::string &path) {
    std::string name;
    FSNode* parent = resolve_parent_of(path, name);
    if (!parent) return false;
    auto it = parent->children.find(name);
    if (it == parent->children.end()) return false;
    if (!it->second->is_dir()) return false;
    if (!it->second->children.empty()) return false; // not empty
    parent->children.erase(it);
    return true;
}

bool FileSystem::write_file(const std::string &path, const std::string &text) {
    std::string name;
    FSNode* parent = resolve_parent_of(path, name);
    if (!parent) return false;
    auto it = parent->children.find(name);
    if (it == parent->children.end()) {
        // create file
        parent->children[name] = std::make_unique<FSNode>(name, NodeType::FILE_NODE, parent);
    } else if (!it->second->is_file()) {
        return false; // path points to directory
    }
    parent->children[name]->content = text;
    return true;
}

bool FileSystem::cat(const std::string &path, std::string &out) const {
    FSNode* node = resolve_path(path);
    if (!node || !node->is_file()) return false;
    out = node->content;
    return true;
}

std::vector<std::string> FileSystem::ls(const std::string &path) const {
    std::vector<std::string> res;
    FSNode* node = resolve_path(path);
    if (!node) return res;
    if (node->is_file()) {
        res.push_back(node->name);
        return res;
    }
    for (const auto &kv : node->children) res.push_back(kv.first);
    return res;
}

bool FileSystem::cd(const std::string &path) {
    if (path.empty()) {
        cwd = root.get();
        return true;
    }
    FSNode* node = resolve_path(path);
    if (!node || !node->is_dir()) return false;
    cwd = node;
    return true;
}

std::string FileSystem::pwd() const {
    // build path by walking to root
    std::vector<std::string> parts;
    const FSNode* cur = cwd;
    while (cur && cur->parent) {
        parts.push_back(cur->name);
        cur = cur->parent;
    }
    std::string out = "/";
    for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
        out += *it;
        if (it + 1 != parts.rend()) out += "/";
    }
    return out;
}

void FileSystem::tree_recursive(const FSNode* node, const std::string &prefix, bool isLast) const {
    std::cout << prefix;
    std::cout << (isLast ? "└── " : "├── ");
    std::cout << node->name;
    if (node->is_dir()) std::cout << "/\n";
    else std::cout << "\n";

    std::string newPrefix = prefix + (isLast ? "    " : "│   ");
    size_t cnt = 0;
    for (auto it = node->children.begin(); it != node->children.end(); ++it, ++cnt) {
        bool last = (cnt + 1 == node->children.size());
        tree_recursive(it->second.get(), newPrefix, last);
    }
}

void FileSystem::tree(const std::string &path) const {
    const FSNode* node = nullptr;
    if (path.empty()) node = cwd;
    else node = resolve_path(path);
    if (!node) {
        std::cout << "Path not found\n";
        return;
    }
    // If root, print "/" name specially
    if (node == root.get()) {
        std::cout << "/\n";
    } else {
        std::cout << node->name << (node->is_dir() ? "/\n" : "\n");
    }
    size_t cnt = 0;
    for (auto it = node->children.begin(); it != node->children.end(); ++it, ++cnt) {
        bool last = (cnt + 1 == node->children.size());
        tree_recursive(it->second.get(), "", last);
    }
}
