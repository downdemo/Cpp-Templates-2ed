template <typename T>
class A {
 public:
  explicit A(std::size_t i) : v(new T[i]), n(i) { init(); }
  A(const A<T>& rhs) : v(new T[rhs.size()]), n(rhs.size()) { copy(rhs); }
  ~A() { delete[] v; }
  A<T>& operator=(const A<T>& rhs) {
    if (&rhs != this) copy(rhs);
    return *this;
  }
  std::size_t size() const { return n; }
  T& operator[](std::size_t i) { return v[i]; }
  const T& operator[](std::size_t i) const { return v[i]; }
  A<T>& operator+=(const A<T>& b);
  A<T>& operator*=(const A<T>& b);
  A<T>& operator*=(const T& s);

 protected:
  void init() {
    for (std::size_t i = 0; i < size(); ++i) {
      v[i] = T{};
    }
  }
  void copy(const A<T>& rhs) {
    assert(size() == rhs.size());
    for (std::size_t i = 0; i < size(); ++i) {
      v[i] = rhs.v[i];
    }
  }

 private:
  T* v;
  std::size_t n;
};

template <typename T>
A<T> operator+(const A<T>& a, const A<T>& b) {
  assert(a.size() == b.size());
  A<T> res(a.size());
  for (std::size_t i = 0; i < a.size(); ++i) {
    res[i] = a[i] + b[i];
  }
  return res;
}

template <typename T>
A<T> operator*(const A<T>& a, const A<T>& b) {
  assert(a.size() == b.size());
  A<T> res(a.size());
  for (std::size_t i = 0; i < a.size(); ++i) {
    res[i] = a[i] * b[i];
  }
  return res;
}

template <typename T>
A<T> operator*(const T& s, const A<T>& a) {
  A<T> res(a.size());
  for (std::size_t i = 0; i < a.size(); ++i) {
    res[i] = s * a[i];
  }
  return res;
}

template <class T>
A<T>& A<T>::operator+=(const A<T>& b) {
  assert(size() == rhs.size());
  for (std::size_t i = 0; i < size(); ++i) {
    (*this)[i] += b[i];
  }
  return *this;
}

template <class T>
A<T>& A<T>::operator*=(const A<T>& b) {
  assert(size() == rhs.size());
  for (std::size_t i = 0; i < size(); ++i) {
    (*this)[i] *= b[i];
  }
  return *this;
}

template <class T>
A<T>& A<T>::operator*=(const T& s) {
  for (std::size_t i = 0; i < size(); ++i) {
    (*this)[i] *= s;
  }
  return *this;
}

template <typename T>
class A_Scalar {
 public:
  constexpr A_Scalar(const T& v) : val(v) {}
  constexpr const T& operator[](std::size_t) const { return val; }
  constexpr std::size_t size() const { return 0; };

 private:
  const T& val;
};

template <typename T>
class A_Traits {
 public:
  using ExprRef = const T&;
};

template <typename T>
class A_Traits<A_Scalar<T>> {
 public:
  using ExprRef = A_Scalar<T>;
};

template <typename T, typename OP1, typename OP2>
class A_Add {
 public:
  A_Add(const OP1& a, const OP2& b) : op1(a), op2(b) {}
  T operator[](std::size_t i) const { return op1[i] + op2[i]; }
  std::size_t size() const {
    assert(op1.size() == 0 || op2.size() == 0 || op1.size() == op2.size());
    return op1.size() != 0 ? op1.size() : op2.size();
  }

 private:
  typename A_Traits<OP1>::ExprRef op1;
  typename A_Traits<OP2>::ExprRef op2;
};

template <typename T, typename OP1, typename OP2>
class A_Mult {
 public:
  A_Mult(const OP1& a, const OP2& b) : op1(a), op2(b) {}
  T operator[](std::size_t i) const { return op1[i] * op2[i]; }
  std::size_t size() const {
    assert(op1.size() == 0 || op2.size() == 0 || op1.size() == op2.size());
    return op1.size() != 0 ? op1.size() : op2.size();
  }

 private:
  typename A_Traits<OP1>::ExprRef op1;
  typename A_Traits<OP2>::ExprRef op2;
};

template <typename T, typename A1, typename A2>
class A_Subscript {
 public:
  A_Subscript(const A1& a, const A2& b) : a1(a), a2(b) {}
  T& operator[](std::size_t i) { return a1[a2[i]]; }
  decltype(auto) operator[](std::size_t i) const { return a1[a2[i]]; }
  std::size_t size() const { return a2.size(); }

 private:
  const A1& a1;
  const A2& a2;
};

template <typename T, typename Rep = A<T>>
class Array {
 private:
  Rep expr_rep;

 public:
  explicit Array(std::size_t i) : expr_rep(i) {}
  Array(const Rep& rb) : expr_rep(rb) {}

  Array& operator=(const Array& b) {
    assert(size() == b.size());
    for (std::size_t i = 0; i < b.size(); ++i) {
      expr_rep[i] = b[i];
    }
    return *this;
  }

  template <typename T2, typename Rep2>
  Array& operator=(const Array<T2, Rep2>& b) {
    assert(size() == b.size());
    for (std::size_t i = 0; i < b.size(); ++i) {
      expr_rep[i] = b[i];
    }
    return *this;
  }

  std::size_t size() const { return expr_rep.size(); }

  T& operator[](std::size_t i) {
    assert(i < size());
    return expr_rep[i];
  }

  decltype(auto) operator[](std::size_t i) const {
    assert(i < size());
    return expr_rep[i];
  }

  template <class T2, class Rep2>
  Array<T, A_Subscript<T, Rep, Rep2>> operator[](const Array<T2, Rep2>& b) {
    return Array<T, A_Subscript<T, Rep, Rep2>>(
        A_Subscript<T, Rep, Rep2>(*this, b));
  }

  template <class T2, class Rep2>
  decltype(auto) operator[](const Array<T2, Rep2>& b) const {
    return Array<T, A_Subscript<T, Rep, Rep2>>(
        A_Subscript<T, Rep, Rep2>(*this, b));
  }

  Rep& rep() { return expr_rep; }
  const Rep& rep() const { return expr_rep; }
};

template <typename T, typename R1, typename R2>
Array<T, A_Add<T, R1, R2>> operator+(const Array<T, R1>& a,
                                     const Array<T, R2>& b) {
  return Array<T, A_Add<T, R1, R2>>(A_Add<T, R1, R2>(a.rep(), b.rep()));
}

template <typename T, typename R1, typename R2>
Array<T, A_Mult<T, R1, R2>> operator*(const Array<T, R1>& a,
                                      const Array<T, R2>& b) {
  return Array<T, A_Mult<T, R1, R2>>(A_Mult<T, R1, R2>(a.rep(), b.rep()));
}

template <typename T, typename R2>
Array<T, A_Mult<T, A_Scalar<T>, R2>> operator*(const T& s,
                                               const Array<T, R2>& b) {
  return Array<T, A_Mult<T, A_Scalar<T>, R2>>(
      A_Mult<T, A_Scalar<T>, R2>(A_Scalar<T>(s), b.rep()));
}
