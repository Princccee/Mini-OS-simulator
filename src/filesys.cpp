#include "filesys.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <cctype>

FSNode::FSNode(const std::string &name_, NodeType t, FSNode* parent_)
    : name(name_), type(t), parent(parent_) {}

FileSystem::FileSystem() {
    root = std::make_unique<FSNode>("/", NodeType::DIR_NODE, nullptr);
    cwd = root.get();
}

std::vector<std::string> FileSystem::split_path(const std::string &path) const {
    std::vector<std::string> parts;
    std::string cur;
    // trim leading/trailing spaces
    size_t a = 0;
    while (a < path.size() && std::isspace((unsigned char)path[a])) ++a;
    size_t b = path.size();
    while (b > a && std::isspace((unsigned char)path[b-1])) --b;
    if (a >= b) return parts;
    for (size_t i = a; i < b; ++i) {
        char c = path[i];
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
    if (!path.empty() && path[0] == '/') node = root.get();
    else node = cwd;
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
    basename.clear();
    if (path.empty()) return nullptr;
    std::string p = path;
    // strip trailing slashes
    while (!p.empty() && p.back() == '/') p.pop_back();
    if (p.empty()) return root.get();
    size_t pos = p.find_last_of('/');
    if (pos == std::string::npos) {
        basename = p;
        return cwd;
    } else if (pos == 0) {
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
        parent->children[name] = std::make_unique<FSNode>(name, NodeType::FILE_NODE, parent);
        it = parent->children.find(name);
    } else if (!it->second->is_file()) {
        return false;
    }
    it->second->content = text;
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

/* ---------------- JSON persistence helpers ---------------- */

std::string FileSystem::escape_json_string(const std::string &s) {
    std::ostringstream oss;
    for (char ch : s) {
        switch (ch) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if ((unsigned char)ch < 0x20) {
                    // control char as \u00XX
                    oss << "\\u";
                    const char *hex = "0123456789abcdef";
                    oss << '0' << '0' << hex[((unsigned char)ch >> 4) & 0xF] << hex[((unsigned char)ch) & 0xF];
                } else {
                    oss << ch;
                }
        }
    }
    return oss.str();
}

static std::string serialize_node_json(const FSNode* node) {
    std::ostringstream oss;
    if (node->is_dir()) {
        oss << "{";
        oss << "\"type\":\"dir\",";
        oss << "\"name\":\"" << FileSystem::escape_json_string(node->name) << "\",";
        oss << "\"children\":{";
        bool first = true;
        for (const auto &kv : node->children) {
            if (!first) oss << ",";
            first = false;
            oss << "\"" << FileSystem::escape_json_string(kv.first) << "\":";
            oss << serialize_node_json(kv.second.get());
        }
        oss << "}";
        oss << "}";
    } else {
        oss << "{";
        oss << "\"type\":\"file\",";
        oss << "\"name\":\"" << FileSystem::escape_json_string(node->name) << "\",";
        oss << "\"content\":\"" << FileSystem::escape_json_string(node->content) << "\"";
        oss << "}";
    }
    return oss.str();
}

bool FileSystem::save_to_file(const std::string &filename) const {
    try {
        std::ofstream ofs(filename, std::ios::out | std::ios::trunc);
        if (!ofs.is_open()) return false;
        ofs << serialize_node_json(root.get());
        ofs.close();
        return true;
    } catch (...) {
        return false;
    }
}

/* ------------ Small JSON parser (compatible with serializer above) ------------ */

void FileSystem::skip_ws(const std::string &s, size_t &i) {
    while (i < s.size() && std::isspace((unsigned char)s[i])) ++i;
}

static std::string parse_json_string(const std::string &s, size_t &i) {
    FileSystem::skip_ws(s, i);
    if (i >= s.size() || s[i] != '"') throw std::runtime_error("Expected string quote");
    ++i;
    std::string out;
    while (i < s.size()) {
        char c = s[i++];
        if (c == '"') break;
        if (c == '\\') {
            if (i >= s.size()) throw std::runtime_error("Invalid escape");
            char e = s[i++];
            switch (e) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u':
                    // simple handling: skip \uXXXX (not decoding), put '?'
                    if (i + 4 <= s.size()) {
                        i += 4;
                        out.push_back('?');
                    } else {
                        throw std::runtime_error("Invalid \\u escape");
                    }
                    break;
                default:
                    out.push_back(e); break;
            }
        } else {
            out.push_back(c);
        }
    }
    return out;
}

// parse a node object (directory or file)
// This parser is intentionally limited and accepts the structure we produce.
std::unique_ptr<FSNode> FileSystem::parse_node_from_json(const std::string &json, size_t &idx) {
    skip_ws(json, idx);
    if (idx >= json.size() || json[idx] != '{') throw std::runtime_error("Expected {");
    ++idx;

    std::string type_str;
    std::string name_str;
    std::string content_str;
    // children parsed into temporary vector of pairs (name -> node)
    std::vector<std::pair<std::string, std::unique_ptr<FSNode>>> tmp_children;

    while (true) {
        skip_ws(json, idx);
        if (idx >= json.size()) throw std::runtime_error("Unexpected end in object");
        if (json[idx] == '}') { ++idx; break; }

        // parse key
        std::string key = parse_json_string(json, idx);
        skip_ws(json, idx);
        if (idx >= json.size() || json[idx] != ':') throw std::runtime_error("Expected :");
        ++idx;
        skip_ws(json, idx);

        if (key == "type") {
            type_str = parse_json_string(json, idx);
        } else if (key == "name") {
            name_str = parse_json_string(json, idx);
        } else if (key == "content") {
            content_str = parse_json_string(json, idx);
        } else if (key == "children") {
            // parse children object: { "childname": { ... }, "child2": { ... } }
            skip_ws(json, idx);
            if (idx >= json.size() || json[idx] != '{') throw std::runtime_error("Expected children object {");
            ++idx;
            skip_ws(json, idx);
            while (true) {
                skip_ws(json, idx);
                if (idx >= json.size()) throw std::runtime_error("Unexpected end in children");
                if (json[idx] == '}') { ++idx; break; }
                std::string childName = parse_json_string(json, idx);
                skip_ws(json, idx);
                if (idx >= json.size() || json[idx] != ':') throw std::runtime_error("Expected : after child key");
                ++idx;
                // parse child node recursively
                std::unique_ptr<FSNode> childNode = parse_node_from_json(json, idx);
                tmp_children.emplace_back(childName, std::move(childNode));
                skip_ws(json, idx);
                if (idx < json.size() && json[idx] == ',') { ++idx; continue; }
            }
        } else {
            // unknown key: try to skip a generic value (string/object)
            // We only expect the keys above in our serializer; for safety, skip a value
            if (idx < json.size() && json[idx] == '"') {
                std::string dummy = parse_json_string(json, idx);
                (void)dummy;
            } else if (idx < json.size() && json[idx] == '{') {
                // skip nested object crudely
                int braces = 0;
                do {
                    if (json[idx] == '{') ++braces;
                    else if (json[idx] == '}') --braces;
                    ++idx;
                } while (idx < json.size() && braces > 0);
            } else {
                // skip token until comma or }
                while (idx < json.size() && json[idx] != ',' && json[idx] != '}') ++idx;
            }
        }
        skip_ws(json, idx);
        if (idx < json.size() && json[idx] == ',') { ++idx; continue; }
    }

    // require type and name
    if (type_str.empty()) throw std::runtime_error("Node missing type");
    if (name_str.empty()) {
        // root may use "/" name; allow empty -> use "/"
        name_str = "/";
    }

    std::unique_ptr<FSNode> node;
    if (type_str == "dir") {
        node = std::make_unique<FSNode>(name_str, NodeType::DIR_NODE, nullptr);
        // attach children and set their parent pointer
        for (auto &p : tmp_children) {
            p.second->parent = node.get();
            node->children.emplace(p.first, std::move(p.second));
        }
    } else {
        node = std::make_unique<FSNode>(name_str, NodeType::FILE_NODE, nullptr);
        node->content = content_str;
    }
    return node;
}

bool FileSystem::load_from_file(const std::string &filename) {
    try {
        std::ifstream ifs(filename, std::ios::in);
        if (!ifs.is_open()) return false;
        std::ostringstream oss;
        oss << ifs.rdbuf();
        std::string json = oss.str();
        ifs.close();
        size_t idx = 0;
        std::unique_ptr<FSNode> parsed = parse_node_from_json(json, idx);
        if (!parsed || !parsed->is_dir()) return false;
        parsed->parent = nullptr;
        root = std::move(parsed);
        cwd = root.get();
        return true;
    } catch (...) {
        return false;
    }
}
