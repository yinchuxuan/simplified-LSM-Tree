#pragma once

#include <math.h>
#include "level.h"
#include "kvstore_api.h"
#include "skiplist.h"

class KVStore : public KVStoreAPI{
	// You can add your implementation here

	typedef skiplist<uint64_t, std::string> memTable;
	typedef quadnode<std::pair<uint64_t, std::string>> node;
	friend class level;

	private:
		std::shared_ptr<memTable> ptrToMemTable;				//resourse manager of memtable
		fs::path storage;		//path of data storage
		std::shared_ptr<std::list<level>> ptrToLevelTable; 		//resourse manager of leveltable
		uint64_t SizeOfMemTable; 		//size of memtable

		void putIntoMemTable(uint64_t key, const std::string &s){ //put pair into memtable
			ptrToMemTable->put(key, s);
		}	

		bool MemTableIsFull() const{		//whether the memtable is full
			return SizeOfMemTable >= 2097152;
		}

		node *findInMemTable(uint64_t key){			//find pair in memtable
			return ptrToMemTable->find(key);
		}

		void deleteInMemTable(uint64_t key){		//delete pair in memtable
			SizeOfMemTable -= ptrToMemTable->remove(key);
		}

		void transfer();		//transfer memtable to SSTable

	public:
		KVStore(const std::string &dir);

		~KVStore();

		void put(uint64_t key, const std::string &s) override;

		std::string get(uint64_t key) override;

		bool del(uint64_t key) override;

		void reset() override;
};
