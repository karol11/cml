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

  int size() { return d_ref_get_count(a); }
};

class str {
  friend class var;
  friend class dom;
  d_str *s;
  str(d_str *s) : s(s) {}

 public:
  operator const char *() { return d_c_str(s); }
  const char *c_str() { return d_c_str(s); }
};

class field_val_iterator {
  friend class var;
  friend class struc;
  d_field *f;
  d_struct *s;
  explicit field_val_iterator(d_struct *s, d_field *f) : f(f), s(s) {}

 public:
  bool operator==(const field_val_iterator &other) { return f == other.f; }
  bool operator!=(const field_val_iterator &other) { return f != other.f; }
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
  d_struct *s;
  struc(d_struct *s) : s(s) {}

 public:
  template <typename VAL>
  struc &operator()(const field &f, const VAL &val);

  template <typename VAL>
  struc &operator()(const char *f_name, const VAL &val);

  const char *get_name() { return d_ref_get_name(s); }
  void set_name(const char *name) { d_ref_set_name(s, name); }

  type type();

  field_val_iterator begin() {
    return field_val_iterator(s, d_enumerate_fields(d_ref_get_type(s)));
  }
  field_val_iterator end() { return field_val_iterator(s, nullptr); }
};

class field {
  friend class type;
  friend class struc;
  friend class var;
  friend class field_iterator;
  friend class type;
  friend class field_val_iterator;
  d_field *f;
  field(d_field *f) : f(f) {}

 public:
  const char *name();
  field &operator++() {
    f = d_next_field(f);
    return *this;
  }
  field operator*() { return *this; }
  bool operator==(const field &other) { return f == other.f; }
  bool operator!=(const field &other) { return f != other.f; }
};

class type {
  friend class dom;
  friend class struc;
  d_type *t;
  type(d_type *t) : t(t) {}

 public:
  field operator()(const char *field_name) {
    return field(d_add_field(t, field_name));
  }
  field operator>>(const char *field_name) {
    return field(d_lookup_field(t, field_name));
  }

  struc operator()() { return struc(d_make_struct(t)); }

  template <typename FIELD, typename VAL>
  struc operator()(const FIELD &field, const VAL &val) {
    return struc(d_make_struct(t))(field, val);
  }
  field begin() { return field(d_enumerate_fields(t)); }
  field end() { return field(nullptr); }
};

class array_iterator {
  friend class var;
  d_array *arr;
  int i;
  array_iterator(d_array *arr, int i) : arr(arr), i(i) {}

 public:
  bool operator==(const array_iterator &other) { return i == other.i; }
  bool operator!=(const array_iterator &other) { return i != other.i; }
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
  double operator()(double def) { return d_as_double(v, def); }
  double operator=(double val) {
    d_set_double(v, val);
    return val;
  }
  long long operator()(long long def) { return d_as_int(v, def); }
  long long operator=(long long val) {
    d_set_int(v, val);
    return val;
  }
  int operator()(int def) { return static_cast<int>(d_as_int(v, def)); }
  int operator=(int val) {
    d_set_int(v, val);
    return val;
  }
  bool operator()(bool def) { return d_as_bool(v, def ? 1 : 0) != 0; }
  bool operator=(bool val) {
    d_set_bool(v, val);
    return val;
  }
  struc operator*() { return struc(d_get_ref(v)); }
  struc operator=(struc val) {
    d_set_ref(v, val.s);
    return val;
  }
  arr operator=(arr val) {
    d_ref_set_array(v, val.a);
    return val;
  }
  const char *operator()(const char *def) { return d_as_str(v, def); }
  void operator=(str val) { d_set_str_ref(v, val.s); }
  void operator=(const char *str_should_be_created_by_dom) = delete;

  var operator[](const field &f) { return var(d_peek_field(v, f.f)); }
  var operator[](int i) { return var(d_at(v, i)); }
  int size() { return d_get_count(v); }
  array_iterator begin() { return array_iterator(d_ref_get_arr(v), 0); }
  array_iterator end() { return array_iterator(nullptr, d_get_count(v)); }
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

type struc::type() { return cml::type(d_ref_get_type(s)); }

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

#endif
