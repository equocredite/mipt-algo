#include <iostream>
#include <vector>
#include <list>
#include <queue>
#include <set>
#include <cassert>

class ResidualNetwork {
  public:

    constexpr static int INF = static_cast<int>(1e6);
    constexpr static size_t NONE = static_cast<size_t>(1e6);

    class Edge {
      private:

        size_t from_,
               to_;
        int    capacity_,
               flow_;

      public:

        Edge(size_t _from, size_t _to, int _capacity)
                : from_(_from)
                , to_(_to)
                , capacity_(_capacity)
                , flow_(0)
        {}

        size_t from() const { return from_; }
        size_t to() const { return to_; }
        int capacity() const { return capacity_; }
        int residual_capacity() const { return capacity_ - flow_; }
        int& flow() { return flow_; }
        bool saturated() { return residual_capacity() == 0; }
    };

    template<bool BackEdge = false>
    class EdgeIterator {
      private:

        std::vector<Edge>*   edges_;
        std::vector<size_t>* next_edge_;
        size_t               current_edge_id_;

      public:

        EdgeIterator() = default;

        EdgeIterator(std::vector<Edge>* _edges,
                     std::vector<size_t>* _next_edge,
                     size_t _start_id)
                : edges_(_edges)
                , next_edge_(_next_edge)
                , current_edge_id_(_start_id) {
            if (valid() && operator*().saturated()) {
                operator++();
            }
        }

        Edge& operator *() {
            return edges_->at(current_edge_id_ ^ BackEdge);
        }

        Edge& back_edge() {
            return edges_->at(current_edge_id_ ^ BackEdge ^ 1);
        }

        bool valid() {
            return (current_edge_id_ != NONE);
        }

        void operator ++() {
            if (valid()) current_edge_id_ = next_edge_->at(current_edge_id_);
            while (valid() && operator*().saturated()) {
                current_edge_id_ = next_edge_->at(current_edge_id_);
            }
        }

        size_t go_through() {
            return edges_->at(current_edge_id_).to();
        }
    };

  private:

    std::vector<Edge>   edges_;
    std::vector<size_t> next_edge_,
                        head_;

    size_t              vertices_cnt_,
                        source_,
                        sink_;

  public:

    void discard_flow() {
        for (auto& edge : edges_) {
            edge.flow() = 0;
        }
    }

    template<bool BackEdge = false>
    EdgeIterator<BackEdge> make_iterator(size_t vertex_id) {
        return EdgeIterator<BackEdge>(&edges_, &next_edge_, head_[vertex_id]);
    }

    ResidualNetwork() = default;
    ResidualNetwork(size_t _n, size_t _source, size_t _sink)
            : vertices_cnt_(_n)
            , source_(_source)
            , sink_(_sink) {
        head_.assign(vertices_cnt_, NONE);
    }

    size_t vertices_cnt() const { return vertices_cnt_; }
    size_t source() const { return source_; }
    size_t sink() const { return sink_; }

    void add_residual_edge(size_t from, size_t to, int capacity) {
        edges_.emplace_back(from, to, capacity);
        next_edge_.emplace_back(head_[from]);
        head_[from] = edges_.size() - 1;
    }

    void add_edge(size_t from, size_t to, int capacity, bool directed = true) {
        add_residual_edge(from, to, capacity);
        add_residual_edge(to, from, directed ? 0 : capacity);
    }
};

class MaxFlowAlgorithm {
  protected:

    ResidualNetwork* network_;
    int              flow_;

  public:

    MaxFlowAlgorithm() = default;

    void discard() { network_ = nullptr; }

    void init(ResidualNetwork* _network) {
        network_ = _network;
        flow_ = 0;
    }

    int flow() const { return flow_; }

    virtual void run() = 0;
};

class Malhotra : public MaxFlowAlgorithm {
  private:

    enum DIRECTION {
        LEFT = -1,
        RIGHT = 1
    };

    template<DIRECTION direction>
    class LayeredEdgeIterator {
      private:

        ResidualNetwork::EdgeIterator<direction == LEFT> current_edge_;
        std::vector<int>* layer_id_;

        int delta_(size_t vertex1_id, size_t vertex2_id) const {
            return layer_id_->at(vertex2_id) - layer_id_->at(vertex1_id);
        }

        bool correct_delta_() {
            return delta_((*current_edge_).from(), (*current_edge_).to()) == 1;
        }

      public:

        LayeredEdgeIterator() = default;

        LayeredEdgeIterator(ResidualNetwork* _network,
                            size_t _vertex_id,
                            std::vector<int>* _layer_id)
            : current_edge_(_network->make_iterator<direction == LEFT>(_vertex_id))
            , layer_id_(_layer_id) {
            if (valid() && !correct_delta_()) {
                operator ++();
            }
        }

        ResidualNetwork::Edge& operator *() {
            return *current_edge_;
        }

        ResidualNetwork::Edge& back_edge() {
            return current_edge_.back_edge();
        }

        size_t go_through() {
            return current_edge_.go_through();
        }

        bool valid() {
            return current_edge_.valid();
        }

        void operator ++() {
            ++current_edge_;
            while (current_edge_.valid() && !correct_delta_()) {
                ++current_edge_;
            }
        }
    };

    std::vector<int>                 potential_in_,
                                     potential_out_,
                                     excess_,
                                     layer_id_;
    std::vector<char>                deleted_,
                                     can_reach_sink_;

    std::vector<LayeredEdgeIterator<LEFT>>  iterator_left_;
    std::vector<LayeredEdgeIterator<RIGHT>> iterator_right_;

    int potential_(size_t u) {
        return std::min(potential_in_[u], potential_out_[u]);
    }

    void mark_reachable_(size_t vertex_id, std::vector<char>& visited) {
        visited[vertex_id] = true;
        for (LayeredEdgeIterator<LEFT> it(network_, vertex_id, &layer_id_); it.valid(); ++it) {
            if (!visited[it.go_through()]) {
                mark_reachable_(it.go_through(), visited);
            }
        }
    }

    bool build_layers_() {
        layer_id_.assign(network_->vertices_cnt(), ResidualNetwork::INF);
        layer_id_[network_->source()] = 0;
        std::queue<size_t> bfs_queue;
        bfs_queue.push(network_->source());
        while (!bfs_queue.empty()) {
            size_t vertex_id = bfs_queue.front();
            bfs_queue.pop();
            for (auto it = network_->make_iterator(vertex_id); it.valid(); ++it) {
                if (layer_id_[(*it).to()] > layer_id_[vertex_id] + 1) {
                    layer_id_[(*it).to()] = layer_id_[vertex_id] + 1;
                    bfs_queue.push((*it).to());
                }
            }
        }
        can_reach_sink_.assign(network_->vertices_cnt(), false);
        mark_reachable_(network_->sink(), can_reach_sink_);
        for (size_t vertex_id = 0; vertex_id < network_->vertices_cnt(); ++vertex_id) {
            if (!can_reach_sink_[vertex_id]) {
                layer_id_[vertex_id] = ResidualNetwork::INF;
            }
        }
        return (layer_id_[network_->sink()] < ResidualNetwork::INF);
    }

    template<DIRECTION direction>
    void count_potentials_(std::vector<int>& potential, size_t special_vertex) {
        potential.assign(network_->vertices_cnt(), 0);
        potential[special_vertex] = ResidualNetwork::INF;
        for (size_t vertex_id = 0; vertex_id < network_->vertices_cnt(); ++vertex_id) {
            for (LayeredEdgeIterator<direction> it(network_, vertex_id, &layer_id_); it.valid(); ++it) {
                potential[vertex_id] += (*it).residual_capacity();
            }
        }
    }

    template<DIRECTION direction>
    void init_edge_iterators_(std::vector<LayeredEdgeIterator<direction>>& iterator) {
        iterator.resize(network_->vertices_cnt());
        for (size_t vertex_id = 0; vertex_id < network_->vertices_cnt(); ++vertex_id) {
            iterator[vertex_id] = LayeredEdgeIterator<direction>(network_, vertex_id, &layer_id_);
        }
    }

    void init_iteration_() {
        count_potentials_<LEFT>(potential_in_, network_->source());
        count_potentials_<RIGHT>(potential_out_, network_->sink());
        init_edge_iterators_<LEFT>(iterator_left_);
        init_edge_iterators_<RIGHT>(iterator_right_);
        deleted_.assign(network_->vertices_cnt(), false);
    }

    std::vector<int>& direction_to_potential_(DIRECTION direction) {
        return (direction == LEFT ? potential_out_ : potential_in_);
    }

    template<DIRECTION direction>
    void delete_adjacent_(size_t vertex_id, std::queue<size_t>& saturated) {
        for (LayeredEdgeIterator<direction> it(network_, vertex_id, &layer_id_); it.valid(); ++it) {
            size_t to = it.go_through();
            direction_to_potential_(direction)[to] -= (*it).residual_capacity();
            if (potential_(to) == 0 && !deleted_[to]) {
                deleted_[to] = true;
                saturated.push(to);
            }
        }
    }

    void delete_saturated_vertices_() {
        std::queue<size_t> saturated;
        for (size_t vertex_id = 0; vertex_id < network_->vertices_cnt(); ++vertex_id) {
            if (!deleted_[vertex_id] && potential_(vertex_id) == 0) {
                saturated.push(vertex_id);
            }
        }
        while (!saturated.empty()) {
            size_t vertex_id = saturated.front();
            saturated.pop();
            deleted_[vertex_id] = true;
            delete_adjacent_<LEFT>(vertex_id, saturated);
            delete_adjacent_<RIGHT>(vertex_id, saturated);
        }
    }

    size_t get_reference_node() {
        size_t reference_node = ResidualNetwork::NONE;
        for (size_t vertex_id = 0; vertex_id < network_->vertices_cnt(); ++vertex_id) {
            if (potential_(vertex_id) > 0 && (
                    reference_node == ResidualNetwork::NONE ||
                    potential_(reference_node) > potential_(vertex_id))
                    ) {
                reference_node = vertex_id;
            }
        }
        return reference_node;
    }

    template<DIRECTION direction>
    void push_(size_t reference_node, int push_value,
               std::vector<LayeredEdgeIterator<direction>>& iterator
    ) {
        excess_.assign(network_->vertices_cnt(), 0);
        excess_[reference_node] = push_value;
        std::queue<size_t> bfs_queue;
        bfs_queue.push(reference_node);
        direction_to_potential_(direction)[reference_node] += push_value;
        std::vector<char> visited(network_->vertices_cnt(), false);
        while (!bfs_queue.empty()) {
            size_t vertex_id = bfs_queue.front();
            bfs_queue.pop();
            visited[vertex_id] = true;
            potential_in_[vertex_id] -= push_value;
            potential_out_[vertex_id] -= push_value;
            for (LayeredEdgeIterator<direction>& it = iterator[vertex_id]; it.valid(); ++it) {
                size_t to = it.go_through();
                if (deleted_[to]) continue;
                int push_through_edge = std::min(push_value, (*it).residual_capacity());
                excess_[vertex_id] -= push_through_edge;
                excess_[to] += push_through_edge;
                (*it).flow() += push_through_edge;
                it.back_edge().flow() -= push_through_edge;
                if (!visited[to]) {
                    visited[to] = true;
                    bfs_queue.push(to);
                }
                if (!(*it).saturated()) break;
            }
        }
    }

  public:

    void run() override {
        network_->discard_flow();
        while (build_layers_()) {
            init_iteration_();
            delete_saturated_vertices_();
            while (potential_(network_->sink()) > 0) {
                size_t reference_node = get_reference_node();
                int push_value = potential_(reference_node);
                flow_ += push_value;
                push_<RIGHT>(reference_node, push_value, iterator_right_);
                push_<LEFT>(reference_node, push_value, iterator_left_);
                delete_saturated_vertices_();
            }
        }
    }
};

class PushRelabel : public MaxFlowAlgorithm {
  private:

    std::vector<size_t> height_;
    std::vector<int> excess_;

    void push_through_edge_(ResidualNetwork::EdgeIterator<> it, int push_value) {
        (*it).flow() += push_value;
        it.back_edge().flow() -= push_value;
    }

    void push_and_update_excess_(ResidualNetwork::EdgeIterator<> it) {
        int push_value = std::min(excess_[it.back_edge().to()], (*it).residual_capacity());
        push_through_edge_(it, push_value);
        excess_[it.back_edge().to()] -= push_value;
        excess_[(*it).to()] += push_value;
    }

    void relabel_(size_t vertex_id) {
        auto minimal_adjacent_height = static_cast<size_t>(ResidualNetwork::INF);
        for (auto it = network_->make_iterator(vertex_id); it.valid(); ++it) {
            if ((*it).saturated()) continue;
            minimal_adjacent_height = std::min(minimal_adjacent_height,
                                               height_[(*it).to()]);
        }
        height_[vertex_id] = minimal_adjacent_height + 1;
    }

    void init_() {
        excess_.assign(network_->vertices_cnt(), 0);
        excess_[network_->source()] = ResidualNetwork::INF;
        height_.assign(network_->vertices_cnt(), 0);
        height_[network_->source()] = network_->vertices_cnt();
        network_->discard_flow();
        for (auto it = network_->make_iterator(network_->source()); it.valid(); ++it) {
            push_and_update_excess_(it);
        }
    }

    void discharge_(size_t vertex_id) {
        while (excess_[vertex_id] > 0) {
            for (auto it = network_->make_iterator(vertex_id); it.valid(); ++it) {
                if ((*it).saturated()) continue;
                if (height_[vertex_id] == height_[(*it).to()] + 1) {
                    push_and_update_excess_(it);
                }
            }
            relabel_(vertex_id);
        }
    }

  public:

    void run() override {
        init_();
        std::list<size_t> order;
        for (size_t vertex_id = 0; vertex_id < network_->vertices_cnt(); ++vertex_id) {
            if (vertex_id != network_->source() && vertex_id != network_->sink()) {
                order.push_back(vertex_id);
            }
        }
        for (auto it = order.begin(); it != order.end(); ++it) {
            size_t old_height = height_[*it];
            discharge_(*it);
            if (height_[*it] > old_height) {
                order.push_front(*it);
                order.erase(it);
                it = order.begin();
            }
        }
        flow_ = excess_[network_->sink()];
    }
};

struct Data {
    size_t topic_cnt;
    std::vector<int> values;
    std::vector<std::set<size_t>> dependencies;

    void read(std::istream& in) {
        in >> topic_cnt;
        values.resize(topic_cnt);
        dependencies.resize(topic_cnt);
        for (auto& value : values) {
            in >> value;
        }
        for (size_t topic_id = 0; topic_id < topic_cnt; ++topic_id) {
            size_t dependencies_cnt;
            in >> dependencies_cnt;
            while (dependencies_cnt--) {
                size_t required_topic_id;
                in >> required_topic_id;
                dependencies[topic_id].insert(required_topic_id - 1);
            }
        }
    }
};

template<class MaxFlowAlgorithm>
int solve(const Data& data) {
    size_t source_id = data.topic_cnt,
            sink_id   = data.topic_cnt + 1;
    ResidualNetwork network(data.topic_cnt + 2, source_id, sink_id);
    int positive_sum = 0;
    for (size_t i = 0; i < data.topic_cnt; ++i) {
        positive_sum += std::max(data.values[i], 0);
        for (size_t j = 0; j < i; ++j) {
            if (data.dependencies[i].count(j)) {
                network.add_edge(i, j, ResidualNetwork::INF, !data.dependencies[j].count(i));
            } else if (data.dependencies[j].count(i)) {
                network.add_edge(j, i, ResidualNetwork::INF);
            }
        }
    }
    for (size_t topic_id = 0; topic_id < data.topic_cnt; ++topic_id) {
        if (data.values[topic_id] >= 0) {
            network.add_edge(source_id, topic_id, data.values[topic_id]);
        } else {
            network.add_edge(topic_id, sink_id, -data.values[topic_id]);
        }
    }
    MaxFlowAlgorithm algorithm;
    algorithm.init(&network);
    algorithm.run();
    return positive_sum - algorithm.flow();
}

void run(std::istream& in, std::ostream& out) {
    Data data;
    data.read(in);
    int answer_malhotra     = solve<Malhotra>(data);
    int answer_pushrelabel  = solve<PushRelabel>(data);
    assert(answer_malhotra == answer_pushrelabel);
    out << answer_malhotra;
}

int main() {
    run(std::cin, std::cout);
}