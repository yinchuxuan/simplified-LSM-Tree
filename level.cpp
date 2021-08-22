#include "level.h"

/**
 * Add SSTable to level.
 * Create a new SSTable(.dat file), naming after the size of level.
 * Write pair into the SSTable and record index on indextable. 
 */
void addSSTable(const quadlist<std::pair<uint64_t, std::string>> &l, level *le){
	if (!l.empty()) {
		le->size++;
		fs::path name = le->levelPath / (std::to_string(le->size) + ".dat");		//the file path of new SSTable
		fs::path indexPath = le->levelPath / "index" / (std::to_string(le->size) + ".dat");

		quadnode<std::pair<uint64_t, std::string>> *tmp = l.first()->next;
		std::vector<index> pairIndex;
		uint64_t offset = 0;

		//traverse pair list
		//write data to SSTable file and record on indextable
		std::ofstream outFile1(name.string(), std::ios::out | std::ios::binary);
		std::ofstream outFile2(indexPath.string(), std::ios::out | std::ios::binary);
		while (!isTailer(tmp)) {
			int SizeOfData = tmp->data.second.size() + 1;
			std::string buf = tmp->data.second + "\0";
			const char *buffer = buf.c_str();
			outFile1.write(buffer, SizeOfData);
			index i = index((tmp->data).first, offset, SizeOfData, le->size, le->order);
			pairIndex.push_back(i);
			outFile2.write((char*)&i, sizeof(i));
			offset += SizeOfData;
			tmp = tmp->next;
		}
		outFile1.close();
		outFile2.close();

		std::sort(pairIndex.begin(), pairIndex.end());
		le->indextable->push_back(level::IndexTable(pairIndex, name));

		if (pairIndex.front().key < le->minKey) {		//update minKey
			le->minKey = pairIndex.front().key;
		}

		if (pairIndex.back().key > le->maxKey) {		//update maxKey
			le->maxKey = pairIndex.back().key;
		}
	}
}

/**
 * Binary Search in SSTable' index.
 * Return the pair's position in vector of index.
 * If fail to find the pair, return -1.
 */
int level::binarySearch(const std::vector<index> &l, uint64_t key) const{
    int left = 0, right = l.size() - 1;

    while(left <= right){
        int mid = (left + right) / 2;
        if(key ==  l[mid].key){
            return mid;
        }else if(key < l[mid].key){
            right = mid - 1;
        }else{
            left = mid + 1;
        }
    }

    return -1;
}   

/**
 * Read value from SSTable.
 * If fail to open SSTable, throw run_time error.
 */
std::string level::ReadFromSSTable(uint64_t offset, fs::path name, uint64_t size) const{
    std::ifstream inFile(name.string(), std::ios::in | std::ios::binary);
    if(!inFile){
        throw std::runtime_error("fail to open SSTable!");
    }

    char buffer[100000];
    inFile.seekg(offset, std::ios::beg);        //set file pointer to the right position
    inFile.read(buffer, size);

	inFile.close();
    return std::string(buffer);
}

/**
 * Get value from all the SSTable in this level.
 * If fail to find the pair, return empty string.
 */
std::string level::get(uint64_t key) const{
    //traverse all the SSTable in this level
	if(size != 0) {
		clock_t stamp = 0;
		int position = -1;
		std::list<IndexTable>::iterator it = indextable->end();

		//find the latest updated item in this level
		for (std::list<IndexTable>::iterator iter = indextable->begin(); iter != indextable->end(); iter++) {
			int tmp = binarySearch(iter->first, key);
			if (tmp != -1 ) {
				if (iter->first[tmp].timeStamp > stamp) {
					position = tmp;
					it = iter;
					stamp = iter->first[tmp].timeStamp;
				}
			}
		}

		if (it != indextable->end() && !it->first[position].flag) {
			return ReadFromSSTable(it->first[position].offset, it->second, it->first[position].size);
		}
	}

    return "";
}

/**
 * Lazy delete the pair in this level.
 * If the pair is in this level, change its flag and update
 * its timeStamp, return true, else return false.
 */
bool level::del(uint64_t key){
	if (size != 0) {
		for (std::list<IndexTable>::iterator iter = indextable->begin(); iter != indextable->end(); iter++) {
			int position = binarySearch(iter->first, key);
			if (position != -1 && !iter->first[position].flag) {
				iter->first[position].flag = true;
				iter->first[position].timeStamp = clock();

				fs::path indexPath = levelPath / "index" / (std::to_string(iter->first[position].order) + ".dat");
				fs::remove(indexPath);
				std::ofstream outFile(indexPath.string(), std::ios::out | std::ios::binary);

				for (std::vector<index>::iterator i = iter->first.begin(); i != iter->first.begin(); i++) {
					outFile.write((char*)&(*i), sizeof(*i));
				}
				return true;
			}
		}
	}

	return false;
}

/**
 * Find all the SSTable in next level that is covered 
 * by the range of keys in this level.
 */
std::list<level::IndexTable*> level::findCoveredTable() const {
	std::list<IndexTable*> result;

	for (std::list<IndexTable>::iterator iter = nextLevel->indextable->begin(); iter != nextLevel->indextable->end(); iter++) {
		if (iter->first.front().key <= maxKey && iter->first.back().key >= minKey) {
			result.push_back(&(*iter));
		}
	}

	return result;
}

/**
 * Merge all the same keys in the sorted tmpIndexTable.
 * Remain the nearest index and delete all the others.
 */
void level::merge(std::vector<index> &tmpIndexTable) {
	for (std::vector<index>::iterator iter = tmpIndexTable.begin(); iter != tmpIndexTable.end();iter++) {
		std::vector<index>::iterator next = iter + 1;
		clock_t timeStamp = iter->timeStamp;
		std::vector<index>::iterator nearestOne = iter;
		
		//find the nearest index among the same keys
		while (next != tmpIndexTable.end() && next->key == iter->key) {
			if (next->timeStamp > timeStamp) {
				nearestOne = next;
				timeStamp = next->timeStamp;
			}
			next++;
		}

		//delete all the other indexs
		if (nearestOne->flag) {			//if the nearest index is deleted
			iter = --tmpIndexTable.erase(iter, next);
		} 
		else {
			if (next != iter + 1) {
				int offset = next - nearestOne;
				nearestOne = tmpIndexTable.erase(iter, nearestOne);
				next = nearestOne + offset;
				iter = --tmpIndexTable.erase(++nearestOne, next);
			}
		}
	}
}

/**
 * Find whether the iter is in the list
 */
bool level::inTable(std::list<IndexTable>::iterator &iter, std::list<IndexTable*> &l) const{
	for (std::list<IndexTable*>::iterator i = l.begin(); i != l.end(); i++) {
		if (&(*iter) == *i) {
			return true;
		}
	}

	return false;
}

/**
 * Rename all the SSTables in this level.
 * Naming them by order.
 */
void level::renaming() {
	int Size = 0;
	fs::remove_all(levelPath / "index");
	fs::create_directory(levelPath / "index");

	for (std::list<IndexTable>::iterator iter = indextable->begin(); iter != indextable->end(); iter++) {
		Size++;
		fs::path name = levelPath / (std::to_string(Size) + ".dat");
		fs::rename(iter->second, name);
		fs::path indexPath = levelPath / "index" / (std::to_string(Size) + ".dat");
		std::ofstream outFile(indexPath.string(), std::ios::out | std::ios::binary);

		for (std::vector<index>::iterator j = iter->first.begin(); j != iter->first.end(); j++) {
			outFile.write((char *)&(*j), sizeof(*j));
		}

		for (std::vector<index>::iterator i = iter->first.begin(); i != iter->first.end(); i++) {
			i->level = order;
			i->order = Size;
		}
	}
}

/**
 * Do compaction between two level when overflow happened.
 * Find all the covered SSTables in next level and merge them with
 * all the SSTables in this level, then write them to the next level.
 */
void level::compaction() {
	if (nextLevel == nullptr) {					//if this is the bottom level 
		throw std::runtime_error("There is not enough memory to store these data!");
	}

	std::list<IndexTable*> CoveredTable = findCoveredTable();
	std::list<IndexTable*> AllTable = CoveredTable;			//all the SSTables that will join the compaction

	for (std::list<IndexTable>::iterator iter = indextable->begin(); iter != indextable->end(); iter++) {
		AllTable.push_front(&(*iter));
	}

	std::vector<index> tmpIndexTable;			//all indexs contained in these SSTables

	for (std::list<IndexTable*>::iterator iter = AllTable.begin(); iter != AllTable.end(); iter++) {
		std::vector<index>* tmp = &((*iter)->first);
		for (std::vector<index>::iterator iter = tmp->begin(); iter != tmp->end(); iter++) {
			tmpIndexTable.push_back(*iter);
		}
	}

	std::sort(tmpIndexTable.begin(), tmpIndexTable.end());
	merge(tmpIndexTable);

	//create a SSTable in next level for every 2MB data
	unsigned Size = 0;
	quadlist<std::pair<uint64_t, std::string>> tmp;
	for (std::vector<index>::iterator i = tmpIndexTable.begin(); i != tmpIndexTable.end(); i++) {
		tmp.insert_back(std::pair(i->key, ReadFromSSTable(i->offset, levelPath.parent_path() / ("level" + std::to_string(i->level)) / (std::to_string(i->order) + ".dat"), i->size)));
		Size += i->size;

		if (Size >= 2097152) {
			addSSTable(tmp, nextLevel);
			tmp.clear();
			Size = 0;
		}
	}

	addSSTable(tmp, nextLevel);			//create a SSTable in next level for rest data

	//delete all indexs and SSTables that join the compaction in this level and next level
	for (auto &iter:fs::directory_iterator(levelPath)) {
		fs::remove_all(iter.path());
	}
	fs::create_directory(levelPath / "index");
	indextable->clear();
	size = 0;
	minKey = 100000000;
	maxKey = 0;

	nextLevel->minKey = 100000000;
	nextLevel->maxKey = 0;
	for (std::list<IndexTable>::iterator iter = nextLevel->indextable->begin(); iter != nextLevel->indextable->end(); iter++) {
		if (inTable(iter,CoveredTable)) {
			fs::remove(iter->second);
			iter = nextLevel->indextable->erase(iter);
			nextLevel->size--;
		}
		else {
			if (iter->first.front().key < nextLevel->minKey) {
				nextLevel->minKey = iter->first.front().key;
			}

			if (iter->first.back().key > nextLevel->maxKey) {
				nextLevel->maxKey = iter->first.back().key;
			}
		}
	}

	nextLevel->renaming();

	if (nextLevel->size > nextLevel->capacity) {
		nextLevel->compaction();
	}
}

/**
* Restore index from disk 
* Read index infomation in each level and write it
* to indextable of each level
*/
void level::restoreIndex() {
	for (auto &iter : fs::directory_iterator(levelPath / "index")) {
		std::ifstream inFile(iter.path().string(), std::ios::in | std::ios::binary);
		std::vector<index> indexlist;
		index tmp;

		//read each item and refresh minKey and maxKey
		while (inFile.read((char*)&tmp, sizeof(tmp))) {
			indexlist.push_back(tmp);

			if (tmp.key > maxKey) {
				maxKey = tmp.key;
			}

			if (tmp.key < minKey) {
				minKey = tmp.key;
			}
		}

		indextable->push_back(IndexTable(indexlist, levelPath / (std::to_string(tmp.order) + ".dat")));
		size++;
	}
}
