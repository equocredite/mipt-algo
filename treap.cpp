#include <iostream>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include <limits>
#include <random>
#include <ctime>

class Treap
{
private:

    static const long long INF_VALUE = static_cast<long long>(1e14);
    static const size_t INF_INDEX = static_cast<size_t>(1e9);

    static size_t get_random()
    {
        static std::mt19937 random_generator(time(0));
        static std::uniform_int_distribution<size_t> distribute(0, INF_INDEX);
        return distribute(random_generator);
    }

    struct Node
    {
        long long value, bound[2], border[2], paint, add, sum;
        size_t priority, size;
        bool reversed, inversion[2];
        Node *child[2];

        explicit Node(long long);
    };
    typedef Node *Link;

    Link root_;

    static bool compare(const long long&, const long long&, size_t);
    static long long min_max(const long long&, const long long&, size_t);
    static void create(Link&, const long long&);
    static void remove(Link&);
    static size_t subtree_size_(Link);
    static long long subtree_sum_(Link);
    static long long subtree_bound_(Link, size_t);
    static bool has_inversion_(Link, size_t);
    static void subtree_reverse_(Link&);
    static void subtree_paint_(Link&, const long long&);
    static void subtree_add_(Link&, const long long&);
    static void update_size_(Link);
    static void update_sum_(Link);
    static void update_bound_(Link);
    static void update_inversions_(Link);
    static void update_border_(Link);
    static void update_(Link);
    static void push_reverse_(Link);
    static void push_paint_(Link);
    static void push_add_(Link);
    static void push_(Link);
    static void split_(Link, Link &, Link &, size_t, size_t);
    static void merge_(Link &, Link, Link);

    class SegmentSplitter
    {
    private:
        Link &root_, left_part, mid, right_part;
    public:
        SegmentSplitter(Link&, size_t, size_t);
        ~SegmentSplitter();
        Link &get();
    };

    static void print_(Link, std::ostream&);
    static size_t get_inversion_(Link, size_t, size_t);
    static size_t get_nearest_(Link, const long long&, size_t, size_t);
    void destruct(Link);

    template<typename ReturnType, typename Operation>
    ReturnType perform_operation(size_t, size_t, Operation operate);

public:

    enum QueryType
    {
        SUM = 1,
        INSERT = 2,
        REMOVE = 3,
        PAINT = 4,
        ADD = 5,
        NEXT_PERMUTATION = 6,
        PREV_PERMUTATION = 7
    };

    Treap();
    ~Treap();
    void insert(const long long&, size_t);
    void remove(size_t);
    void paint(const long long&, size_t, size_t);
    void add(const long long&, size_t, size_t);
    long long get_sum(size_t, size_t);
    void permute(size_t, size_t, QueryType);
    static void permute(Link & t, QueryType);
    void print(std::ostream&);
};

Treap::Node::Node(long long _value)
        : value(_value)
        , bound({_value, _value})
        , border({_value, _value})
        , paint(INF_VALUE)
        , add(0)
        , sum(_value)
        , priority(get_random())
        , size(1)
        , reversed(false)
        , inversion({false, false})
        , child({nullptr, nullptr})
{}

bool Treap::compare(const long long &a, const long long &b, size_t type)
{
    if (type) return a > b;
    return a < b;
}

long long Treap::min_max(const long long &a, const long long &b, size_t type)
{
    if (type) return std::max(a, b);
    return std::min(a, b);
}

void Treap::create(Link & t, const long long &value)
{
    t = new Node(value);
}

void Treap::remove(Link & t)
{
    delete t;
    t = nullptr;
}

size_t Treap::subtree_size_(Link t)
{
    return t != nullptr ? t->size : 0;
}

long long Treap::subtree_sum_(Link t)
{
    return t != nullptr ? t->sum : 0;
}

long long Treap::subtree_bound_(Link t, size_t type)
{
    return t != nullptr ? t->bound[type] : INF_VALUE * (type ? -1 : 1);
}

bool Treap::has_inversion_(Link t, size_t type)
{
    return t != nullptr ? t->inversion[type] : false;
}

void Treap::subtree_reverse_(Link &t)
{
    if (t == nullptr) return;
    t->reversed ^= 1;
    std::swap(t->inversion[0], t->inversion[1]);
    std::swap(t->child[0], t->child[1]);
    std::swap(t->border[0], t->border[1]);
}

void Treap::subtree_paint_(Link &t, const long long &x)
{
    if (t == nullptr) return;
    t->value = t->bound[0] = t->bound[1] = t->border[0] = t->border[1] = t->paint = x;
    t->add = t->inversion[0] = t->inversion[1] = 0;
    t->sum = (long long)(t->size) * x;
}

void Treap::subtree_add_(Link &t, const long long &x)
{
    if (t == nullptr) return;
    t->value += x;
    t->bound[0] += x;
    t->bound[1] += x;
    t->border[0] += x;
    t->border[1] += x;
    t->add += x;
    t->sum += (long long)(t->size) * x;
}

void Treap::update_size_(Link t)
{
    t->size = subtree_size_(t->child[0]) + subtree_size_(t->child[1]) + 1;
}

void Treap::update_sum_(Link t)
{
    t->sum = subtree_sum_(t->child[0]) + subtree_sum_(t->child[1]) + t->value;
}

void Treap::update_bound_(Link t)
{
    for (size_t type = 0; type < 2; ++type)
    {
        t->bound[type] = min_max(t->value, min_max(subtree_bound_(t->child[0], type),
                                                   subtree_bound_(t->child[1], type),
                                                   type), type);
    }
}

void Treap::update_inversions_(Link t)
{
    for (size_t type = 0; type < 2; ++type)
    {
        t->inversion[type] =
                has_inversion_(t->child[0], type) ||
                has_inversion_(t->child[1], type) ||
                compare(subtree_bound_(t->child[0], type),
                        min_max(t->value, subtree_bound_(t->child[1], !type), !type), type) ||
                compare(min_max(subtree_bound_(t->child[0], type), t->value, type),
                        subtree_bound_(t->child[1], !type), type);
    };
}

void Treap::update_border_(Link t)
{
    for (size_t i = 0; i < 2; ++i)
        t->border[i] = (t->child[i] != nullptr ? t->child[i]->border[i] : t->value);
}

void Treap::update_(Link t)
{
    if (t == nullptr) return;
    update_size_(t);
    update_sum_(t);
    update_bound_(t);
    update_inversions_(t);
    update_border_(t);
}

void Treap::push_reverse_(Link t)
{
    if (t->reversed)
    {
        for (Link son : {t->child[0], t->child[1]})
            subtree_reverse_(son);
        t->reversed = false;
    }
}

void Treap::push_paint_(Link t)
{
    if (t->paint != INF_VALUE)
    {
        for (Link son : {t->child[0], t->child[1]})
            subtree_paint_(son, t->paint);
        t->paint = INF_VALUE;
    }
}

void Treap::push_add_(Link t)
{
    if (t->add != 0)
    {
        for (Link son : {t->child[0], t->child[1]})
            subtree_add_(son, t->add);
        t->add = 0;
    }
}

void Treap::push_(Link t)
{
    if (t == nullptr) return;
    push_reverse_(t);
    push_paint_(t);
    push_add_(t);
}

void Treap::split_(Link t, Link &left, Link &right, size_t key, size_t cur_key = 0)
{
    push_(t);
    if (t == nullptr) left = right = nullptr;
    else if (key <= cur_key + subtree_size_(t->child[0]))
    {
        split_(t->child[0], left, t->child[0], key, cur_key);
        right = t;
    } else
    {
        split_(t->child[1], t->child[1], right, key, cur_key + subtree_size_(t->child[0]) + 1);
        left = t;
    }
    update_(t);
}

void Treap::merge_(Link &t, Link left, Link right)
{
    push_(left);
    push_(right);
    if (left == nullptr || right == nullptr)
        t = (left != nullptr ? left : right);
    else if (left->priority > right->priority)
    {
        merge_(left->child[1], left->child[1], right);
        t = left;
    } else
    {
        merge_(right->child[0], left, right->child[0]);
        t = right;
    }
    update_(t);
}

Treap::SegmentSplitter::SegmentSplitter(Link &t, size_t left, size_t right)
        : root_(t)
        , left_part(nullptr)
        , mid(nullptr)
        , right_part(nullptr)
{
    split_(root_, left_part, mid, left);
    split_(mid, mid, right_part, right - left + 1);
}

Treap::SegmentSplitter::~SegmentSplitter()
{
    merge_(left_part, left_part, mid);
    merge_(root_, left_part, right_part);
}

Treap::Link &Treap::SegmentSplitter::get()
{ return mid; }

void Treap::print_(Link t, std::ostream &out)
{
    if (t == nullptr) return;
    push_(t);
    print_(t->child[0], out);
    out << t->value << " ";
    print_(t->child[1], out);
}

size_t Treap::get_inversion_(Link t, size_t type, size_t cur_key = 0)
{
    push_(t);
    if (t == nullptr || t->inversion[type] == false)
        return INF_INDEX;
    if (t->child[1] != nullptr && t->child[1]->inversion[type])
        return get_inversion_(t->child[1], type, cur_key + subtree_size_(t->child[0]) + 1);
    if (t->child[1] != nullptr && compare(t->value, t->child[1]->border[0], type))
        return cur_key + subtree_size_(t->child[0]);
    if (t->child[0] != nullptr && compare(t->child[0]->border[1], t->value, type))
        return cur_key + subtree_size_(t->child[0]) - 1;
    return get_inversion_(t->child[0], type, cur_key);
}

size_t Treap::get_nearest_(Link t, const long long &val, size_t type, size_t cur_key = 0)
{
    push_(t);
    if (t->child[1] != nullptr && compare(val, t->child[1]->bound[!type], type))
        return get_nearest_(t->child[1], val, type, cur_key + subtree_size_(t->child[0]) + 1);
    if (compare(val, t->value, type))
        return cur_key + subtree_size_(t->child[0]);
    return get_nearest_(t->child[0], val, type, cur_key);
}

void Treap::destruct(Link t)
{
    if (t == nullptr) return;
    destruct(t->child[0]);
    destruct(t->child[1]);
    delete t;
}

template<typename ReturnType, typename Operation>
ReturnType Treap::perform_operation(size_t left, size_t right, Operation operate)
{ return operate(SegmentSplitter(root_, left, right).get()); }

Treap::Treap() : root_(nullptr) {}
Treap::~Treap() { destruct(root_); }

void Treap::insert(const long long &x, size_t pos)
{
    perform_operation<void>(pos, pos - 1, [x](Link &t)
    { create(t, x); });
}

void Treap::remove(size_t pos)
{
    perform_operation<void>(pos, pos, [](Link &t)
    { remove(t); });
}

long long Treap::get_sum(size_t left, size_t right)
{
    return perform_operation<long long>(left, right, [](Link t)
    { return subtree_sum_(t); });
}

void Treap::paint(const long long &x, size_t left, size_t right)
{
    perform_operation<void>(left, right, [x](Link &t)
    { subtree_paint_(t, x); });
}

void Treap::add(const long long &x, size_t left, size_t right)
{
    perform_operation<void>(left, right, [x](Link &t)
    { subtree_add_(t, x); });
}

void Treap::print(std::ostream &out)
{ print_(root_, out); out << "\n"; }

void Treap::permute(Link & t, QueryType permutation_type)
{
    size_t capture_key = (permutation_type == NEXT_PERMUTATION
                     ? get_inversion_(t, 0)
                     : get_inversion_(t, 1));
    if (capture_key == INF_INDEX)
    {
        subtree_reverse_(t);
        return;
    }
    Link t_left, capture, suffix;
    split_(t, t_left, capture, capture_key);
    split_(capture, capture, suffix, 1);
    size_t nearest_key = (permutation_type == NEXT_PERMUTATION
                       ? get_nearest_(suffix, capture->value, 0)
                       : get_nearest_(suffix, capture->value, 1));
    Link suffix_left, nearest, suffix_right;
    split_(suffix, suffix_left, nearest, nearest_key);
    split_(nearest, nearest, suffix_right, 1);
    merge_(t, t_left, nearest);
    merge_(suffix_left, suffix_left, capture);
    merge_(suffix, suffix_left, suffix_right);
    subtree_reverse_(suffix);
    merge_(t, t, suffix);
}

void Treap::permute(size_t left, size_t right, QueryType permutation_type)
{
    perform_operation<void>(left, right, [permutation_type](Link & t)
    { permute(t, permutation_type); });
}

namespace query_utility
{
    struct Query
    {
        Treap::QueryType type;
        virtual void make_virtual() const {}
    };

    struct SumPermutation : public Query //1, 6, 7
    {
        size_t left, right;
        void read(std::istream &in)
        {
            in >> left >> right;
        }
    };

    struct PaintAdd : public SumPermutation // 4, 5
    {
        long long x;
        void read(std::istream &in)
        {
            in >> x;
            SumPermutation::read(in);
        }
    };

    struct Remove : public Query // 3
    {
        size_t pos;
        void read(std::istream &in)
        {
            in >> pos;
        }
    };

    struct Insert : public Remove // 2
    {
        long long x;
        void read(std::istream &in)
        {
            in >> x;
            Remove::read(in);
        }
    };

    struct Data
    {
        std::vector<long long> elements;
        std::vector<Query*> query;
    };
}

std::vector<long long> read_elements(std::istream &in)
{
    size_t n;
    in >> n;
    std::vector<long long> answer(n);
    for (size_t i = 0; i < n; ++i)
        in >> answer[i];
    return answer;
}

std::vector<query_utility::Query*> read_queries(std::istream &in)
{
    size_t q;
    in >> q;
    std::vector<query_utility::Query*> query(q);
    for (size_t i = 0; i < q; ++i)
    {
        size_t type_id;
        in >> type_id;
        Treap::QueryType type = static_cast<Treap::QueryType>(type_id);
        if (type == Treap::SUM ||
            type == Treap::NEXT_PERMUTATION ||
            type == Treap::PREV_PERMUTATION)
        {
            query[i] = new query_utility::SumPermutation;
            dynamic_cast<query_utility::SumPermutation*>(query[i])->read(in);
        }
        if (type == Treap::PAINT || type == Treap::ADD)
        {
            query[i] = new query_utility::PaintAdd;
            dynamic_cast<query_utility::PaintAdd*>(query[i])->read(in);
        }
        if (type == Treap::REMOVE)
        {
            query[i] = new query_utility::Remove;
            dynamic_cast<query_utility::Remove*>(query[i])->read(in);
        }
        if (type == Treap::INSERT)
        {
            query[i] = new query_utility::Insert;
            dynamic_cast<query_utility::Insert*>(query[i])->read(in);
        }
        query[i]->type = type;
    }
    return query;
}

query_utility::Data read_data(std::istream &in)
{
    query_utility::Data answer = { read_elements(in), read_queries(in) };
    return answer;
}

std::vector<long long> process_data(Treap &t, const query_utility::Data &data)
{
    for (size_t i = 0; i < data.elements.size(); ++i)
        t.insert(data.elements[i], i);

    std::vector<long long> answer;
    for (query_utility::Query* q : data.query)
    {
        if (q->type == Treap::SUM ||
            q->type == Treap::NEXT_PERMUTATION ||
            q->type == Treap::PREV_PERMUTATION)
        {
            query_utility::SumPermutation *ptr = dynamic_cast<query_utility::SumPermutation*>(q);
            if (ptr->type == Treap::SUM)
                answer.push_back(t.get_sum(ptr->left, ptr->right));
            else
                t.permute(ptr->left, ptr->right, ptr->type);
        } else if (q->type == Treap::PAINT || q->type == Treap::ADD)
        {
            query_utility::PaintAdd *ptr = dynamic_cast<query_utility::PaintAdd*>(q);
            if (ptr->type == Treap::PAINT)
                t.paint(ptr->x, ptr->left, ptr->right);
            else
                t.add(ptr->x, ptr->left, ptr->right);
        } else if (q->type == Treap::INSERT)
        {
            query_utility::Insert *ptr = dynamic_cast<query_utility::Insert*>(q);
            t.insert(ptr->x, ptr->pos);
        } else
        {
            t.remove(dynamic_cast<query_utility::Remove*>(q)->pos);
        }
    }
    return answer;
}

void print_query_answer(const std::vector<long long> &query_answer, std::ostream &out)
{
    for (auto i : query_answer)
        out << i << "\n";
}

void print_answer(Treap &t, const std::vector<long long> &answer, std::ostream &out)
{
    print_query_answer(answer, out);
    t.print(out);
}

void solution(std::istream &in, std::ostream &out)
{
    Treap t;
    query_utility::Data data = read_data(in);
    std::vector<long long> query_answer = process_data(t, data);
    print_answer(t, query_answer, out);
}

int main()
{
    std::ios_base::sync_with_stdio(false);
    std::istream &in = std::cin;
    std::ostream &out = std::cout;
    solution(in, out);
    return 0;
}
