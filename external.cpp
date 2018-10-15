#include <string>
#include <fstream>
#include <iostream>
#include <cmath>
#include <vector>
#include <iterator>
#include <cstdio>
#include <algorithm>

// Interface

template<typename T>
void serialize(T value, std::ostream &out);

template<typename T>
T deserialize(std::istream &in);

template<typename T>
class SerializeIterator
{
public:
    typedef void value_type;
    typedef void difference_type;
    typedef void pointer;
    typedef void reference;
    typedef std::output_iterator_tag iterator_category;

    explicit SerializeIterator(std::ostream &stream);

    SerializeIterator& operator=(const T &value);
    SerializeIterator& operator*() { return *this; } // does nothing
    SerializeIterator& operator++() { return *this;} // does nothing
    SerializeIterator& operator++(int) { return *this; } // does nothing

private:
    std::ostream *stream_;
};

template<typename T>
class DeserializeIterator
{
public:
    typedef T value_type;
    typedef std::ptrdiff_t difference_type;
    typedef T *pointer;
    typedef T &reference;
    typedef std::input_iterator_tag iterator_category;

    DeserializeIterator();
    explicit DeserializeIterator(std::istream &stream);

    const T &operator*() const;
    const T &operator->() const;
    DeserializeIterator &operator++();

    DeserializeIterator operator++(int);

    bool isEnd() const;

private:
    std::istream *stream_;
    T value_;
    bool isEnd_;
};

template<typename T>
bool operator==(const DeserializeIterator<T>& first, const DeserializeIterator<T>& second);

template<typename T>
bool operator!=(const DeserializeIterator<T>& first, const DeserializeIterator<T>& second);

std::string tempFilename();

template<typename InputIter, typename OutputIter, typename Merger>
class ExternalAlgoritm
{
public:
    typedef typename std::iterator_traits<InputIter>::value_type value_type;

    ExternalAlgoritm(InputIter begin, InputIter end,
                     size_t size, size_t maxObjectsInMemory,
                     OutputIter out);

    ~ExternalAlgoritm();

    void run();

private:
    virtual void prepare(std::vector<value_type>& data) = 0;

    InputIter begin_;
    InputIter end_;
    size_t size_;
    size_t maxObjectsInMemory_;
    OutputIter out_;

    size_t countOfFiles_;
    std::fstream* fstreams_;
    std::vector<std::string> filenames_;
};

template<class T>
struct DeserializerCompare
{
    bool operator()(const DeserializeIterator<T>& first, const DeserializeIterator<T>& second);
};

template<class T>
class SortMerger
{
public:
    explicit SortMerger(const std::vector<DeserializeIterator<T> >& deserializers);

    bool hasNext() const;
    T next();

private:
    std::vector<DeserializeIterator<T> > deserializers_;
};

template<typename InputIter, typename OutputIter>
class ExternalSort : public ExternalAlgoritm<
        InputIter, OutputIter, SortMerger<typename std::iterator_traits<InputIter>::value_type> >
{
public:
    typedef ExternalAlgoritm<
            InputIter, OutputIter, SortMerger<typename std::iterator_traits<InputIter>::value_type> > Base;

    ExternalSort(InputIter begin, InputIter end,
                 size_t size, size_t maxObjectsInMemory,
                 OutputIter out);

private:
    virtual void prepare(std::vector<typename Base::value_type>& container);
};

template<class T>
class ReverseMerger
{
public:
    explicit ReverseMerger(const std::vector<DeserializeIterator<T> >& deserializers);

    bool hasNext() const;
    T next();

private:
    std::vector<DeserializeIterator<T> > deserializers_;
};

template<typename InputIter, typename OutputIter>
class ExternalReverse : public ExternalAlgoritm<
        InputIter, OutputIter, ReverseMerger<typename std::iterator_traits<InputIter>::value_type> >
{
public:
    typedef ExternalAlgoritm<
            InputIter, OutputIter, ReverseMerger<typename std::iterator_traits<InputIter>::value_type> > Base;

    ExternalReverse(InputIter begin, InputIter end,
                    size_t size, size_t maxObjectsInMemory,
                    OutputIter out);

private:
    virtual void prepare(std::vector<typename Base::value_type>& container);
};

// Implementation

int main()
{
    std::ifstream ifs("input.txt");
    std::ofstream ofs("output.txt");
    size_t type, count, max;
    ifs >> type >> count >> max;

    if (type == 1)
    {
        ExternalSort<
                std::istream_iterator<int>,
                std::ostream_iterator<int>
        > alg(
                std::istream_iterator<int>(ifs), std::istream_iterator<int>(),
                count, max,
                std::ostream_iterator<int>(ofs, " "));
        alg.run();
    } else
    {
        ExternalReverse<
                std::istream_iterator<int>,
                std::ostream_iterator<int>
        > alg(
                std::istream_iterator<int>(ifs), std::istream_iterator<int>(),
                count, max,
                std::ostream_iterator<int>(ofs, " "));
        alg.run();
    }
    return 0;
}

template<typename T>
void serialize(T value, std::ostream &out)
{
    out.write(reinterpret_cast<const char *>(&value), sizeof(T));
}

template<typename T>
T deserialize(std::istream &in)
{
    T value;
    in.read(reinterpret_cast<char *>(&value), sizeof(T));
    return value;
}

template<typename T>
SerializeIterator<T>::SerializeIterator(std::ostream &stream)
    : stream_(&stream)
{}

template<typename T>
SerializeIterator<T> &SerializeIterator<T>::operator =(const T &value)
{
    serialize<T>(value, *stream_);
    return *this;
}

template<typename T>
DeserializeIterator<T>::DeserializeIterator()
    : stream_(nullptr)
    , isEnd_(true)
{}

template<typename T>
DeserializeIterator<T>::DeserializeIterator(std::istream &stream)
    : stream_(&stream)
{
    value_ = deserialize<T>(*stream_);
    if (stream_->eof())
        isEnd_ = true;
}

template<typename T>
const T &DeserializeIterator<T>::operator *() const
{
    return value_;
}

template<typename T>
const T &DeserializeIterator<T>::operator ->() const
{
    return value_;
}

template<typename T>
DeserializeIterator<T> &DeserializeIterator<T>::operator ++()
{
    value_ = deserialize<T>(*stream_);
    if (stream_->eof())
        isEnd_ = true;
    return *this;
}

template<typename T>
DeserializeIterator<T> DeserializeIterator<T>::operator ++(int)
{
    DeserializeIterator answer = *this;
    value_ = deserialize<T>(*stream_);
    if (stream_->eof())
        isEnd_ = true;
    return answer;
}

template<typename T>
bool DeserializeIterator<T>::isEnd() const
{
    return isEnd_;
}

template<typename T>
bool operator == (const DeserializeIterator<T> &first, const DeserializeIterator<T> &second)
{
    return first.isEnd();
}

template<typename T>
bool operator != (const DeserializeIterator<T> &first, const DeserializeIterator<T> &second)
{
    return !(first == second);
}

std::string tempFilename()
{
    const std::string baseFilename = "tempFile";
    static int fileId = 0;
    return baseFilename + std::to_string(fileId++);
}

template<typename InputIter, typename OutputIter, typename Merger>
ExternalAlgoritm<InputIter, OutputIter, Merger>::ExternalAlgoritm(InputIter begin, InputIter end,
    size_t size, size_t maxObjectsInMemory, OutputIter out)
    : begin_(begin)
    , end_(end)
    , size_(size)
    , maxObjectsInMemory_(maxObjectsInMemory)
    , out_(out)
{
    countOfFiles_ = (size_ + maxObjectsInMemory_ - 1) / maxObjectsInMemory_; // rounding up
    fstreams_ = new std::fstream[countOfFiles_];
    filenames_.resize(countOfFiles_);
    for (size_t i = 0; i < countOfFiles_; ++i)
    {
        filenames_[i] = tempFilename();
        fstreams_[i].open(filenames_[i], std::fstream::out);
    }
}

template<typename InputIter, typename OutputIter, typename Merger>
ExternalAlgoritm<InputIter, OutputIter, Merger>::~ExternalAlgoritm()
{
    for (size_t i = 0; i < countOfFiles_; ++i)
    {
        fstreams_[i].close();
        remove(filenames_[i].c_str());
    }
    delete[] fstreams_;
}

template<typename InputIter, typename OutputIter, typename Merger>
void ExternalAlgoritm<InputIter, OutputIter, Merger>::run()
{
    InputIter iter = begin_;
    for (size_t i = 0; i < countOfFiles_; ++i)
    {
        std::vector<value_type> values;
        for (size_t j = 0; j < maxObjectsInMemory_ && iter != end_; ++j)
        {
            value_type newValue = *iter;
            values.push_back(newValue);
            ++iter;
        }
        prepare(values);
        SerializeIterator<value_type> fileWrite(fstreams_[i]);
        for (size_t j = 0; j < values.size(); ++j)
        {
            fileWrite = values[j];
        }
        fstreams_[i].close();
    }
    std::vector<DeserializeIterator<value_type> > deserializers(countOfFiles_);
    for (size_t i = 0; i < countOfFiles_; ++i)
    {
        fstreams_[i].open(filenames_[i], std::fstream::in);
        deserializers[i] = DeserializeIterator<value_type>(fstreams_[i]);
    }
    Merger merger(deserializers);
    while (merger.hasNext())
        out_ = merger.next();
}

template<class T>
bool DeserializerCompare<T>::operator ()(const DeserializeIterator<T> &first, const DeserializeIterator<T> &second)
{
    return *first > *second;
}

template<class T>
SortMerger<T>::SortMerger(const std::vector<DeserializeIterator<T> > &deserializers)
    : deserializers_(deserializers)
{
    std::make_heap(deserializers_.begin(), deserializers_.end(), DeserializerCompare<T>());
}

template<class T>
bool SortMerger<T>::hasNext() const
{
    return !deserializers_.empty();
}

template<class T>
T SortMerger<T>::next()
{
    T answer = *(deserializers_.front()++);
    std::pop_heap(deserializers_.begin(), deserializers_.end(), DeserializerCompare<T>());
    if (deserializers_.back().isEnd())
        deserializers_.pop_back();
    else
        std::push_heap(deserializers_.begin(), deserializers_.end(), DeserializerCompare<T>());
    return answer;
}

template<typename InputIter, typename OutputIter>
ExternalSort<InputIter, OutputIter>::ExternalSort(InputIter begin, InputIter end,
    size_t size, size_t maxObjectsInMemory, OutputIter out)
    : ExternalSort::ExternalAlgoritm(begin, end, size, maxObjectsInMemory, out)
{}

template<typename InputIter, typename OutputIter>
void ExternalSort<InputIter, OutputIter>::prepare(std::vector<typename Base::value_type> &container)
{
    std::make_heap(container.begin(), container.end());
    std::sort_heap(container.begin(), container.end());
}

template<class T>
ReverseMerger<T>::ReverseMerger(const std::vector<DeserializeIterator<T> > &deserializers)
    : deserializers_(deserializers)
{}

template<class T>
bool ReverseMerger<T>::hasNext() const
{
    return !deserializers_.empty();
}

template<class T>
T ReverseMerger<T>::next()
{
    T answer = *(deserializers_.back());
    ++deserializers_.back();
    if (deserializers_.back().isEnd())
        deserializers_.pop_back();
    return answer;

}

template<typename InputIter, typename OutputIter>
ExternalReverse<InputIter, OutputIter>::ExternalReverse(InputIter begin, InputIter end,
    size_t size, size_t maxObjectsInMemory, OutputIter out)
    : ExternalReverse::ExternalAlgoritm(begin, end, size, maxObjectsInMemory, out)
{}

template<typename InputIter, typename OutputIter>
void ExternalReverse<InputIter, OutputIter>::prepare(std::vector<typename Base::value_type> &container)
{
    std::reverse(container.begin(), container.end());
};
