// Implementation of Efficient Dynamic Array class DArray
#include <iostream>
#include <cstring>
#include <cassert>

#include "DArray.h"

using namespace std;

// Default constructor
DArray::DArray() {
    Init();
}

// Set an array with default values
DArray::DArray(int nSize, double dValue)
    : m_pData(new double[nSize]), m_nSize(nSize), m_nMax(nSize) {
    for (int i = 0; i < nSize; i++)
        m_pData[i] = dValue;
}

// Copy constructor
DArray::DArray(const DArray& arr)
    : m_pData(new double[arr.m_nSize]), m_nSize(arr.m_nSize), m_nMax(arr.m_nSize) {
    for (int i = 0; i < m_nSize; i++)
        m_pData[i] = arr.m_pData[i];
}

// Destructor
DArray::~DArray() {
    Free();
}

// Initialize the array
void DArray::Init() {
    m_pData = nullptr;
    m_nSize = 0;
    m_nMax = 0;
}

// Free the array
void DArray::Free() {
    delete[] m_pData;
    m_pData = nullptr;
    m_nSize = 0;
    m_nMax = 0;
}

// Reserve enough memory
void DArray::Reserve(int nSize) {
    if (m_nMax >= nSize)
        return;

    while (m_nMax < nSize)
        m_nMax = (m_nMax == 0) ? 1 : 2 * m_nMax;

    double* pData = new double[m_nMax];
    memcpy(pData, m_pData, m_nSize * sizeof(double));

    delete[] m_pData;
    m_pData = pData;
}

// Print the elements of the array
void DArray::Print() const {
    cout << "size = " << m_nSize << ":";
    for (int i = 0; i < m_nSize; i++)
        cout << " " << GetAt(i);
    cout << endl;
}

// Get the size of the array
int DArray::GetSize() const {
    return m_nSize;
}

// Set the size of the array
void DArray::SetSize(int nSize) {
    if (m_nSize == nSize)
        return;

    Reserve(nSize);

    for (int i = m_nSize; i < nSize; i++)
        m_pData[i] = 0.0;

    m_nSize = nSize;
}

// Get an element at an index
const double& DArray::GetAt(int nIndex) const {
    assert(nIndex >= 0 && nIndex < m_nSize);
    return m_pData[nIndex];
}

// Set the value of an element
void DArray::SetAt(int nIndex, double dValue) {
    assert(nIndex >= 0 && nIndex < m_nSize);
    m_pData[nIndex] = dValue;
}

// Overload operator []
double& DArray::operator[](int nIndex) {
    assert(nIndex >= 0 && nIndex < m_nSize);
    return m_pData[nIndex];
}

// Overload operator [] (const version)
const double& DArray::operator[](int nIndex) const {
    assert(nIndex >= 0 && nIndex < m_nSize);
    return m_pData[nIndex];
}

// Add a new element at the end of the array
void DArray::PushBack(double dValue) {
    Reserve(m_nSize + 1);
    m_pData[m_nSize] = dValue;
    m_nSize++;
}

// Delete an element at some index
void DArray::DeleteAt(int nIndex) {
    assert(nIndex >= 0 && nIndex < m_nSize);

    for (int i = nIndex + 1; i < m_nSize; i++)
        m_pData[i - 1] = m_pData[i];

    m_nSize--;
}

// Insert a new element at some index
void DArray::InsertAt(int nIndex, double dValue) {
    assert(nIndex >= 0 && nIndex <= m_nSize);

    Reserve(m_nSize + 1);

    for (int i = m_nSize; i > nIndex; i--)
        m_pData[i] = m_pData[i - 1];

    m_pData[nIndex] = dValue;
    m_nSize++;
}

// Overload operator =
DArray& DArray::operator=(const DArray& arr) {
    if (this == &arr)
        return *this;

    Reserve(arr.m_nSize);
    m_nSize = arr.m_nSize;
    memcpy(m_pData, arr.m_pData, arr.m_nSize * sizeof(double));

    return *this;
}
