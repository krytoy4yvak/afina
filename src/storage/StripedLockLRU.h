#ifndef AFINA_STORAGE_STRIPED_LOCK_LRU_H
#define AFINA_STORAGE_STRIPED_LOCK_LRU_H

#include <vector>
#include <functional>
#include <string>
#include <cassert>

#include "SimpleLRU.h"
#include "ThreadSafeSimpleLRU.h"

namespace Afina {
namespace Backend {

/**
 * # SimpleLRU thread safe version
 *
 *
 */
class StripedLockLRU : public SimpleLRU {

public:
  
	StripedLockLRU(size_t stripe_limit, size_t stripe_count) {
		this->stripe_count = stripe_count;
		for (int i = 0 ; i < stripe_count ; ++i) {
			std::unique_ptr<ThreadSafeSimplLRU> x(new ThreadSafeSimplLRU(stripe_limit));
			stripes.push_back(std::move(x));	
		}
	}

	static std::shared_ptr<StripedLockLRU> BuildStripedLRU(std::size_t memory_limit = 4 * 1024 * 1024, std::size_t stripe_count = 4) {    
		std::size_t stripe_limit = memory_limit / stripe_count;
		if (stripe_limit < 1024 * 1024) {
			throw std::runtime_error("Stripe size < MIN_STRIPE_SIZE");		
		}
	        return std::make_shared<StripedLockLRU>(stripe_limit, stripe_count);
	}

    ~StripedLockLRU() {
    }

    // see SimpleLRU.h
    bool Put(const std::string &key, const std::string &value) override {
        return stripes[std::hash<std::string>{}(key) % stripe_count]->ThreadSafeSimplLRU::Put(key, value);
    }

    // see SimpleLRU.h
    bool PutIfAbsent(const std::string &key, const std::string &value) override {
        return stripes[std::hash<std::string>{}(key)  % stripe_count]->ThreadSafeSimplLRU::PutIfAbsent(key, value);
    }

    // see SimpleLRU.h
    bool Set(const std::string &key, const std::string &value) override {
        return stripes[std::hash<std::string>{}(key)  % stripe_count]->ThreadSafeSimplLRU::Set(key, value);
    }

    // see SimpleLRU.h
    bool Delete(const std::string &key) override {
        return stripes[std::hash<std::string>{}(key)  % stripe_count]->ThreadSafeSimplLRU::Delete(key);
    }

    // see SimpleLRU.h
    bool Get(const std::string &key, std::string &value) override {
        return stripes[std::hash<std::string>{}(key) % stripe_count]->ThreadSafeSimplLRU::Get(key, value);
    }

private:

    std::vector<std::unique_ptr<ThreadSafeSimplLRU>> stripes;
    std::size_t stripe_count;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_THREAD_SAFE_SIMPLE_LRU_H
