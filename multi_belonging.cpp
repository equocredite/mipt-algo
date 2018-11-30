#include <iostream>
#include <algorithm>
#include <vector>
#include <set>
#include <map>
#include <string>

struct Point {
    double x, y;

    Point() = default;

    Point(double _x, double _y)
        : x(_x)
        , y(_y)
    {}
};

double cross_product(const Point& a, const Point& b) {
    return a.x * b.y - b.x * a.y;
}

struct Data {
    std::vector<Point> vertices,
                       queries;
};

class MultiBelongingAlgorithm {
  public:

    enum PointPosition {
        OUTSIDE,
        INSIDE,
        BORDER
    };

  private:

    enum EdgePosition {
        POLYGON_ABOVE,
        POLYGON_BELOW
    };

    class Segment {
      private:

        Point begin_, end_;
        EdgePosition position_;

      public:

        Segment() = default;

        Segment(Point _begin, Point _end, EdgePosition _position)
                : begin_(_begin)
                , end_(_end)
                , position_(_position)
        {}

        explicit Segment(Point point)
                : Segment(point, point, POLYGON_ABOVE)
        {}

        bool vertical() const {
            return begin_.x == end_.x;
        }

        const Point& begin() const {
            return begin_;
        }

        const Point& end() const {
            return end_;
        }

        EdgePosition position() const {
            return position_;
        }

        double get_y(double x) const {
            if (vertical()) return begin_.y;
            return begin_.y + (end_.y - begin_.y) * (x - begin_.x) / (end_.x - begin_.x);
        }
    };

    enum VerticesOrder {
        COUNTER_CLOCKWISE,
        CLOCKWISE
    };

    struct SweeplineEvent {
        enum Type {
            OPEN_EDGE,
            CLOSE_EDGE,
            QUERY
        };

        double coordinate;
        Type type;
        size_t id;

        SweeplineEvent(double _coordinate, Type _type, size_t _id)
             : coordinate(_coordinate)
             , type(_type)
             , id(_id)
        {}

        SweeplineEvent(double _coordinate, Type _type)
            : SweeplineEvent(_coordinate, _type, SIZE_MAX)
        {}
    };

    struct SweeplineEventOpenQueryCloseOrder {
        bool operator () (const SweeplineEvent& first_event, const SweeplineEvent& second_event) {
            if (first_event.coordinate != second_event.coordinate) {
                return first_event.coordinate < second_event.coordinate;
            }
            static std::vector<SweeplineEvent::Type> order =
                    { SweeplineEvent::OPEN_EDGE, SweeplineEvent::QUERY, SweeplineEvent::CLOSE_EDGE };
            return std::find(order.begin(), order.end(), first_event.type) <
                   std::find(order.begin(), order.end(), second_event.type);
        }
    };

    struct SweeplineEventCloseOpenQueryOrder {
        bool operator () (const SweeplineEvent& first_event, const SweeplineEvent& second_event) {
            if (first_event.coordinate != second_event.coordinate) {
                return first_event.coordinate < second_event.coordinate;
            }
            static std::vector<SweeplineEvent::Type> order =
                    { SweeplineEvent::CLOSE_EDGE, SweeplineEvent::OPEN_EDGE, SweeplineEvent::QUERY };
            return std::find(order.begin(), order.end(), first_event.type) <
                   std::find(order.begin(), order.end(), second_event.type);
        }
    };

    std::vector<Point>         queries_,
                               vertices_;
    std::vector<Segment>       edges_;
    std::vector<PointPosition> results_;

    struct PointLexicographicallyLess {
        bool operator () (const Point& first_point, const Point& second_point) const {
            return std::make_pair(first_point.x, first_point.y) <
                   std::make_pair(second_point.x, second_point.y);
        }
    };

    struct SegmentRelativelyLower {
        bool operator () (const Segment& first_point, const Segment& second_point) const {
            double x_intersection_begin = std::max(first_point.begin().x, second_point.begin().x),
                   x_intersection_end   = std::min(first_point.end().x, second_point.end().x);
            Point first_border_ordinates(first_point.get_y(x_intersection_begin),
                                         first_point.get_y(x_intersection_end)),
                  second_border_ordinates(second_point.get_y(x_intersection_begin),
                                          second_point.get_y(x_intersection_end));
            return PointLexicographicallyLess()(first_border_ordinates, second_border_ordinates);
        }
    };

    void update_answer_(size_t query_id, PointPosition position) {
        results_[query_id] = std::max(results_[query_id], position);
    }

    void find_queries_coinciding_with_vertices_() {
        std::set<Point, PointLexicographicallyLess> vertices(vertices_.begin(), vertices_.end());
        for (size_t i = 0; i < queries_.size(); ++i) {
            if (vertices.count(queries_[i])) {
                update_answer_(i, BORDER);
            }
        }
    }

    double calc_oriented_area_() const {
        double oriented_area = 0;
        for (size_t i = 0; i < vertices_.size(); ++i) {
            oriented_area += cross_product(vertices_[i], vertices_[(i + 1) % vertices_.size()]);
        }
        return oriented_area;
    }

    VerticesOrder determine_vertices_order_() const {
        double oriented_area = calc_oriented_area_();
        if (oriented_area < 0) {
            return CLOCKWISE;
        } else {
            return COUNTER_CLOCKWISE;
        }
    }

    void make_edges_() {
        VerticesOrder order = determine_vertices_order_();
        edges_.resize(vertices_.size());
        for (size_t i = 0; i < vertices_.size(); ++i) {
            Point left  = vertices_[i],
                  right = vertices_[(i + 1) % vertices_.size()];
            bool left_to_right = PointLexicographicallyLess()(left, right);
            EdgePosition position = left_to_right ^ (order == CLOCKWISE) ? POLYGON_ABOVE : POLYGON_BELOW;
            if (!left_to_right) {
                std::swap(left, right);
            }
            edges_[i] = Segment(left, right, position);
        }
    }

    void vertical_sweepline_(const std::vector<Segment>& edges,
                             const std::vector<size_t>& query_ids
    ) {
        std::vector<SweeplineEvent> events;
        for (const Segment& edge : edges) {
            events.emplace_back(std::min(edge.begin().y, edge.end().y), SweeplineEvent::OPEN_EDGE);
            events.emplace_back(std::max(edge.begin().y, edge.end().y), SweeplineEvent::CLOSE_EDGE);
        }
        for (size_t id : query_ids) {
            events.emplace_back(queries_[id].y, SweeplineEvent::QUERY, id);
        }
        sort(events.begin(), events.end(), SweeplineEventOpenQueryCloseOrder());
        size_t opened_cnt = 0;
        for (const SweeplineEvent& event : events) {
            switch(event.type) {
                case SweeplineEvent::OPEN_EDGE:
                    ++opened_cnt;
                    break;
                case SweeplineEvent::CLOSE_EDGE:
                    --opened_cnt;
                    break;
                case SweeplineEvent::QUERY:
                    if (opened_cnt > 0) {
                        update_answer_(event.id, BORDER);
                    }
            }
        }
    }

    void find_queries_lying_on_vertical_edges_() {
        std::map<double, std::vector<Segment>> vertical_edges;
        for (const Segment& edge : edges_) {
            if (edge.vertical()) {
                vertical_edges[edge.begin().x].push_back(edge);
            }
        }

        std::map<double, std::vector<size_t>> query_abscissae;
        for (size_t i = 0; i < queries_.size(); ++i) {
            query_abscissae[queries_[i].x].push_back(i);
        }

        for (auto& i: vertical_edges) {
            vertical_sweepline_(i.second, query_abscissae[i.first]);
        }
    }

    void find_corner_cases_() {
        find_queries_coinciding_with_vertices_();
        find_queries_lying_on_vertical_edges_();
    }

  public:

    MultiBelongingAlgorithm() = default;

    void load_data(const Data& data) {
        vertices_ = data.vertices;
        queries_  = data.queries;
    }

    static std::string get_string_result(PointPosition position) {
        switch (position) {
            case INSIDE:
                return "INSIDE\n";
            case OUTSIDE:
                return "OUTSIDE\n";
            case BORDER:
                return "BORDER\n";
        }
    }

    void run() {
        results_.assign(queries_.size(), OUTSIDE);
        make_edges_();
        find_corner_cases_();

        std::vector<SweeplineEvent> events;
        for (size_t i = 0; i < edges_.size(); ++i) if (!edges_[i].vertical()) {
            events.emplace_back(edges_[i].begin().x, SweeplineEvent::OPEN_EDGE, i);
            events.emplace_back(edges_[i].end().x, SweeplineEvent::CLOSE_EDGE, i);
        }
        for (size_t i = 0; i < queries_.size(); ++i) {
            events.emplace_back(queries_[i].x, SweeplineEvent::QUERY, i);
        }

        sort(events.begin(), events.end(), SweeplineEventCloseOpenQueryOrder());
        std::multiset<Segment, SegmentRelativelyLower> opened_edges;

        for (const SweeplineEvent& event : events) {
            switch (event.type) {
                case SweeplineEvent::OPEN_EDGE:
                    opened_edges.insert(edges_[event.id]);
                    break;
                case SweeplineEvent::CLOSE_EDGE:
                    opened_edges.erase(opened_edges.find(edges_[event.id]));
                    break;
                case SweeplineEvent::QUERY:
                    Segment query(queries_[event.id]);
                    auto it = opened_edges.lower_bound(query);
                    if (it != opened_edges.end() && it->get_y(queries_[event.id].x) == queries_[event.id].y) {
                        update_answer_(event.id, BORDER);
                    }
                    if (it != opened_edges.begin()) {
                        --it;
                        if (it->position() == POLYGON_ABOVE) {
                            update_answer_(event.id, INSIDE);
                        }
                    }
            }
        }
    }

    std::vector<PointPosition> get_results() const {
        return results_;
    }
};

Data read_data(std::istream& in) {
    Data data;
    size_t vertices_cnt,
           queries_cnt;
    in >> vertices_cnt;
    data.vertices.resize(vertices_cnt);
    for (auto& vertex : data.vertices) {
        in >> vertex.x >> vertex.y;
    }
    in >> queries_cnt;
    data.queries.resize(queries_cnt);
    for (auto& query : data.queries) {
        in >> query.x >> query.y;
    }
    return data;
}

std::vector<Data> read_tests(std::istream& in) {
    size_t test_cnt;
    in >> test_cnt;
    std::vector<Data> tests(test_cnt);
    for (auto& data : tests) {
        data = read_data(in);
    }
    return tests;
}

std::vector<std::vector<MultiBelongingAlgorithm::PointPosition>> run_tests(std::vector<Data> tests) {
    std::vector<std::vector<MultiBelongingAlgorithm::PointPosition>> results;
    for (auto& data : tests) {
        MultiBelongingAlgorithm algo;
        algo.load_data(data);
        algo.run();
        results.push_back(algo.get_results());
    }
    return results;
}

void write_result(std::vector<MultiBelongingAlgorithm::PointPosition> result, std::ostream& out) {
    for (auto point_result : result) {
        out << MultiBelongingAlgorithm::get_string_result(point_result);
    }
}

void write_results(std::vector<std::vector<MultiBelongingAlgorithm::PointPosition>> results, std::ostream& out) {
    for (auto& result : results) {
        write_result(result, out);
    }
}

void run(std::istream& in, std::ostream& out) {
    auto tests   = read_tests(in);
    auto results = run_tests(tests);
    write_results(results, out);
}

int main() {
    run(std::cin, std::cout);
}