#ifndef CML_CPP_H
#define CML_CPP_H

#include <iostream>
#include "cml_dom_reader.h"
#include "cml_dom_writer.h"
#include "dom.h"

namespace cml {
class dom;
class var;
class field;
class type;
class struc;
class str;
class arr;

class arr {
  friend class dom;
  friend class var;
  d_array *a;
  d_dom *d;
  arr(d_array *a, d_dom *d) : a(a), d(d) {}
 public:
  template <typename A>
  arr &operator+(const A &a);

  int size() const { return d_ref_get_count(a); }
};

class str {
  friend class var;
  friend class dom;
  d_str *s;
  str(d_str *s) : s(s) {}

 public:
  operator const char *() const { return d_c_str(s); }
  const char *c_str() const { return d_c_str(s); }
};

class field_val_iterator {
  friend class var;
  friend class struc;
  d_field *f;
  d_struct *s;
  explicit field_val_iterator(d_struct *s, d_field *f) : f(f), s(s) {}

 public:
  bool operator==(const field_val_iterator &other) const { return f == other.f; }
  bool operator!=(const field_val_iterator &other) const { return f != other.f; }
  field_val_iterator &operator++() {
    f = d_next_field(f);
    return *this;
  }
  std::pair<field, var> operator*();
};

class struc {
  friend class type;
  friend class var;
  friend class dom;
  friend std::hash<struc>;
  d_struct *s;
  struc(d_struct *s) : s(s) {}

 public:
  static struc null() { return struc(nullptr); }
  template <typename VAL>
  struc &operator()(const field &f, const VAL &val);

  template <typename VAL>
  struc &operator()(const char *f_name, const VAL &val);

  const char *get_name() const { return d_ref_get_name(s); }
  void set_name(const char *name) { d_ref_set_name(s, name); }

  type type() const;
  size_t get_tag() const { return d_ref_get_tag(s); }
  void set_tag(size_t val) { d_ref_set_tag(s, val); }

  field_val_iterator begin() {
    return field_val_iterator(s, d_enumerate_fields(d_ref_get_type(s)));
  }
  field_val_iterator end() { return field_val_iterator(s, nullptr); }
  bool operator== (const struc& b) const { return s == b.s; }
};

class field {
  friend class type;
  friend class struc;
  friend class var;
  friend class field_iterator;
  friend class type;
  friend class field_val_iterator;
  friend struct std::hash<field>;
  d_field *f;
  field(d_field *f) : f(f) {}

 public:
  const char *name() const { return d_field_name(f); }
  field &operator++() {
    f = d_next_field(f);
    return *this;
  }
  field operator*() { return *this; }
  bool operator==(const field &other) const { return f == other.f; }
  bool operator!=(const field &other) const { return f != other.f; }
};

class type {
  friend class dom;
  friend class struc;
  friend std::hash<type>;
  d_type *t;
  type(d_type *t) : t(t) {}

 public:
  const char *name() const { return d_type_name(t); }
  field operator()(const char *field_name) {
    return field(d_add_field(t, field_name));
  }
  field operator>>(const char *field_name) const {
    return field(d_lookup_field(t, field_name));
  }

  struc operator()() const { return struc(d_make_struct(t)); }

  template <typename FIELD, typename VAL>
  struc operator()(const FIELD &field, const VAL &val) const {
    return struc(d_make_struct(t))(field, val);
  }
  field begin() { return field(d_enumerate_fields(t)); }
  field end() { return field(nullptr); }
  bool operator==(const type &other) const { return t == other.t; }
  bool operator!=(const type &other) const { return t != other.t; }
};

class array_iterator {
  friend class var;
  d_array *arr;
  int i;
  array_iterator(d_array *arr, int i) : arr(arr), i(i) {}

 public:
  bool operator==(const array_iterator &other) const { return i == other.i; }
  bool operator!=(const array_iterator &other) const { return i != other.i; }
  array_iterator &operator++() {
    i++;
    return *this;
  }
  var operator*();
};

class var {
  friend class struc;
  friend class arr;
  friend class dom;
  friend class array_iterator;
  friend class field_val_iterator;
  d_var *v;
  var(d_var *v) : v(v) {}

 public:
  var() : v(0) {}
  double operator()(double def) const { return d_as_double(v, def); }
  double operator=(double val) {
    d_set_double(v, val);
    return val;
  }
  long long operator()(long long def) const { return d_as_int(v, def); }
  long long operator=(long long val) {
    d_set_int(v, val);
    return val;
  }
  int operator()(int def) const { return static_cast<int>(d_as_int(v, def)); }
  int operator=(int val) {
    d_set_int(v, val);
    return val;
  }
  bool operator()(bool def) const { return d_as_bool(v, def ? 1 : 0) != 0; }
  bool operator=(bool val) {
    d_set_bool(v, val);
    return val;
  }
  struc operator*() const { return struc(d_get_ref(v)); }
  struc operator=(struc val) {
    d_set_ref(v, val.s);
    return val;
  }
  arr operator=(arr val) {
    d_ref_set_array(v, val.a);
    return val;
  }
  const char *operator()(const char *def) const { return d_as_str(v, def); }
  void operator=(str val) { d_set_str_ref(v, val.s); }
  void operator=(const char *str_should_be_created_by_dom) = delete;

  var operator[](const field &f) { return var(d_peek_field(v, f.f)); }
  var operator[](int i) { return var(d_at(v, i)); }
  int size() { return d_get_count(v); }
  array_iterator begin() { return array_iterator(d_ref_get_arr(v), 0); }
  array_iterator end() { return array_iterator(nullptr, d_get_count(v)); }
  void operator=(const var&) = delete;
  int kind() { return d_kind(v); }
};

template <typename VAL>
struc &struc::operator()(const field &f, const VAL &val) {
  var(d_ref_get_field(s, f.f)) = val;
  return *this;
}

template <typename VAL>
struc &struc::operator()(const char *f_name, const VAL &val) {
  var(d_ref_get_field(s, d_add_field(d_ref_get_type(s), f_name))) = val;
  return *this;
}

type struc::type() const { return cml::type(d_ref_get_type(s)); }

var array_iterator::operator*() { return var(d_ref_at(arr, i)); }

std::pair<field, var> field_val_iterator::operator*() {
  return std::make_pair<field, var>(field(f), var(d_ref_get_field(s, f)));
}

int os_put_c(void *context, char ch) {
  static_cast<std::ostream *>(context)->put(ch);
  return 1;
}
int is_get_c(void *context) {
  return static_cast<std::istream *>(context)->get();
}

struct err_handler {
  virtual void on_error(const char *error, int line, int char_pos) = 0;
  virtual ~err_handler() {}
};

void on_err_proc(void *context, const char *error, int line_num, int char_pos) {
  if (context)
    static_cast<err_handler *>(context)->on_error(error, line_num, char_pos);
}

class dom {
  friend class arr;
  d_dom *d;

 public:
  dom() : d(d_alloc_dom()) {}
  dom(std::istream &&s, err_handler *on_error = 0)
      : d(cml_read(is_get_c, &s, on_err_proc, on_error)) {}
  ~dom() { d_dispose_dom(d); }

  type operator()(const char *type_name) {
    return type(d_add_type(d, type_name));
  }
  type operator>>(const char *type_name) {
    return type(d_lookup_type(d, type_name));
  }

  template <typename A>
  A operator=(const A &a) {
    return var(d_root(d)) = a;
  }

  var operator[](const field &f) { return var(d_root(d))[f]; }
  var operator[](int i) { return var(d_root(d))[i]; }
  template <typename DEF>
  var root(const DEF &def) {
    return var(d_root(d))(def);
  }
  struc operator*() { return *var(d_root(d)); }
  array_iterator begin() { return var(d_root(d)).begin(); }
  array_iterator end() { return var(d_root(d)).end(); }

  struc operator[](const char *name) { return struc(d_get_named(d, name)); }

  cml::str str(const char *val) { return cml::str(d_make_str(d, val)); }
  cml::arr arr(int size = 0) { return cml::arr(d_make_array(d, size), d); }

  operator var () { return var(d_root(d)); }
  void untag() { d_untag(d_root(d)); }
  void write(std::ostream &s) { cml_write(d, os_put_c, &s); }

 private:
  dom(const dom &) = delete;
  void operator=(const dom &) = delete;
};

template <typename VAL>
inline arr &arr::operator+(const VAL &val) {
  var(d_ref_at(a, d_ref_insert(a, d, size(), 1))) = val;
  return *this;
}

}  // namespace cml

namespace std {

template<>
struct hash<cml::field> {
  typedef cml::field argument_type;
  typedef std::size_t result_type;
  result_type operator()(argument_type const& val) const noexcept {
    return std::hash<void*>{}(((void*)val.f));
  }
};

template<>
struct hash<cml::type> {
  typedef cml::type argument_type;
  typedef std::size_t result_type;
  result_type operator()(argument_type const& val) const noexcept {
    return std::hash<void*>{}(((void*)val.t));
  }
};

template<>
struct hash<cml::struc> {
  typedef cml::struc argument_type;
  typedef std::size_t result_type;
  result_type operator()(argument_type const& val) const noexcept {
    return std::hash<void*>{}(((void*)val.s));
  }
};

}  // namespace std

#endif
