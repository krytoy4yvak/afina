#include "SimpleLRU.h"

namespace Afina {
namespace Backend {


bool SimpleLRU::Delete(const std::string &key) {
	auto map_iter = _lru_index.find(std::ref(key)); 
	if (map_iter == _lru_index.end()) {
		return false;	
	}
        lru_node & node = map_iter->second.get();
	_lru_index.erase(map_iter);
	_cur_size -= node.key.length();
	_cur_size -= node.value.length();
	lru_node * left = node.prev;
	if (left == nullptr) {
		//если node - первый элемент
		std::swap(_lru_head, node.next);
		if (_lru_head.get() != nullptr)
			_lru_head.get()->prev = nullptr;
		node.next.reset();		
	}
	else {
		if (node.next.get() == nullptr) {
			//если node - последний элемент
			tail = left;
			node.prev = nullptr;
			left->next.reset();
		}
		else { //если node -не первый и не последний элемент
			std::swap(left->next, node.next);
			left->next.get()->prev = left;
			node.prev = nullptr;		
			node.next.reset();
		}
	}
	return true;
}

void SimpleLRU::push_front(const std::string & key, const std::string &value) {
	lru_node * new_node = new lru_node {key, value, nullptr, std::move(_lru_head)};
	if (new_node->next.get() != nullptr) {
		new_node->next.get()->prev = new_node;	
	}
	_lru_head = std::unique_ptr<lru_node>(new_node);
	if (_cur_size == 0) { //если это первый элемент
		tail = _lru_head.get();
	}
        _lru_index.emplace(std::make_pair(std::ref(new_node->key), std::ref(*new_node)));
}

bool SimpleLRU::add_node(const std::string & key, const std::string &value) {
	std::size_t size = key.length() + value.length();
	if (size > _max_size) {
		return false;
	}
	std::size_t new_size = _cur_size + size ;
	if (new_size <= _max_size) {
	//нам хватает места, чтобы вставить новый элемент
		push_front(key, value);
		_cur_size = new_size; 	
	}
	else { 
		while (_cur_size + size > _max_size) {
			_cur_size -= tail->key.length();
			_cur_size -= tail->value.length();
			_lru_index.erase(tail->key);
			tail = tail->prev;
			if (tail == nullptr) {
				_lru_head.reset();
			}
			else {
				tail->next.get()->prev = nullptr;
				tail->next.reset();
			}
		}
		push_front(key, value);
		_cur_size += size; 	
	}
	return true;
}



void SimpleLRU::ensure_space(std::size_t old_len, std::size_t new_len) {
	while (_cur_size - old_len + new_len  > _max_size) {
		_cur_size -= tail->key.length();
		_cur_size -= tail->value.length();
		_lru_index.erase(tail->key);
		tail = tail->prev;
		tail->next.get()->prev = nullptr;
		tail->next.reset();
	}
	_cur_size = _cur_size - old_len + new_len ;
}


void SimpleLRU::move_to_head(std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<std::string>>::iterator & map_iter) {
	lru_node & node = map_iter->second.get();
	lru_node * left = node.prev;	
	if (left != nullptr) {
		//если node не первый элемент, то переместим его в начало списка
		if (node.next.get() == nullptr) {
			tail = left;
		}
		else { 
			node.next.get()->prev = left;							
		}
		node.prev = nullptr;
		std::swap(left->next, node.next);
		std::swap(node.next,_lru_head);
		node.next.get()->prev = _lru_head.get();
	}
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
	auto map_iter = _lru_index.find(std::ref(key)); 
	if (map_iter == _lru_index.end()) {
		return add_node(key, value);	
	}
	move_to_head(map_iter);
	lru_node & node = map_iter->second.get();
	std::size_t old_len = node.value.length();
	std::size_t new_len = value.length();
	ensure_space(old_len, new_len);
	map_iter->second.get().value = value;		
	return true;	
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
	move_to_head(map_iter);
	lru_node & node = map_iter->second.get();
	std::size_t old_len = node.value.length();
	std::size_t new_len = value.length();
	ensure_space(old_len, new_len);
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
	return true;	
}

} // namespace Backend
} // namespace Afina
