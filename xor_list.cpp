#include <iostream>
#include <memory>
#include <list>
#include <vector>
#include <iterator>

namespace allocator_utility {
    struct Chunk {
        static const size_t kDefaultSize;
        unsigned char* begin, * next;
        size_t size;
        Chunk* previous;

        Chunk()
                : begin(nullptr)
                , next(nullptr)
                , size(0)
                , previous(nullptr)
        {}

        explicit Chunk(size_t _size, Chunk* previous = nullptr)
                : begin(reinterpret_cast<unsigned char*>(malloc(_size)))
                , next(begin)
                , size(_size)
                , previous(previous)
        {}

        ~Chunk() {
            free(begin);
        }

        size_t left() {
            return begin + size - next;
        }
    };

    const size_t Chunk::kDefaultSize = 1024;
}

template<typename T>
class StackAllocator {
  private:
    typedef allocator_utility::Chunk Chunk;
    Chunk* current_chunk_;

  public:
    typedef size_t         size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T*             pointer;
    typedef const T*       const_pointer;
    typedef T&             reference;
    typedef const T&       const_reference;
    typedef T              value_type;

    template<typename OtherT>
    struct rebind {
        typedef StackAllocator<OtherT> other;
    };

    StackAllocator() noexcept {
        current_chunk_ = new Chunk();
    }

    StackAllocator(const StackAllocator& sa)
            : StackAllocator()
    {}

    ~StackAllocator() noexcept {
        while (current_chunk_ != nullptr)
        {
            Chunk* previous = current_chunk_->previous;
            current_chunk_->~Chunk();
            free(current_chunk_);
            current_chunk_ = previous;
        }
    }

    pointer allocate(size_type n) {
        if (current_chunk_->left() < n * sizeof(T))
        {
            void* ptr = malloc(sizeof(Chunk));
            current_chunk_ = new(ptr) Chunk(std::max(n * sizeof(T), Chunk::kDefaultSize),
                                            current_chunk_);
        }
        unsigned char* ptr = current_chunk_->next;
        current_chunk_->next += n * sizeof(T);
        return reinterpret_cast<T*>(ptr);
    }

    void deallocate(pointer p, size_type n)
    {}

    template<typename Type, typename... Args>
    void construct(Type *ptr, Args&&... args) {
        ::new(reinterpret_cast<void*>(ptr)) Type(std::forward<Args>(args)...);
    }


    template<typename Type>
    void destroy(Type* ptr) {
        ptr->~Type();
    }
};

template<typename T>
struct Node {
    T value;
    size_t link_sum;

    template<typename... Type>
    explicit Node(Type&&... values)
            : value(std::forward<Type>(values)...)
            , link_sum(0)
    {}

    explicit Node(T&& value)
        : value(std::move(value))
        , link_sum(0)
    {}
};

template<typename T>
class Iterator : std::iterator<std::bidirectional_iterator_tag, T> {
  private:
    //right is the main one
    Node<T>* left, * right;

    template<typename Type, typename Allocator>
    friend class XorList;
  public:

    explicit Iterator(Node<T>* _right = nullptr)
            : left(nullptr)
            , right(_right)
    {}

    T& operator *() const {
        return right->value;
    }

    void operator ++() {
        if (right == nullptr) {
            return;
        }
        auto new_right =
                reinterpret_cast<Node<T>*>(right->link_sum ^
                                           reinterpret_cast<size_t>(left));
        left = right;
        right = new_right;
    }

    void operator --() {
        if (left == nullptr) {
            return;
        }
        auto new_left =
                reinterpret_cast<Node<T>*>(left->link_sum ^
                                           reinterpret_cast<size_t>(right));
        right = left;
        left = new_left;
    }

    bool operator == (const Iterator<T>& other) {
        return right == other.right;
    }

    bool operator != (const Iterator<T>& other) {
        return !(*this == other);
    }

};

template<typename T, class Allocator = std::allocator<T>>
class XorList {
  public:
    typedef Iterator<T> iterator;

  private:
    iterator begin_, back_;
    size_t size_;
    typename Allocator::template rebind<Node<T>>::other allocator_;
    size_t xor_pointers_(Node<T>* node1, Node<T>* node2) {
        return reinterpret_cast<size_t>(node1) ^ reinterpret_cast<size_t>(node2);
    }

    template<typename... Type>
    void push_front_(Type&&... values) {
        Node<T>* new_node = allocator_.allocate(1);
        allocator_.construct(new_node, std::forward<Type>(values)...);
        if (size() == 0) {
            begin_.right = new_node;
            back_ = begin_;
        } else {
            begin_.left = new_node;
            begin_.right->link_sum ^= reinterpret_cast<size_t>(new_node);
            new_node->link_sum = reinterpret_cast<size_t>(begin_.right);
            if (begin_ == back_) {
                back_.left = new_node;
            }
            --begin_;
        }
        ++size_;
    }

    template<typename... Type>
    void insert_before_(iterator it, Type&&... values) {
        if (it == begin_) {
            push_front_(std::forward<Type>(values)...);
        }
        else {
            Node<T>* new_node = allocator_.allocate(1);
            allocator_.construct(new_node, std::forward<Type>(values)...);
            new_node->link_sum = xor_pointers_(it.left, it.right);
            if (it.left != nullptr) {
                it.left->link_sum ^= xor_pointers_(it.right, new_node);
            }
            if (it.right != nullptr) {
                it.right->link_sum ^= xor_pointers_(it.left, new_node);
            }
            if (it == begin_) {
                begin_.right = new_node;
            }
            if (it == end()) {
                ++back_;
            } else if (it == back_) {
                back_.left = new_node;
            }
            ++size_;
        }
    }

  public:

    typedef T value_type;

    void clear() {
        while (size() != 0)
            pop_back();
    }

    iterator begin() const {
        return begin_;
    }

    iterator end() const {
        iterator it;
        it.left = back_.right;
        return it;
    }

    size_t size() const {
        return size_;
    }

    void push_back(const T& value) {
        insert_before_(end(), value);
    }

    void push_back(T&& value) {
        insert_before_(end(), std::move(value));
    }

    void push_front(const T& value) {
        push_front_(value);
    }

    void push_front(T&& value) {
        push_front_(std::move(value));
    }

    void pop_back() {
        Node<T>* old_node = back_.right;
        --back_;
        if (begin_.right == old_node) {
            --begin_;
        }
        if (back_.right != nullptr) {
            back_.right->link_sum ^= reinterpret_cast<size_t>(old_node);
        }
        allocator_.destroy(old_node);
        allocator_.deallocate(old_node, 1);
        --size_;
    }

    void pop_front() {
        Node<T>* old_node = begin_.right;
        ++begin_;
        if (begin_.right != nullptr) {
            begin_.right->link_sum ^= reinterpret_cast<size_t>(old_node);
        }
        begin_.left = nullptr;
        if (back_.right == old_node) {
            ++back_;
        }
        if (back_.left == old_node) {
            back_.left = nullptr;
        }
        allocator_.destroy(old_node);
        allocator_.deallocate(old_node, 1);
        --size_;
    }

    iterator insert_before(iterator it, const T& value) {
        insert_before_(it, value);
    }

    iterator insert_before(iterator it, T&& value) {
        insert_before_(it, std::move(value));
    }

    iterator insert_after(iterator it, const T& value) {
        ++it;
        insert_before_(it, value);
    }

    iterator insert_after(iterator it, T&& value) {
        ++it;
        insert_before_(it, std::move(value));
    }

    void erase(iterator it) {
        if (it == begin_) {
            pop_front();
        }
        else if (it == back_) {
            pop_back();
        }
        else {
            iterator next_it = it;
            ++next_it;
            Node<T>* next_right = next_it.right;
            if (it.left != nullptr) {
                it.left->link_sum ^= xor_pointers_(it.right, next_right);
            }
            if (next_right != nullptr) {
                next_right->link_sum ^= xor_pointers_(it.left, it.right);
            }
            allocator_.destroy(it.right);
            allocator_.deallocate(it.right, 1);
            --size_;
        }
    }

    explicit XorList(size_t count, const T& value = T(), const Allocator& alloc = Allocator())
            : allocator_(alloc)
            , size_(0) {
        while (count--) {
            insert_after(back_, value);
        }
    }

    explicit XorList(const Allocator& alloc = Allocator())
            : begin_(Iterator<T>())
            , back_(Iterator<T>())
            , size_(0)
            , allocator_(typename Allocator::template rebind<Node<T>>::other())
    {}

    XorList(const XorList<T>& other)
            : size_(0)
    {
        for (auto it = other.begin(); it != other.end(); ++it) {
            push_back(*it);
        }
    }

    XorList(XorList&& other) noexcept
            : begin_(other.begin_)
            , back_(other.back_)
            , size_(other.size_)
            , allocator_(other.allocator_)
    {
        other.begin_ = iterator();
        other.back_ = iterator();
        other.size_ = 0;
    }

    ~XorList() {
        clear();
    }

    XorList& operator = (const XorList& other) {
        clear();
        for (auto it = other.begin(); it != other.end(); ++it) {
            push_back(*it);
        }
    }

    XorList& operator = (XorList&& other) noexcept {
        clear();
        begin_ = other.begin_;
        other.begin_ = iterator();
        back_ = other.back_;
        other.back_ = iterator();
        size_ = other.size_;
        other.size_ = 0;

    }
};

std::pair<double, double> testStackAllocator(size_t n) {
    std::pair<double, double> time_taken;

    std::list<int, StackAllocator<int>> stack_list;
    clock_t stack_start = clock();
    for (size_t i = 0; i < n; ++i) {
        if (stack_list.empty() || rand() % 2) {
            stack_list.push_back(i);
        }
        else {
            stack_list.erase(stack_list.begin());
        }
    }
    time_taken.first = static_cast<double>(clock() - stack_start) / CLOCKS_PER_SEC;

    std::list<int, std::allocator<int>> std_list;
    clock_t std_start = clock();
    for (size_t i = 0; i < n; ++i) {
        if (std_list.empty() || rand() % 2) {
            std_list.push_back(i);
        }
        else {
            std_list.erase(std_list.begin());
        }
    }
    time_taken.second = static_cast<double>(clock() - std_start) / CLOCKS_PER_SEC;

    return time_taken;
}

std::pair<double, double> testXorList(size_t n)
{
    std::pair<double, double> time_taken;

    XorList<int, StackAllocator<int>> stack_list;
    clock_t stack_start = clock();
    for (size_t i = 0; i < n; ++i) {
        if (stack_list.size() == 0 || rand() % 2) {
            stack_list.push_back(i);
        }
        else {
            stack_list.erase(stack_list.begin());
        }
    }
    time_taken.first = static_cast<double>(clock() - stack_start) / CLOCKS_PER_SEC;

    XorList<int, std::allocator<int>> std_list;
    clock_t std_start = clock();
    for (size_t i = 0; i < n; ++i) {
        if (std_list.size() == 0 || rand() % 2) {
            std_list.push_back(i);
        }
        else {
            std_list.erase(std_list.begin());
        }
    }
    time_taken.second = static_cast<double>(clock() - std_start) / CLOCKS_PER_SEC;

    return time_taken;
};

int main() {
    auto t = testXorList(10000000);
    std::cout << t.first << " " << t.second << " " << t.second / t.first;
}