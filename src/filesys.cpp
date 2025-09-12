#include "filesys.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <cctype>

// ---------------- FSNode ----------------
FSNode::FSNode(const std::string &name_, NodeType t, FSNode* parent_)
    : name(name_), type(t), parent(parent_),
      content(""),
      permissions(0644),
      ctime(0), mtime(0), atime(0),
      owner("user") {}

// ---------------- FileSystem ----------------
FileSystem::FileSystem() {
    root = std::make_unique<FSNode>("/", NodeType::DIR_NODE, nullptr);
    cwd = root.get();
    global_clock = 0;

    // initialize root metadata
    root->permissions = 0755;
    root->owner = "root";
    root->ctime = root->mtime = root->atime = ++global_clock;
}

/* ---------------- change directory ---------------- */
bool FileSystem::cd(const std::string &path) {
    FSNode* node = resolve_path(path);
    if (!node || !node->is_dir()) {
        return false;
    }
    cwd = node;
    cwd->atime = ++global_clock; // update access time
    return true;
}

/* ---------------- print working directory ---------------- */
std::string FileSystem::pwd() const {
    std::vector<std::string> parts;
    const FSNode* node = cwd;
    while (node && node != root.get()) {
        parts.push_back(node->name);
        node = node->parent;
    }
    std::string path = "/";
    for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
        path += *it;
        if (std::next(it) != parts.rend()) path += "/";
    }
    return path;
}

/* ---------- Utility: format permissions (e.g. rwxr-xr-x) --------- */
std::string FileSystem::perms_to_string(int mode) {
    std::string s;
    for (int shift = 6; shift >= 0; shift -= 3) {
        int val = (mode >> shift) & 7;
        s.push_back((val & 4) ? 'r' : '-');
        s.push_back((val & 2) ? 'w' : '-');
        s.push_back((val & 1) ? 'x' : '-');
    }
    return s;
}

/* ---------------- path utilities ---------------- */
std::vector<std::string> FileSystem::split_path(const std::string &path) const {
    std::vector<std::string> parts;
    std::string cur;
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

/* ---------------- operations (update metadata) ---------------- */

bool FileSystem::mkdir(const std::string &path) {
    std::string name;
    FSNode* parent = resolve_parent_of(path, name);
    if (!parent || !parent->is_dir()) return false;
    if (name.empty()) return false;
    if (parent->children.find(name) != parent->children.end()) return false; // exists

    auto node = std::make_unique<FSNode>(name, NodeType::DIR_NODE, parent);
    node->permissions = 0755;
    node->owner = "user";
    node->ctime = node->mtime = node->atime = ++global_clock;

    parent->children[name] = std::move(node);
    parent->mtime = ++global_clock;
    return true;
}

bool FileSystem::touch(const std::string &path) {
    std::string name;
    FSNode* parent = resolve_parent_of(path, name);
    if (!parent || !parent->is_dir()) return false;
    if (name.empty()) return false;
    auto it = parent->children.find(name);
    if (it != parent->children.end()) {
        // update mtime if it's a file
        if (!it->second->is_file()) return false;
        it->second->mtime = ++global_clock;
        return true;
    }
    auto node = std::make_unique<FSNode>(name, NodeType::FILE_NODE, parent);
    node->permissions = 0644;
    node->owner = "user";
    node->ctime = node->mtime = node->atime = ++global_clock;
    parent->children[name] = std::move(node);
    parent->mtime = ++global_clock;
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
    parent->mtime = ++global_clock;
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
    parent->mtime = ++global_clock;
    return true;
}

bool FileSystem::write_file(const std::string &path, const std::string &text) {
    std::string name;
    FSNode* parent = resolve_parent_of(path, name);
    if (!parent) return false;
    auto it = parent->children.find(name);
    FSNode* node = nullptr;
    if (it == parent->children.end()) {
        auto newNode = std::make_unique<FSNode>(name, NodeType::FILE_NODE, parent);
        newNode->permissions = 0644;
        newNode->owner = "user";
        newNode->ctime = newNode->mtime = newNode->atime = ++global_clock;
        newNode->content = text;
        node = newNode.get();
        parent->children[name] = std::move(newNode);
    } else {
        if (!it->second->is_file()) return false;
        node = it->second.get();
        node->content = text;
        node->mtime = ++global_clock;
    }
    parent->mtime = ++global_clock;
    return true;
}

bool FileSystem::cat(const std::string &path, std::string &out) const {
    FSNode* node = resolve_path(path);
    if (!node || !node->is_file()) return false;
    node->atime = ++global_clock; // mutable global_clock allows this
    out = node->content;
    return true;
}

std::vector<std::string> FileSystem::ls(const std::string &path) const {
    std::vector<std::string> res;
    FSNode* node = resolve_path(path);
    if (!node) return res;

    // update access time for the directory being listed
    if (node->is_dir()) node->atime = ++global_clock;

    auto format_entry = [&](const FSNode* n) {
        std::ostringstream oss;
        oss << (n->is_dir() ? "d" : "-")
            << perms_to_string(n->permissions) << " "
            << n->owner << " "
            << "c:" << n->ctime << " m:" << n->mtime << " a:" << n->atime << " "
            << n->name;
        return oss.str();
    };

    if (node->is_file()) {
        res.push_back(format_entry(node));
        return res;
    }
    for (const auto &kv : node->children) {
        res.push_back(format_entry(kv.second.get()));
    }
    return res;
}

/* tree (show permissions + owner + times) */
void FileSystem::tree_recursive(const FSNode* node, const std::string &prefix, bool isLast) const {
    std::cout << prefix << (isLast ? "└── " : "├── ");
    std::cout << node->name << (node->is_dir() ? "/" : "")
              << " [" << (node->is_dir() ? "d" : "-") << perms_to_string(node->permissions)
              << " o:" << node->owner
              << " c:" << node->ctime << " m:" << node->mtime << " a:" << node->atime << "]\n";

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
    oss << "{";
    oss << "\"type\":\"" << (node->is_dir() ? "dir" : "file") << "\",";
    oss << "\"name\":\"" << FileSystem::escape_json_string(node->name) << "\",";
    oss << "\"owner\":\"" << FileSystem::escape_json_string(node->owner) << "\",";
    oss << "\"permissions\":" << node->permissions << ",";
    oss << "\"ctime\":" << node->ctime << ",";
    oss << "\"mtime\":" << node->mtime << ",";
    oss << "\"atime\":" << node->atime;
    if (node->is_dir()) {
        oss << ",\"children\":{";
        bool first = true;
        for (const auto &kv : node->children) {
            if (!first) oss << ",";
            first = false;
            oss << "\"" << FileSystem::escape_json_string(kv.first) << "\":";
            oss << serialize_node_json(kv.second.get());
        }
        oss << "}";
    } else {
        oss << ",\"content\":\"" << FileSystem::escape_json_string(node->content) << "\"";
    }
    oss << "}";
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

static int parse_json_int(const std::string &s, size_t &i) {
    FileSystem::skip_ws(s, i);
    bool neg = false;
    if (i < s.size() && s[i] == '-') { neg = true; ++i; }
    if (i >= s.size() || !std::isdigit((unsigned char)s[i])) throw std::runtime_error("Expected number");
    long long val = 0;
    while (i < s.size() && std::isdigit((unsigned char)s[i])) {
        val = val * 10 + (s[i++] - '0');
    }
    return neg ? -static_cast<int>(val) : static_cast<int>(val);
}

// parse a node object (directory or file)
std::unique_ptr<FSNode> FileSystem::parse_node_from_json(const std::string &json, size_t &idx) {
    skip_ws(json, idx);
    if (idx >= json.size() || json[idx] != '{') throw std::runtime_error("Expected {");
    ++idx;

    std::string type_str;
    std::string name_str;
    std::string content_str;
    std::string owner_str;
    int perms = 0644;
    int ctime = 0, mtime = 0, atime = 0;

    std::vector<std::pair<std::string, std::unique_ptr<FSNode>>> tmp_children;

    while (true) {
        skip_ws(json, idx);
        if (idx >= json.size()) throw std::runtime_error("Unexpected end in object");
        if (json[idx] == '}') { ++idx; break; }

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
        } else if (key == "owner") {
            owner_str = parse_json_string(json, idx);
        } else if (key == "permissions") {
            perms = parse_json_int(json, idx);
        } else if (key == "ctime") {
            ctime = parse_json_int(json, idx);
        } else if (key == "mtime") {
            mtime = parse_json_int(json, idx);
        } else if (key == "atime") {
            atime = parse_json_int(json, idx);
        } else if (key == "children") {
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
                std::unique_ptr<FSNode> childNode = parse_node_from_json(json, idx);
                tmp_children.emplace_back(childName, std::move(childNode));
                skip_ws(json, idx);
                if (idx < json.size() && json[idx] == ',') { ++idx; continue; }
            }
        } else {
            // Skip unknown value (string/object/number)
            skip_ws(json, idx);
            if (idx < json.size() && json[idx] == '"') {
                std::string dummy = parse_json_string(json, idx);
                (void)dummy;
            } else if (idx < json.size() && json[idx] == '{') {
                int braces = 0;
                do {
                    if (json[idx] == '{') ++braces;
                    else if (json[idx] == '}') --braces;
                    ++idx;
                } while (idx < json.size() && braces > 0);
            } else {
                // number or token
                while (idx < json.size() && json[idx] != ',' && json[idx] != '}') ++idx;
            }
        }
        skip_ws(json, idx);
        if (idx < json.size() && json[idx] == ',') { ++idx; continue; }
    }

    if (type_str.empty()) throw std::runtime_error("Node missing type");
    if (name_str.empty()) {
        name_str = "/";
    }

    std::unique_ptr<FSNode> node;
    if (type_str == "dir") {
        node = std::make_unique<FSNode>(name_str, NodeType::DIR_NODE, nullptr);
        node->owner = owner_str.empty() ? "user" : owner_str;
        node->permissions = perms;
        node->ctime = ctime;
        node->mtime = mtime;
        node->atime = atime;
        for (auto &p : tmp_children) {
            p.second->parent = node.get();
            node->children.emplace(p.first, std::move(p.second));
        }
    } else {
        node = std::make_unique<FSNode>(name_str, NodeType::FILE_NODE, nullptr);
        node->content = content_str;
        node->owner = owner_str.empty() ? "user" : owner_str;
        node->permissions = perms;
        node->ctime = ctime;
        node->mtime = mtime;
        node->atime = atime;
    }
    return node;
}

/* helper to compute max timestamp in subtree */
static int compute_max_time(const FSNode* node) {
    int maxv = node->ctime;
    maxv = std::max(maxv, node->mtime);
    maxv = std::max(maxv, node->atime);
    for (const auto &kv : node->children) {
        int childmax = compute_max_time(kv.second.get());
        if (childmax > maxv) maxv = childmax;
    }
    return maxv;
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
        // set root and compute clock
        int maxTime = compute_max_time(parsed.get());
        root = std::move(parsed);
        cwd = root.get();
        global_clock = std::max(global_clock, maxTime);
        return true;
    } catch (...) {
        return false;
    }
}
