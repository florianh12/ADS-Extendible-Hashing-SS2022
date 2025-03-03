#ifndef ADS_SET_H
#define ADS_SET_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>

template <typename Key, size_t N = 19>
class ADS_set {
public:
	class Iterator;
	using value_type = Key;
	using key_type = Key;
	using reference = value_type &;
	using const_reference = const value_type &;
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;
	using const_iterator = Iterator;
	using iterator = const_iterator;
	using key_equal = std::equal_to<key_type>;	
	using hasher = std::hash<key_type>;			
private:
	struct Bucket;
	void split(Bucket *to_split);
	void expand();
	size_type d{0};
	size_type sz{0};
	Bucket **table{nullptr};
	size_type h(const key_type &key) const { return (hasher{}(key) & ((1 << d) - 1)); }
	struct Bucket {
		size_type b_sz{0};
		size_type t;
		key_type values[N];
		Bucket *next;
		Bucket(size_type t = 0, Bucket *next = nullptr) : t{t}, next{next} {}
		bool push_back(key_type v) {
			if (b_sz >= N)
				return false;
			values[b_sz++] = v;
			return true;
		}
		bool new_v(const key_type &v) const {
			for (size_type i = 0; i < b_sz; i++)
				if (key_equal{}(values[i], v)) {
					return false;
				}
			return true;
		}
		size_type b_count(const key_type& key) const {
			for (size_type i{0}; i < b_sz; i++)
				if (key_equal{}(key, values[i]))
					return 1;
		return 0;	
		}
		void dump(std::ostream &o) const {
			for (size_type i{0}; i < b_sz; i++)
				o << values[i] << ' ';
		}
	};

public:
	ADS_set() : d{0}, sz{0}, table{new Bucket *[(1 << d)] { new Bucket }} {}	   
	ADS_set(std::initializer_list<key_type> ilist) : ADS_set() { insert(ilist); }  
	template <typename InputIt>
	ADS_set(InputIt first, InputIt last) : ADS_set() { insert(first, last); }  
	ADS_set(const ADS_set &other) : ADS_set() {
		insert(other.begin(), other.end());
	}

	~ADS_set() {
		Bucket *tmp{table[0]};
		Bucket *nxt{table[0]->next};
		delete[] table;
		while (nxt != nullptr) {
			delete tmp;
			tmp = nxt;
			nxt = nxt->next;
		}
		delete tmp;
	}

	ADS_set &operator=(const ADS_set &other) {
		if (sz != 0)
			clear();
		insert(other.begin(), other.end());
		return *this;
	}
	ADS_set &operator=(std::initializer_list<key_type> ilist) {
		if (sz != 0)
			clear();
		insert(ilist.begin(), ilist.end());
		return *this;
	}

	size_type size() const { return sz; }														
	bool empty() const { return sz == 0; }														
	void insert(std::initializer_list<key_type> ilist) { insert(ilist.begin(), ilist.end()); }	
	std::pair<iterator, bool> insert(const key_type &key) {
		Bucket *b{table[h(key)]};
		for (size_type i = 0; i < b->b_sz; i++) {
			if (key_equal{}(b->values[i], key)) {
				return std::pair<Iterator, bool>{Iterator{b, i}, false};
			}
		}
		while (true) {
			b = table[h(key)];
			if (b->push_back(key)) {  // Actually add Element
				++sz;
				return std::pair<Iterator, bool>{Iterator{b, ((b->b_sz) - 1)}, true};
			}
			if (b->t < d) {	 // split
				split(b);
			} else {  // expand
				expand();
				split(table[h(key)]);
			}
		}
	}
	template <typename InputIt>
	void insert(InputIt first, InputIt last) {
		for (auto &it{first}; it != last; ++it) {
			Bucket *b{table[h(*it)]};
			if (b->new_v(*it)) {  // Check if new
				while (true) {
					b = table[h(*it)];
					if (b->push_back(*it)) {  // Actually add Element
						++sz;
						break;
					}
					if (b->t < d) {	 // split
						split(b);
					} else {  // expand
						expand();
						split(table[h(*it)]);
					}
				}
			}
		}
	}  

	void clear() {	
		ADS_set tmp;
		swap(tmp);
	}
	size_type erase(const key_type &key) {
		Bucket *bpos{table[h(key)]};
		for (size_type i{0}; i < bpos->b_sz; i++) {
			if (key_equal{}(bpos->values[i], key)) {
				bpos->values[i] = bpos->values[bpos->b_sz - 1];
				bpos->b_sz -= 1;
				sz--;
				return 1;
			}
		}
		return 0;
	}

	size_type count(const key_type &key) const {
		return table[h(key)]->b_count(key);
	}  

	iterator find(const key_type &key) const {
		Bucket* b {table[h(key)]};
		for (size_t i = 0; i < b->b_sz; i++) {
			if (key_equal{}(b->values[i], key))
				return Iterator{b, i};
		}
		return this->end();
	}

	void swap(ADS_set &other) {
		Bucket **help1{this->table};
		size_type help2{this->d};
		this->table = other.table;
		other.table = help1;
		this->d = other.d;
		other.d = help2;
		help2 = this->sz;
		this->sz = other.sz;
		other.sz = help2;
	}

	const_iterator begin() const {
		return Iterator{table[0]};
	}
	const_iterator end() const {
		return Iterator{nullptr};
	}

	void dump(std::ostream &o = std::cerr) const {
		o << "d: " << d << " size: " << sz << '\n';
		for (size_type i{0}; i < static_cast<size_type>((1 << d)); i++) {
			bool ptr{false};
			o << "B" << i << ": ";
			for (size_type j{0}; j < i; j++)
				if (table[i] == table[j] && i != j) {
					ptr = true;
					o << "P" << j << " ";
				}
			if (!ptr)
				table[i]->dump(o);
			o << '\n';
		}
	}

	friend bool operator==(const ADS_set &lhs, const ADS_set &rhs) {  
		if (lhs.sz != rhs.sz)
			return false;
		ADS_set<Key, N>::Iterator tmp{lhs.begin()};
		ADS_set<Key, N>::Iterator tmp_e{lhs.end()};
		for (; tmp != tmp_e; ++tmp) {
			if (rhs.count(*tmp) == 0) {
				return false;
			}
		}
		return true;
	}
	friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs) {
		return !(lhs == rhs);
	}
};

template <typename Key, size_t N>
void ADS_set<Key, N>::expand() {
	size_type help{static_cast<size_type>(1 << d)};
	++d;
	Bucket **tmp{new Bucket *[(1 << d)]};
	for (size_type i{0}; i < help; i++) {
		tmp[i] = table[i];
		tmp[(i + help)] = table[i];
	}
	delete[] table;
	table = tmp;
}

template <typename Key, size_t N>
void ADS_set<Key, N>::split(Bucket *to_split) {
	size_type p_old{(h(to_split->values[0]) & ((1 << to_split->t) - 1))};
	size_type p_new{p_old + (1 << to_split->t)};
	table[p_old]->t += 1;
	size_type amount{static_cast<size_type>(1 << (d - table[p_old]->t))};
	size_type add{static_cast<size_type>(1 << table[p_old]->t)};
	table[p_new] = new Bucket{table[p_old]->t, table[p_old]->next};
	table[p_old]->next = table[p_new];
	for (size_type i{1}; i < amount; i++) {
		table[(p_new + i * add)] = table[p_new];
	}
	size_type n_sz{0};
	for (size_type i{0}; i < (table[p_old]->b_sz); i++) {
		if ((hasher{}(table[p_old]->values[i]) & ((1 << table[p_old]->t) - 1)) == p_new) {
			table[p_new]->push_back((table[p_old]->values[i]));
		} else {
			if (n_sz < i)
				table[p_old]->values[n_sz] = table[p_old]->values[i];
			++n_sz;
		}
	}
	table[p_old]->b_sz = n_sz;
}

template <typename Key, size_t N>
class ADS_set<Key, N>::Iterator {
public:
	using value_type = Key;
	using difference_type = std::ptrdiff_t;
	using reference = const value_type &;
	using pointer = const value_type *;
	using iterator_category = std::forward_iterator_tag;
	Bucket *ptr{nullptr};
	size_type curr_elem{0};
	explicit Iterator(Bucket *ptr = nullptr, size_type curr_elem = 0) : ptr{ptr}, curr_elem{curr_elem} {
		if (ptr != nullptr && 0 == ptr->b_sz) {
			++(*this);
		}
	}
	reference operator*() const { return ptr->values[curr_elem]; }
	pointer operator->() const { return &ptr->values[curr_elem]; }
	Iterator &operator++() {
		++curr_elem;
		if (curr_elem >= ptr->b_sz) {
			ptr = ptr->next;
			curr_elem = 0;
			while (ptr != nullptr && ptr->b_sz == 0) {
				ptr = ptr->next;
			}
		}
		return *this;
	}
	Iterator operator++(int) {
		Iterator help{*this};
		++curr_elem;
		if (curr_elem >= ptr->b_sz) {
			ptr = ptr->next;
			curr_elem = 0;
			while (ptr != nullptr && ptr->b_sz == 0) {
				ptr = ptr->next;
			}
		}
		return help;
	}
	friend bool operator==(const Iterator &lhs, const Iterator &rhs) {
		return lhs.ptr == rhs.ptr && lhs.curr_elem == rhs.curr_elem;
	}
	friend bool operator!=(const Iterator &lhs, const Iterator &rhs) {
		return lhs.ptr != rhs.ptr || lhs.curr_elem != rhs.curr_elem;
	}
};

template <typename Key, size_t N>
void swap(ADS_set<Key, N> &lhs, ADS_set<Key, N> &rhs) { lhs.swap(rhs); }

#endif	// ADS_SET_H
