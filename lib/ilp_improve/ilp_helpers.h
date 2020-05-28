/******************************************************************************
 * ilp_helpers.h 
 * *
 * Source of KaHIP -- Karlsruhe High Quality Partitioning.
 * Christian Schulz <christian.schulz.phone@gmail.com>
 *****************************************************************************/


#ifndef ILP_ILP_HELPERS_H
#define ILP_ILP_HELPERS_H

#include <algorithm>
#include <unordered_set>

#include "definitions.h"
#include "data_structure/graph_access.h"
#include "ilp_improve.h"

class ilp_helpers {

public:
    Gain compute_gain(graph_access &G, NodeID n, PartitionID pid) {

        std::vector<EdgeWeight> part_connections(G.get_partition_count(), 0);
        EdgeWeight own_connection = 0;

        forall_out_edges(G, e, n) {
            NodeID target = G.getEdgeTarget(e);
            if (G.getPartitionIndex(target) == pid) {
                own_connection += G.getEdgeWeight(e);
            } else {
                part_connections[G.getPartitionIndex(target)] += G.getEdgeWeight(e);
            }

        }endfor

        return own_connection - (*std::max_element(part_connections.begin(), part_connections.end()));
    }

    std::vector<std::pair<NodeID, Gain>> bestStartNodes(graph_access &G,
                                                        std::unordered_set<NodeID> & nodesAvailable,
                                                        std::queue<std::vector<NodeID>> & queue) {


        std::vector<std::pair<NodeID, Gain>> gains;

        forall_nodes(G, n) {
            PartitionID partitionIDSource = G.getPartitionIndex(n);
            forall_out_edges(G, e, n) {
                if (G.getPartitionIndex(G.getEdgeTarget(e)) != partitionIDSource) {
                    Gain gain = compute_gain(G, n, partitionIDSource);
                    gains.emplace_back(n, gain);
                    break;
                }
            } endfor
        } endfor

                std::sort(gains.begin(), gains.end(),[&](auto a1, auto a2) {
            if (a1.second == a2.second) {
                return (G.getNodeDegree(a1.first) < G.getNodeDegree(a2.first));  //rand()%2==0;
            } else {
                return (a1.second < a2.second);
            }
        });

        return gains;
    }

    void cutBFSStartNodes(graph_access &G,
                          std::unordered_set<NodeID> & nodesAvailable,
                          std::queue<std::vector<NodeID>> & queue) {
        forall_nodes(G, n) {
            PartitionID partitionIDSource = G.getPartitionIndex(n);
            forall_out_edges(G, e, n) {
                NodeID targetNode = G.getEdgeTarget(e);
                PartitionID partitionIDTarget = G.getPartitionIndex(targetNode);

                if((partitionIDSource!=partitionIDTarget)) {
                    if (nodesAvailable.count(n) ==0) {
                        std::vector<NodeID> vecA (1, n);
                        queue.push(vecA);
                        nodesAvailable.emplace(n);
                    }

                    if (nodesAvailable.count(targetNode)==0) {
                        std::vector<NodeID> vecB(1, targetNode);
                        queue.push(vecB);
                        nodesAvailable.emplace(targetNode);
                    }
                }
            } endfor
        } endfor
    }

    void gainBFSStartNodes(graph_access &G,
                           std::unordered_set<NodeID> & nodesAvailable,
                           std::queue<std::vector<NodeID>> & queue,
                           int min_gain) {
        forall_nodes(G, n) {
            PartitionID partitionIDSource = G.getPartitionIndex(n);

            Gain gain = compute_gain(G, n, partitionIDSource);
            if (gain <= (-1) *  min_gain) {
                if (nodesAvailable.count(n) == 0) {
                    std::vector<NodeID> vec (1, n);
                    queue.push(vec);
                    nodesAvailable.emplace(n);
                }
            }
        } endfor
    }

    size_t numNonzeroes(size_t n, size_t m, size_t k) {
        return (6*m + 2*(EdgeID) n) * (EdgeID) k;
    }

    size_t numEdgesBetweenNonStarters(graph_access & G, std::unordered_set<NodeID> & nodesAvailable) {

        size_t m =0;
        std::vector<std::vector<bool>> edges_exist;

        for(size_t i = 0; i < G.get_partition_count(); ++i) {
            edges_exist.emplace_back(G.get_partition_count(), false);
            edges_exist[i][i]=true;
        }


        forall_nodes(G, v) {
            if (nodesAvailable.count(v) == 0) {
                PartitionID p1 = G.getPartitionIndex(v);
                forall_out_edges(G, e, v){
                    NodeID tgt = G.getEdgeTarget(e);
                    PartitionID p2 = G.getPartitionIndex(tgt);
                    if (nodesAvailable.count(tgt) == 0 && !edges_exist[p1][p2] && !edges_exist[p2][p1]) {
                        m++;
                        edges_exist[p1][p2]=true;
                        edges_exist[p2][p1]=true;
                    }
                }endfor
            }
        } endfor
        return m;
    }

    size_t edgesInCoarse(graph_access & G, std::unordered_set<NodeID> & nodesAvailable,
                         NodeID q, std::vector<size_t> & degrees) {
        size_t m = 0;
        std::vector<bool> neighbors(G.get_partition_count(), false);
        forall_out_edges(G, e, q) {
            if (nodesAvailable.count(G.getEdgeTarget(e)) != 0 &&
                q > G.getEdgeTarget(e)) {
                m++;
            }

            PartitionID p = G.getPartitionIndex(G.getEdgeTarget(e));
            if (nodesAvailable.count(G.getEdgeTarget(e)) == 0 &&
                !neighbors[p]) {
                neighbors[p] = true;
                degrees[q]++;
                m++;
            }
        } endfor
        return m;
    }

    std::vector<PartitionID> extractPartition(graph_access & G) {
        std::vector<PartitionID> pid(G.number_of_nodes());

        forall_nodes(G, n) {
            pid[n] = G.getPartitionIndex(n);
        } endfor
        return pid;
    }


};


#endif //ILP_ILP_HELPERS_H