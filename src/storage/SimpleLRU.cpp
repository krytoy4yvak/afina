#include <iostream>
#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

bool SimpleLRU::Delete(const std::string &key) {
    auto map_iter = _lru_index.find(std::ref(key));
    if (map_iter == _lru_index.end()) {
        return false;
    }
    lru_node &node = map_iter->second.get();
    _lru_index.erase(map_iter);
    _cur_size -= (node.key.length() + node.value.length());
    lru_node *prev = node.prev;
    if (prev == nullptr) {
        std::swap(_lru_head, node.next);
        if (_lru_head.get() != nullptr)
            _lru_head.get()->prev = nullptr;
        node.next.reset();
    } else {
        if (node.next.get() == nullptr) {
            _lru_tail = prev;
            node.prev = nullptr;
            prev->next.reset();
        } else {
            std::swap(prev->next, node.next);
            node.prev = nullptr;
            node.next.reset();
        }
    }
    return true;
}

void SimpleLRU::add_elem(const std::string &key, const std::string &value) {
    lru_node *new_node = new lru_node{key, value, nullptr, std::move(_lru_head)};
    if (new_node->next.get() != nullptr) {
        new_node->next.get()->prev = new_node;
    }
    _lru_head = std::unique_ptr<lru_node>(new_node);
    if (_cur_size == 0) {
        _lru_tail = _lru_head.get();
    }
    _lru_index.emplace(std::make_pair(std::ref(new_node->key), std::ref(*new_node)));
}

bool SimpleLRU::add_node(const std::string &key, const std::string &value) {
    std::size_t size = key.length() + value.length();
    if (size > _max_size) {
        return false;
    }
    std::size_t new_size = _cur_size + size;
    if (new_size <= _max_size) {
        add_elem(key, value);
        _cur_size = new_size;
    } else {
        clear_space(0, size);
        add_elem(key, value);
    }
    std::cout << "curr_size " << _cur_size << "key_size " << key.length() << "value_size " << value.length() << std::endl;
    return true;
}

void SimpleLRU::clear_space(std::size_t old_len, std::size_t new_len) {
    while (_cur_size - old_len + new_len > _max_size) {
        _cur_size -= (_lru_tail->key.length() + _lru_tail->value.length());
        std::cout << "key" << _lru_tail->key << "value" << _lru_tail->value << std::endl;
        _lru_index.erase(_lru_tail->key);
        auto prev = _lru_tail->prev;
        prev->next.reset();
        _lru_tail = prev;
        if (_lru_tail == nullptr) {
            _lru_head.reset();
        } else {
            _lru_tail->next.reset();
        }
    }
    _cur_size = _cur_size - old_len + new_len;
}

void SimpleLRU::go_head(lru_node &node) {
    lru_node *prev = node.prev;
    if (prev == nullptr) {
        return;
    }
    if (node.next.get() == nullptr) {
        _lru_tail = prev;
    } else {
        node.next.get()->prev = prev;
    }
    node.prev = nullptr;
    std::swap(prev->next, node.next);
    std::swap(node.next, _lru_head);
    node.next.get()->prev = _lru_head.get();
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    auto map_iter = _lru_index.find(std::ref(key));
    if (map_iter == _lru_index.end()) {
        return add_node(key, value);
    }
    return Set(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    auto map_iter = _lru_index.find(std::ref(key));
    if (map_iter == _lru_index.end()) {
        return add_node(key, value);
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    std::size_t size = key.length() + value.length();
    if (size > _max_size) {
        return false;
    }
    auto map_iter = _lru_index.find(std::ref(key));
    if (map_iter == _lru_index.end()) {
        return false;
    }
    lru_node &node = map_iter->second.get();
    go_head(node);
    std::size_t old_len = node.value.length();
    std::size_t new_len = value.length();
    clear_space(old_len, new_len);
    map_iter->second.get().value = value;
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto map_iter = _lru_index.find(std::ref(key));
    if (map_iter == _lru_index.end()) {
        return false;
    }
    value = map_iter->second.get().value;
//    go_head(map_iter->second.get());
    return true;
}

} // namespace Backend
} // namespace Afina