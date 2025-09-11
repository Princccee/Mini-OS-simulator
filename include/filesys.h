#ifndef FILESYS_H
#define FILESYS_H

#include <string>
#include <vector>
#include <memory>
#include <map>

enum class NodeType { FILE_NODE, DIR_NODE };

struct FSNode {
    std::string name;
    NodeType type;
    FSNode* parent; // parent pointer (not owning)
    std::map<std::string, std::unique_ptr<FSNode>> children; // directory children
    std::string content; // file contents

    FSNode(const std::string &name_, NodeType t, FSNode* parent_ = nullptr);
    bool is_dir() const { return type == NodeType::DIR_NODE; }
    bool is_file() const { return type == NodeType::FILE_NODE; }
};

class FileSystem {
public:
    FileSystem();

    // high-level ops
    bool mkdir(const std::string &path);
    bool touch(const std::string &path);
    bool remove_file(const std::string &path);   // rm
    bool remove_dir(const std::string &path);    // rmdir (only empty)
    bool write_file(const std::string &path, const std::string &text); // overwrite
    bool cat(const std::string &path, std::string &out) const; // read content
    std::vector<std::string> ls(const std::string &path = "") const;
    bool cd(const std::string &path);
    std::string pwd() const;
    void tree(const std::string &path = "") const;

    // persistence
    bool save_to_file(const std::string &filename) const;
    bool load_from_file(const std::string &filename);

    // helpers
    FSNode* resolve_path(const std::string &path) const; // returns node or nullptr
    FSNode* resolve_parent_of(const std::string &path, std::string &basename) const;
    static std::string escape_json_string(const std::string &s);
    static void skip_ws(const std::string &s, size_t &i);

private:
    std::unique_ptr<FSNode> root;
    FSNode* cwd; // pointer into the tree

    // internal utils
    std::vector<std::string> split_path(const std::string &path) const;
    void tree_recursive(const FSNode* node, const std::string &prefix, bool isLast) const;

    // JSON helpers (implemented in cpp)
    static std::unique_ptr<FSNode> parse_node_from_json(const std::string &json, size_t &idx);
    
};

#endif // FILESYS_H
