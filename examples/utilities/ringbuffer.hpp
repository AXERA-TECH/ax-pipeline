#pragma once
#include "vector"

template <typename T>
class SimpleRingBuffer
{
private:
    std::vector<T> mBuffer;
    int nCurIdx = 0;

public:
    SimpleRingBuffer() {}
    SimpleRingBuffer(int size)
    {
        this->resize(size);
    }
    ~SimpleRingBuffer()
    {
        // mBuffer.swap(std::vector<T>());
    }
    void resize(int size)
    {
        mBuffer.resize(size);
    }

    T &next()
    {
        nCurIdx = (nCurIdx + 1) % mBuffer.size();
        return get(nCurIdx++ % mBuffer.size());
    }

    T &get(int idx)
    {
        return mBuffer[idx];
    }
    
    size_t size()
    {
        return mBuffer.size();
    }
};
