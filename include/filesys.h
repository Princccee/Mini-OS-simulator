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
    FSNode* parent; // raw pointer to parent (owned by map in parent)
    std::map<std::string, std::unique_ptr<FSNode>> children; // for dirs
    std::string content; // for files

    FSNode(const std::string &name_, NodeType t, FSNode* parent_ = nullptr);
    bool is_dir() const { return type == NodeType::DIR_NODE; }
    bool is_file() const { return type == NodeType::FILE_NODE; }
};

class FileSystem {
public:
    FileSystem();

    // high-level ops (return true on success)
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

    // helpers
    FSNode* resolve_path(const std::string &path) const; // returns node or nullptr
    FSNode* resolve_parent_of(const std::string &path, std::string &basename) const;

private:
    std::unique_ptr<FSNode> root;
    FSNode* cwd; // pointer into tree

    // internal utils
    std::vector<std::string> split_path(const std::string &path) const;
    void tree_recursive(const FSNode* node, const std::string &prefix, bool isLast) const;
};

#endif // FILESYS_H
