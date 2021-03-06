// Copyright (c) 2009-2017 The Regents of the University of Michigan
// This file is part of the HOOMD-blue project, released under the BSD 3-Clause License.


// Maintainer: jglaser

#include "hoomd/HOOMDMath.h"
#include "hoomd/VectorMath.h"
#include <vector>
#include <stack>

#include "OBB.h"

#ifndef __OBB_TREE_H__
#define __OBB_TREE_H__

/*! \file OBBTree.h
    \brief OBBTree build and query methods
*/

// need to declare these class methods with __device__ qualifiers when building in nvcc
// DEVICE is __host__ __device__ when included in nvcc and blank when included into the host compiler
#ifdef NVCC
#define DEVICE __device__
#else
#define DEVICE
#endif

namespace hpmc
{

namespace detail
{

/*! \addtogroup overlap
    @{
*/

const unsigned int OBB_INVALID_NODE = 0xffffffff;   //!< Invalid node index sentinel

#ifndef NVCC

//! Node in an OBBTree
/*! Stores data for a node in the OBB tree
*/
template<unsigned int node_capacity>
struct OBBNode
    {
    //! Default constructor
    OBBNode()
        {
        left = right = parent = OBB_INVALID_NODE;
        num_particles = 0;
        skip = 0;
        }

    OBB obb;           //!< The box bounding this node's volume
    unsigned int left;   //!< Index of the left child
    unsigned int right;  //!< Index of the right child
    unsigned int parent; //!< Index of the parent node
    unsigned int skip;   //!< Number of array indices to skip to get to the next node in an in order traversal

    unsigned int particles[node_capacity];      //!< Indices of the particles contained in the node
    unsigned int num_particles;                 //!< Number of particles contained in the node
    } __attribute__((aligned(32)));

//! OBB Tree
/*! An OBBTree stores a binary tree of OBBs. A leaf node stores up to node_capacity particles by index. The bounding
    box of a leaf node is surrounds all the bounding boxes of its contained particles. Internal nodes have OBBs that
    enclose all of their children. The tree supports the following operations:

    - Query  : Search through the tree and build a list of all particles that intersect with the query OBB. Runs in
               O(log N) time
    - buildTree : build an efficiently arranged tree given a complete set of OBBs, one for each particle.

    **Implementation details**

    OBBTree stores all nodes in a flat array manged by std::vector. To easily locate particle leaf nodes for update,
    a reverse mapping is stored to locate the leaf node containing a particle. m_root tracks the index of the root node
    as the tree is built. The nodes store the indices of their left and right children along with their OBB. Nodes
    are allocated as needed with allocate(). With multiple particles per leaf node, the total number of internal nodes
    needed is not known (but can be estimated) until build time.

    For performance, no recursive calls are used. Instead, each function is either turned into a loop if it uses
    tail recursion, or it uses a local stack to traverse the tree. The stack is cached between calls to limit
    the amount of dynamic memory allocation.
*/
template< unsigned int node_capacity>
class OBBTree
    {
    public:
        //! Construct an OBBTree
        OBBTree()
            : m_nodes(0), m_num_nodes(0), m_node_capacity(0), m_root(0)
            {
            }

        // Destructor
        ~OBBTree()
            {
            if (m_nodes)
                free(m_nodes);
            }

        //! Build a tree smartly from a list of OBBs and internal coordinates
        inline void buildTree(OBB *obbs, std::vector<std::vector<vec3<OverlapReal> > >& internal_coordinates,
            OverlapReal vertex_radius, unsigned int N);

        //! Build a tree from a list of OBBs
        inline void buildTree(OBB *obbs, unsigned int N);

        //! Find all particles that overlap with the query OBB
        inline unsigned int query(std::vector<unsigned int>& hits, const OBB& obb) const;

        //! Update the OBB of a particle
        inline void update(unsigned int idx, const OBB& obb);

        //! Get the height of a given particle's leaf node
        inline unsigned int height(unsigned int idx);

        //! Get the number of nodes
        inline unsigned int getNumNodes() const
            {
            return m_num_nodes;
            }

        //! Test if a given index is a leaf node
        /*! \param node Index of the node (not the particle) to query
        */
        inline bool isNodeLeaf(unsigned int node) const
            {
            return (m_nodes[node].left == OBB_INVALID_NODE);
            }

        //! Get the OBBNode
        /*! \param node Index of the node (not the particle) to query
         */
        inline const OBBNode<node_capacity>& getNode(unsigned int node) const
            {
            return m_nodes[node];
            }

        //! Get the OBB of a given node
        /*! \param node Index of the node (not the particle) to query
        */
        inline const OBB& getNodeOBB(unsigned int node) const
            {
            return (m_nodes[node].obb);
            }

        //! Get the skip of a given node
        /*! \param node Index of the node (not the particle) to query
        */
        inline unsigned int getNodeSkip(unsigned int node) const
            {
            return (m_nodes[node].skip);
            }

        //! Get the left child of a given node
        /*! \param node Index of the node (not the particle) to query
        */
        inline unsigned int getNodeLeft(unsigned int node) const
            {
            return (m_nodes[node].left);
            }

        //! Get the number of particles in a given node
        /*! \param node Index of the node (not the particle) to query
        */
        inline unsigned int getNodeNumParticles(unsigned int node) const
            {
            return (m_nodes[node].num_particles);
            }

        //! Get the particles in a given node
        /*! \param node Index of the node (not the particle) to query
        */
        inline unsigned int getNodeParticle(unsigned int node, unsigned int j) const
            {
            return (m_nodes[node].particles[j]);
            }

    private:
        OBBNode<node_capacity> *m_nodes;                  //!< The nodes of the tree
        unsigned int m_num_nodes;           //!< Number of nodes
        unsigned int m_node_capacity;       //!< Capacity of the nodes array
        unsigned int m_root;                //!< Index to the root node of the tree
        std::vector<unsigned int> m_mapping;//!< Reverse mapping to find node given a particle index

        //! Initialize the tree to hold N particles
        inline void init(unsigned int N);

        //! Build a node of the tree recursively
        inline unsigned int buildNode(OBB *obbs, std::vector<std::vector<vec3<OverlapReal> > >& internal_coordinates,
            OverlapReal vertex_radius, std::vector<unsigned int>& idx, unsigned int start, unsigned int len, unsigned int parent);

        //! Allocate a new node
        inline unsigned int allocateNode();

        //! Update the skip value for a node
        inline unsigned int updateSkip(unsigned int idx);
    };


/*! \param N Number of particles to allocate space for

    Initialize the tree with room for N particles.
*/
template<unsigned int node_capacity>
inline void OBBTree<node_capacity>::init(unsigned int N)
    {
    // clear the nodes
    m_num_nodes = 0;

    // init the root node and mapping to invalid states
    m_root = OBB_INVALID_NODE;
    m_mapping.resize(N);

    for (unsigned int i = 0; i < N; i++)
        m_mapping[i] = OBB_INVALID_NODE;
    }

/*! \param hits Output vector of positive hits.
    \param obb The OBB to query
    \returns the number of box overlap checks made during the recursion

    The *hits* vector is not cleared, elements are only added with push_back. query() traverses the tree and finds all
    of the leaf nodes that intersect *obb*. The index of each intersecting leaf node is added to the hits vector.
*/
template<unsigned int node_capacity>
inline unsigned int OBBTree<node_capacity>::query(std::vector<unsigned int>& hits, const OBB& obb) const
    {
    unsigned int box_overlap_counts = 0;

    // avoid pointer indirection overhead of std::vector
    OBBNode<node_capacity>* nodes = &m_nodes[0];

    // stackless search
    for (unsigned int current_node_idx = 0; current_node_idx < m_num_nodes; current_node_idx++)
        {
        // cache current node pointer
        const OBBNode<node_capacity>& current_node = nodes[current_node_idx];

        box_overlap_counts++;
        if (overlap(current_node.obb, obb))
            {
            if (current_node.left == OBB_INVALID_NODE)
                {
                for (unsigned int i = 0; i < current_node.num_particles; i++)
                    hits.push_back(current_node.particles[i]);
                }
            }
        else
            {
            // skip ahead
            current_node_idx += current_node.skip;
            }
        }

    return box_overlap_counts;
    }


/*! \param idx Particle to get height for
    \returns Height of the node
*/
template<unsigned int node_capacity>
inline unsigned int OBBTree<node_capacity>::height(unsigned int idx)
    {
    assert(idx < m_mapping.size());

    // find the node this particle is in
    unsigned int node_idx = m_mapping[idx];

    // handle invalid nodes
    if (node_idx == OBB_INVALID_NODE)
        return 0;

    // follow the parent pointers up and count the steps
    unsigned int height = 1;

    unsigned int current_node = m_nodes[node_idx].parent;
    while (current_node != OBB_INVALID_NODE)
        {
        current_node = m_nodes[current_node].parent;
        height += 1;
        }

    return height;
    }


/*! \param obbs List of OBBs for each particle (must be 32-byte aligned)
    \param internal_coordinates List of lists of vertex contents of OBBs
    \param vertex_radius Radius of every vertex
    \param N Number of OBBs in the list

    Builds a balanced tree from a given list of OBBs for each particle. Data in \a obbs will be modified during
    the construction process.
*/
template<unsigned int node_capacity>
inline void OBBTree<node_capacity>::buildTree(OBB *obbs, std::vector<std::vector<vec3<OverlapReal> > >& internal_coordinates,
    OverlapReal vertex_radius, unsigned int N)
    {
    init(N);

    std::vector<unsigned int> idx;
    for (unsigned int i = 0; i < N; i++)
        idx.push_back(i);

    m_root = buildNode(obbs, internal_coordinates, vertex_radius, idx, 0, N, OBB_INVALID_NODE);
    updateSkip(m_root);
    }

/*! \param obbs List of OBBs for each particle (must be 32-byte aligned)
    \param N Number of OBBs in the list

    Builds a balanced tree from a given list of OBBs for each particle. Data in \a obbs will be modified during
    the construction process.
*/
template<unsigned int node_capacity>
inline void OBBTree<node_capacity>::buildTree(OBB *obbs, unsigned int N)
    {
    init(N);

    std::vector<unsigned int> idx;
    for (unsigned int i = 0; i < N; i++)
        idx.push_back(i);

    // initialize internal coordinates from OBB corners
    std::vector< std::vector<vec3<OverlapReal> > > internal_coordinates;
    for (unsigned int i = 0; i < N; ++i)
        {
        internal_coordinates.push_back(obbs[i].getCorners());
        }

    m_root = buildNode(obbs, internal_coordinates, 0.0, idx, 0, N, OBB_INVALID_NODE);
    updateSkip(m_root);
    }


/*! \param obbs List of OBBs
    \param idx List of indices
    \param start Start point in obbs and idx to examine
    \param len Number of obbs to examine
    \param parent Index of the parent node

    buildNode is the main driver of the smart OBB tree build algorithm. Each call produces a node, given a set of
    OBBs. If there are fewer OBBs than fit in a leaf, a leaf is generated. If there are too many, the total OBB
    is computed and split on the largest length axis. The total tree is built by recursive splitting.

    The obbs and idx lists are passed in by reference. Each node is given a subrange of the list to own (start to
    start + len). When building the node, it partitions it's subrange into two sides (like quick sort).
*/
template<unsigned int node_capacity>
inline unsigned int OBBTree<node_capacity>::buildNode(OBB *obbs,
                                       std::vector<std::vector<vec3<OverlapReal> > >& internal_coordinates,
                                       OverlapReal vertex_radius,
                                       std::vector<unsigned int>& idx,
                                       unsigned int start,
                                       unsigned int len,
                                       unsigned int parent)
    {
    // merge all the OBBs into one, as tightly as possible
    OBB my_obb = obbs[start];
    std::vector<vec3<OverlapReal> > merge_internal_coordinates;

    for (unsigned int i = start; i < start+len; ++i)
        {
        for (unsigned int j = 0; j < internal_coordinates[i].size(); ++j)
            {
            merge_internal_coordinates.push_back(internal_coordinates[i][j]);
            }
        }

    // merge internal coordinates
    my_obb = compute_obb(merge_internal_coordinates, vertex_radius);

    // handle the case of a leaf node creation
    if (len <= node_capacity)
        {
        unsigned int new_node = allocateNode();
        m_nodes[new_node].obb = my_obb;
        m_nodes[new_node].parent = parent;
        m_nodes[new_node].num_particles = len;

        for (unsigned int i = 0; i < len; i++)
            {
            // assign the particle indices into the leaf node
            m_nodes[new_node].particles[i] = idx[start+i];

            // assign the reverse mapping from particle indices to leaf node indices
            m_mapping[idx[start+i]] = new_node;
            }

        return new_node;
        }

    // otherwise, we are creating an internal node - allocate an index
    unsigned int my_idx = allocateNode();

    // need to split the list of obbs into two sets for left and right
    unsigned int start_left = 0;
    unsigned int start_right = len;

    rotmat3<OverlapReal> my_axes(transpose(my_obb.rotation));

    // if there are only 2 obbs, put one on each side
    if (len == 2)
        {
        // nothing to do, already partitioned
        }
    else
        {
        // the x-axis has largest covariance by construction, so split along that axis
        // split on x direction according to spatial median
        for (unsigned int i = 0; i < start_right; i++)
            {
            OverlapReal proj = dot(obbs[start+i].center-my_obb.center,my_axes.row0);
            if (proj < OverlapReal(0.0))
                {
                // if on the left side, everything is happy, just continue on
                }
            else
                {
                // if on the right side, need to swap the current obb with the one at start_right-1, subtract
                // one off of start_right to indicate the addition of one to the right side and subtrace 1
                // from i to look at the current index (new obb). This is quick and easy to write, but will
                // randomize indices - might need to look into a stable partitioning algorithm!
                std::swap(obbs[start+i], obbs[start+start_right-1]);
                std::swap(idx[start+i], idx[start+start_right-1]);
                std::swap(internal_coordinates[start+i], internal_coordinates[start+start_right-1]);
                start_right--;
                i--;
                }
            }
        }
    // sanity check. The left or right tree may have ended up empty. If so, just borrow one particle from it
    if (start_right == len)
        start_right = len-1;
    if (start_right == 0)
        start_right = 1;

    // note: calling buildNode has side effects, the m_nodes array may be reallocated. So we need to determine the left
    // and right children, then build our node (can't say m_nodes[my_idx].left = buildNode(...))
    unsigned int new_left = buildNode(obbs, internal_coordinates, vertex_radius, idx, start+start_left, start_right-start_left, my_idx);
    unsigned int new_right = buildNode(obbs, internal_coordinates,  vertex_radius, idx, start+start_right, len-start_right, my_idx);

    // now, create the children and connect them up
    m_nodes[my_idx].obb = my_obb;
    m_nodes[my_idx].parent = parent;
    m_nodes[my_idx].left = new_left;
    m_nodes[my_idx].right = new_right;

    return my_idx;
    }

/*! \param idx Index of the node to update

    updateSkip() updates the skip field of every node in the tree. The skip field is used in the stackless
    implementation of query. Each node's skip field lists the number of nodes that are children to this node. Because
    of the order in which nodes are built in buildNode(), this number is the number of elements to skip in a search
    if a box-box test does not overlap.
*/
template<unsigned int node_capacity>
inline unsigned int OBBTree<node_capacity>::updateSkip(unsigned int idx)
    {
    // leaf nodes have no nodes under them
    if (isNodeLeaf(idx))
        {
        return 1;
        }
    else
        {
        // node idx needs to skip all the nodes underneath it (determined recursively)
        unsigned int left_idx = m_nodes[idx].left;
        unsigned int right_idx = m_nodes[idx].right;

        unsigned int skip = updateSkip(left_idx) + updateSkip(right_idx);
        m_nodes[idx].skip = skip;
        return skip + 1;
        }
    }

/*! Allocates a new node in the tree
*/
template<unsigned int node_capacity>
inline unsigned int OBBTree<node_capacity>::allocateNode()
    {
    // grow the memory if needed
    if (m_num_nodes >= m_node_capacity)
        {
        // determine new capacity
        OBBNode<node_capacity> *m_new_nodes = NULL;
        unsigned int m_new_node_capacity = m_node_capacity*2;
        if (m_new_node_capacity == 0)
            m_new_node_capacity = 16;

        // allocate new memory
        int retval = posix_memalign((void**)&m_new_nodes, 32, m_new_node_capacity*sizeof(OBBNode<node_capacity>));
        if (retval != 0)
            {
            throw std::runtime_error("Error allocating OBBTree memory");
            }

        // if we have old memory, copy it over
        if (m_nodes != NULL)
            {
            memcpy(m_new_nodes, m_nodes, sizeof(OBBNode<node_capacity>)*m_num_nodes);
            free(m_nodes);
            }
        m_nodes = m_new_nodes;
        m_node_capacity = m_new_node_capacity;
        }

    m_nodes[m_num_nodes] = OBBNode<node_capacity>();
    m_num_nodes++;
    return m_num_nodes-1;
    }

// end group overlap
/*! @}*/

#endif // NVCC

}; // end namespace detail

}; // end namespace hpmc

#endif //__OBB_TREE_H__
