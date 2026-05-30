# HW0: C++ Warmup 教学报告

## 1. 问题定义

本次热身练习通过 5 个递进式小练习，掌握 C++ 面向对象编程的核心概念：

1. **BasicDArray** — 动态数组基础（内存管理、类封装）
2. **EfficientDArray** — 内存效率优化（预分配策略）
3. **TemplateDArray** — 模板化（泛型编程）
4. **list_Polynomial** — STL list 实现多项式（链表操作）
5. **map_Polynomial** — STL map 实现多项式（关联容器）

---

## 2. 核心知识点

### 2.1 内存管理（new/delete）

```cpp
// 分配
double* p = new double[n];      // 动态数组
double* p = new double(3.14);   // 单个对象

// 释放
delete[] p;  // 数组必须用 delete[]
delete p;    // 单个对象用 delete
```

**关键**：`new` 和 `delete` 必须配对，否则内存泄漏。

### 2.2 类的三大特殊函数

| 函数 | 作用 | 何时调用 |
|------|------|----------|
| **构造函数** | 初始化对象 | `DArray a;` 或 `DArray a(10);` |
| **析构函数** | 清理资源 | 对象销毁时自动调用 |
| **拷贝构造函数** | 深拷贝 | `DArray b = a;` 或 `DArray b(a);` |

### 2.3 运算符重载

```cpp
// 赋值运算符
DArray& operator=(const DArray& arr) {
    if (this == &arr) return *this;  // 防止自赋值
    delete[] m_pData;                // 释放旧内存
    // 分配新内存并拷贝...
    return *this;
}

// 下标运算符
double& operator[](int nIndex) { return m_pData[nIndex]; }
```

### 2.4 预分配策略（EfficientDArray）

```cpp
void Reserve(int nSize) {
    if (m_nMax >= nSize) return;  // 空间够用
    
    // 容量翻倍增长
    while (m_nMax < nSize)
        m_nMax = (m_nMax == 0) ? 1 : 2 * m_nMax;
    
    // 分配新内存，拷贝数据
    double* pData = new double[m_nMax];
    memcpy(pData, m_pData, m_nSize * sizeof(double));
    delete[] m_pData;
    m_pData = pData;
}
```

**为什么翻倍**：保证 n 次 PushBack 的均摊时间复杂度为 O(1)。

### 2.5 模板（Template）

```cpp
template<class T>
class DArray {
    T* m_pData;
    // ...
};

// 使用时指定类型
DArray<double> a;
DArray<int> b;
DArray<char> c;
```

**模板实现必须放在 .h 或 .inl 文件中**，因为编译器需要在编译时看到完整实现。

### 2.6 STL 容器对比

| 容器 | 底层结构 | 随机访问 | 插入/删除 | 适用场景 |
|------|----------|----------|-----------|----------|
| **vector** | 动态数组 | O(1) | O(n) | 频繁随机访问 |
| **list** | 双向链表 | O(n) | O(1) | 频繁插入删除 |
| **map** | 红黑树 | O(log n) | O(log n) | 键值对查找 |

---

## 3. 实现要点

### 3.1 BasicDArray
- 每次 `PushBack`/`DeleteAt` 都重新分配内存
- 简单但低效（O(n) 每次操作）

### 3.2 EfficientDArray
- 增加 `m_nMax` 记录容量
- `Reserve()` 预分配，容量翻倍
- `PushBack` 均摊 O(1)

### 3.3 TemplateDArray
- 将 `double` 替换为模板参数 `T`
- 实现放在 `.inl` 文件中

### 3.4 PolynomialList vs PolynomialMap

**性能对比**（从运行结果看）：
- 小规模：List 构造 19ms，Map 构造 13ms
- 大规模：List 构造 45971ms，Map 构造 826ms

**结论**：`map` 在大规模数据下明显更快，因为 `list` 需要遍历查找插入位置。

---

## 4. 踩坑记录

1. **拷贝构造函数必须深拷贝**：否则多个对象共享同一内存，析构时 double free
2. **赋值运算符要检查自赋值**：`if (this == &arr) return *this;`
3. **模板类的实现文件**：必须用 `.inl` 或直接写在 `.h` 中
4. **`delete[]` vs `delete`**：数组必须用 `delete[]`

---

## 5. 延伸思考

- `std::vector` 就是你写的 `DArray` 的工业级版本
- `std::map` 底层是红黑树，保证 O(log n) 的插入/查找
- C++11 的 `emplace_back` 可以避免不必要的拷贝
- 移动语义（`std::move`）可以进一步优化性能
