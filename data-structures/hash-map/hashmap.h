#include <iostream>
#include <tuple>

namespace dsa {

template <typename K, typename V>
class Hash_map {
public:
  explicit Hash_map(size_t size = 4, double max_load_factor = 0.75);
  Hash_map(const Hash_map& other);
  void clear();
  virtual ~Hash_map();
  Hash_map& operator=(const Hash_map& rhs);
  V& operator[](const K& key);
  void insert(const K& key, const V& value);
  void set(const K& key, const V& value);
  void remove(const K& key);
  bool contains(const K& key) const;
  bool operator==(const Hash_map& rhs) const;
  size_t size() const;
  size_t capacity() const;
  double current_load_factor() const;
  double max_load_factor() const;
  void set_max_load_factor(double x);

private:
  static constexpr double maxlf_max = 0.75;
  static constexpr double maxlf_min = 0.20;

  enum Mark { null,
    live,
    dead };
  size_t size_;
  size_t capacity_;
  K* keys_;
  V* values_;
  Mark* marks_;
  double max_load_factor_;
  size_t threshold_size_;
  std::hash<K> hasher_;

  size_t& normalize_capacity(size_t& capacity) const;
  double& normalize_maxlf(double& maxlf) const;
  size_t seek(const K& key) const;
  void increase_capacity();
};

template <typename KeyType, typename ValueType>
constexpr double Hash_map<KeyType, ValueType>::maxlf_max;

template <typename KeyType, typename ValueType>
constexpr double Hash_map<KeyType, ValueType>::maxlf_min;

template <typename K, typename V>
Hash_map<K, V>::Hash_map(size_t size, double max_load_factor) :
    size_(0),
    capacity_(normalize_capacity(size)),
    max_load_factor_(normalize_maxlf(max_load_factor)),
    threshold_size_(max_load_factor_ * capacity_),
    keys_(new K[capacity_]()),
    values_(new V[capacity_]()),
    marks_(new Mark[capacity_]()),
    hasher_(std::hash<K>()) {}

template <typename K, typename V>
Hash_map<K, V>::Hash_map(const Hash_map& other) :
    size_(other.size_),
    capacity_(other.capacity_),
    max_load_factor_(other.max_load_factor_),
    threshold_size_(max_load_factor_ * capacity_),
    keys_(new K[capacity_]()),
    values_(new V[capacity_]()),
    marks_(new Mark[capacity_]()),
    hasher_(other.hasher_) {

  for (int i = 0; i < capacity_; i++) {
    keys_[i] = other.keys_[i];
    values_[i] = other.keys_[i];
    marks_[i] = other.marks_[i];
  }
}

template <typename K, typename V>
inline size_t& Hash_map<K, V>::normalize_capacity(size_t& cap) const {
  if (cap < 4) {
    cap = 4;
  } else if (((cap - 1) & cap) != 0) {
    cap = 1 << (32 - __builtin_clz(cap));
  }
  return cap;
}

template <typename K, typename V>
inline double& Hash_map<K, V>::normalize_maxlf(double& maxlf) const {
  if (maxlf < maxlf_min) {
    maxlf = maxlf_min;
  } else if (maxlf > maxlf_max) {
    maxlf = maxlf_max;
  }
  return maxlf;
}

template <typename K, typename V>
inline size_t Hash_map<K, V>::seek(const K& key) const {
  auto keep_bits = capacity_ - 1;
  auto index = hasher_(key) & keep_bits;
  while (keys_[index] != key && marks_[index] != null) {
    index = (index + 1) & keep_bits;
  }
  return index;
}

template <typename K, typename V>
void Hash_map<K, V>::increase_capacity() {
  auto tmp_cap = (capacity_ == 0) ? 4 : (capacity_ << 1);
  auto tmp_keys = new K[tmp_cap]();
  auto tmp_values = new V[tmp_cap]();
  auto tmp_marks = new Mark[tmp_cap]();
  auto keep_bits = tmp_cap - 1;
  for (int i = 0; i < capacity_; i++) {
    if (marks_[i] == live) {
      auto index = hasher_(keys_[i]) & keep_bits;
      while (tmp_marks[index] != null) {
        index = (index + 1) & keep_bits;
      }
      std::tie(tmp_keys[index], tmp_values[index], tmp_marks[index]) =
          std::make_tuple(keys_[i], values_[i], marks_[i]);
    }
  }
  clear();
  std::tie(capacity_, keys_, values_, marks_, threshold_size_) =
      std::make_tuple(tmp_cap, tmp_keys, tmp_values, tmp_marks,
                      (max_load_factor_ * capacity_));
}

template <typename K, typename V>
inline void Hash_map<K, V>::clear() {
  delete[] keys_, delete[] values_, delete[] marks_;
  std::tie(keys_, values_, marks_) =
      std::make_tuple(nullptr, nullptr, nullptr);
  std::tie(size_, capacity_, threshold_size_, max_load_factor_) =
      std::make_tuple(0, 0, 0, 0);
}

template <typename K, typename V>
Hash_map<K, V>::~Hash_map() { clear(); }

template <typename K, typename V>
inline Hash_map<K, V>& Hash_map<K, V>::operator=(const Hash_map<K, V>& rhs) {
  if (this != &rhs) {
    clear();
    std::tie(size_, capacity_, threshold_size_, max_load_factor_) =
        std::make_tuple(rhs.size_, rhs.capacity_,
                        rhs.threshold_size_, rhs.max_load_factor_);
    keys_ = new K[capacity_]();
    values_ = new V[capacity_]();
    marks_ = new Mark[capacity_]();
    for (int i = 0; i < capacity_; i++) {
      std::tie(keys_[i], values_[i], marks_[i]) =
          std::make_tuple(rhs.keys_[i], rhs.values_[i], rhs.marks_[i]);
    }
  }
  return *this;
}

template <typename K, typename V>
inline V& Hash_map<K, V>::operator[](const K& key) {
  auto index = seek(key);
  if (marks_[index] == null) {
    while (size_ >= threshold_size_) {
      increase_capacity();
    }
    keys_[index] = key;
    marks_[index] = live;
    size_++;
  }
  return values_[index];
}

template <typename K, typename V>
inline void Hash_map<K, V>::insert(const K& key, const V& value) {
  operator[](key) = value;
}

template <typename K, typename V>
inline void Hash_map<K, V>::set(const K& key, const V& value) { insert(key, value); }

template <typename K, typename V>
inline void Hash_map<K, V>::remove(const K& key) {
  auto index = seek(key);
  if (marks_[index] == live) {
    marks_[index] = dead;
    size_--;
  }
}

template <typename K, typename V>
inline bool Hash_map<K, V>::contains(const K& key) const {
  return marks_[seek(key)] == live;
}

template <typename K, typename V>
inline bool Hash_map<K, V>::operator==(const Hash_map<K, V>& rhs) const {
  if (this == &rhs) {
    return true;
  }
  if (size_ != rhs.size_) {
    return false;
  }
  for (int i = 0; i < capacity_; i++) {
    if (!rhs.contains(keys_[i])) {
      return false;
    }
    if (values_[i] != rhs[keys_[i]]) {
      return false;
    }
  }
  return true;
}

template <typename K, typename V>
inline size_t Hash_map<K, V>::size() const { return size_; }

template <typename K, typename V>
inline size_t Hash_map<K, V>::capacity() const { return capacity_; }

template <typename K, typename V>
inline double Hash_map<K, V>::current_load_factor() const {
  return (capacity_ == 0) ? 0 : (static_cast<double>(size_) / capacity_);
}

template <typename K, typename V>
inline double Hash_map<K, V>::max_load_factor() const { return max_load_factor_; }

template <typename K, typename V>
inline void Hash_map<K, V>::set_max_load_factor(double x) {
  max_load_factor_ = normalize_maxlf(x);
  threshold_size_ = max_load_factor_ * capacity_;
  if (size_ >= threshold_size_) {
    increase_capacity();
  }
}

} // namespace dsa