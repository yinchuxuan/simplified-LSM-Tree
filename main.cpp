#include <iostream>
#include <string>
#include "kvstore.h"

KVStore store("./data");

int i = 0;

clock_t test_for_memory_put(int size) {
	clock_t start = clock();

	for (int i = 0; i < size; i++) {
		store.put(i, std::string(100, 's'));
	}

	return clock() - start;
}

clock_t test_for_memory_get(int size) {
	clock_t start = clock();

	for (int i = 0; i < size; i++) {
		store.get(i);
	}

	return clock() - start;
}

clock_t test_for_memory_del(int size) {
	clock_t start = clock();

	for (int i = 0; i < size; i++) {
		store.del(i);
	}

	return clock() - start;
}

void test_for_memory(int size) {
	clock_t time1 = 0;
	clock_t time2 = 0;
	clock_t time3 = 0;

	for (int i = 0; i < 10; i++) {
		time1 += test_for_memory_put(size);
		time2 += test_for_memory_get(size);
		time3 += test_for_memory_del(size);
	}

	std::cout << "test for " << size << " memory puts using " << time1 / 10 << "ms" << std::endl;
	std::cout << "test for " << size << " memory gets using " << time2 / 10 << "ms" << std::endl;
	std::cout << "test for " << size << " memory dels using " << time3 / 10 << "ms" << std::endl;
}

void test_for_disk_put() {
	clock_t start = test_for_memory_put(20971);

	store.put(20972, std::string(100, 's'));
	clock_t t = clock() - start;
	std::cout << "test for disk puts using " << t << "ms" << std::endl;
}

void test_for_disk_get() {
	time_t start = time(0);

	for (int i = 0; i < 20972; i++) {
		store.get(i);
	}

	time_t t = time(0) - start;
	std::cout << "test for memory gets using " << t << "s" << std::endl;
}

void test_for_disk_del() {
	time_t start = time(0);

	for (int i = 0; i < 20972; i++) {
		store.del(i);
	}

	time_t t = time(0) - start;
	std::cout << "test for memory dels using " << t << "s" << std::endl;
}

int throughout() {
	int tmp = 0;
	
	clock_t start = clock();

	while (clock() - start < 20000) {
		store.put(i++, std::string(100, 's'));
		tmp++;
	}

	return tmp;
}

int main() {
	std::ofstream outFile;
	outFile.open("throughout", std::ios::out);

	for (int i = 0; i < 50; i++) {
		int tmp = throughout();
		std::cout << tmp / 20 << " ";
		outFile << tmp / 20 << " ";
	}

	outFile.close();

	system("pause");
	return 0;
}