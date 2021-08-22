#include "kvstore.h"
#include <string>

//constructor
KVStore::KVStore(const std::string &dir): KVStoreAPI(dir),ptrToMemTable(std::make_shared<memTable>()),storage(dir),ptrToLevelTable(std::make_shared<std::list<level>>()), SizeOfMemTable(0){
	try{
		if (!fs::exists(storage)) {
			if (!fs::create_directory(storage)) {
				throw std::runtime_error("the creation of dir has failed!");
			}
		}

		for (int i = 9; i >= 0; i--) {
			fs::path Level = fs::path(dir) / ("level" + std::to_string(i));
			if (ptrToLevelTable->empty()) {
				ptrToLevelTable->push_front(level(i,Level, pow(2, i + 1), 0));
			}
			else {
				ptrToLevelTable->push_front(level(i,Level, pow(2, i + 1), 0, &(*ptrToLevelTable->begin())));
			}

			if (!fs::exists(Level)) {
				if (!fs::create_directory(Level) || !fs::create_directory(Level / "index")) {
					throw std::runtime_error("the creation of dir has failed!");
				}
			}
			else {
				ptrToLevelTable->front().restoreIndex();
			}
		}
	}catch(const std::exception &e){
		std::cerr << e.what() << std::endl;
		exit(1);
	}
}

KVStore::~KVStore(){}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s){
	try{
		putIntoMemTable(key, s);
		SizeOfMemTable += s.size();

		if(MemTableIsFull()){
			transfer();
		}
	}catch(const std::exception &e){
		std::cerr << e.what() << std::endl;
		exit(1);
	}
}

/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key){
	node *position = findInMemTable(key);		//try to find pair in memtable

	if(position != nullptr){					//the pair is in memtable
		return (position->data).second;
	}

	//try to find pair in each level(from level0)
	for(std::list<level>::iterator iter = ptrToLevelTable->begin(); iter != ptrToLevelTable->end();iter++){		
		std::string result = iter->get(key);
		if(result != ""){
			return result;
		}
	}

	return "";
}

/**
 * Delete the given key-value pair if it exists.
 * Return false iff the key is not found.
 */
bool KVStore::del(uint64_t key){
	if(findInMemTable(key) != nullptr){
		deleteInMemTable(key);
		return true;
	}

	for (std::list<level>::iterator iter = ptrToLevelTable->begin(); iter != ptrToLevelTable->end(); iter++) {
		if (iter->del(key)) {
			return true;
		}
	}

	return false;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset(){
	ptrToMemTable->clear();			//clear memtable
}

/**
 * Transfers memtable to SSTable.
 * It removes all pairs in memtable and add a SSTable in level 0, register
 * infomation on index system. If level 0 overflows, then do compaction.
 */
void KVStore::transfer(){
	try {
		addSSTable(ptrToMemTable->pairList(), &(ptrToLevelTable->front())); 	//add SSTable to level0

		if (ptrToLevelTable->front().Size() > ptrToLevelTable->front().Capacity()) {					//if overflow, do compaction
			ptrToLevelTable->front().compaction();
		}

		ptrToMemTable->clear();		//clear memtable

		SizeOfMemTable = 0;
	}catch (const std::exception &e) {
		std::cout << e.what() << std::endl;
		exit(1);
	}
}

