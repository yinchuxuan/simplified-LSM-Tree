#pragma once

#include <list>
#include <vector>
#include <filesystem>
#include <fstream>
#include <ctime>
#include "quadlist.h"

namespace fs = std::filesystem;

//index about a pair
struct index{
	uint64_t key, offset, size, order, level;

	clock_t timeStamp;

	bool flag;

	index(){}

	index(uint64_t k, uint64_t o, uint64_t s, uint64_t ord, uint64_t l):key(k),offset(o),size(s),order(ord),level(l),timeStamp(clock()),flag(false){}

	//operator< overload for sorting
	bool operator<(const index &i){
		return key < i.key;
	}
};

class level{

	typedef std::pair<std::vector<index>, fs::path> IndexTable;
	typedef std::list<level>::iterator Iter;

	friend void addSSTable(const quadlist<std::pair<uint64_t, std::string>> &l, level *le);

	protected:
		uint64_t order;
		const fs::path levelPath;     //filepath of the level
		uint64_t capacity;      //capacity of the level(number of SSTable)
		uint64_t size;          //current size of the level
		std::shared_ptr<std::list<IndexTable>> indextable;      //index table for all SSTable in the level
		level *nextLevel;		//do compaction with this level
		uint64_t maxKey;		//maximum key in this level
		uint64_t minKey;		//minimum key in this level

        int binarySearch(const std::vector<index> &l, uint64_t key) const;      //binary search in index list

        std::string ReadFromSSTable(uint64_t offset, fs::path name, uint64_t size) const;       //read value from SSTable according to offset

		std::list<IndexTable*> findCoveredTable() const;		//find all the SSTable in the nextlevel that is covered by the range 

		void merge(std::vector<index> &tmpIndexTable);			//merge all the index that has same key by timeStamp

		bool inTable(std::list<IndexTable>::iterator &iter, std::list<IndexTable*> &l) const;		//whether the iter is in table

		void renaming();		//renaming all SSTable in this level

    public:
		level(uint64_t o, const fs::path &p, uint64_t c, uint64_t s, level *l = nullptr):order(o),levelPath(p),capacity(c),size(s),indextable(std::make_shared<std::list<IndexTable>>(std::list<IndexTable>())),nextLevel(l),maxKey(0),minKey(100000000){}

        ~level(){}

        std::string get(uint64_t key) const;        //get value from the SSTable in the level

		bool del(uint64_t key);		//lazy delete 

		void compaction();		//do compaction when the level overflow

		void restoreIndex();		//restore index from disk to memory

		uint64_t Size() const {
			return size;
		}

		uint64_t Capacity() const {
			return capacity;
		}
};