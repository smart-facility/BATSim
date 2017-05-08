/****************************************************************
 * FIBOHEAP.HPP
 * 
 * This file contains the implementation of a Fibonacci Heap
 * data structure, necessary for an efficient Dijkstra algorithm
 * implementation.
 * 
 * Authors: Erel Segal - modified by J. Barthelemy
 * Date   : 15 august 2013
 ****************************************************************/

/*! \file FiboHeap.hpp
 *  \brief Fibonacci heap data structure implementation.
 */

#ifndef FIBOHEAP_HPP_
#define FIBOHEAP_HPP_

#include <iostream>
#include <algorithm>
#include <vector>

using namespace std;

typedef unsigned int uint;


//! \brief A template Fibonacci heap node class.
/*!
  Nodes used by the Fibonacci heap data structure, implemented
  in a circular doubly linked list.
 */
template <typename Data, typename Key> class FibonacciHeapNode {
  
  Key myKey;   //!< key associated to the node
  Data myData; //!< data stored in the node

  uint degree; //!< number of children. used in the deletemin algorithm.
  bool mark;   //!< mark used in the decreaseKey algorithm.

  // pointers in a circular doubly linked list
  FibonacciHeapNode<Data,Key>* previous;  //!< pointer to the previous node  
  FibonacciHeapNode<Data,Key>* next;      //!< pointer to the next node
  FibonacciHeapNode<Data,Key>* child;     //!< pointer to the first child in the list of children
  FibonacciHeapNode<Data,Key>* parent;    //!< pointer to the parent

  //! Default constructor.
  FibonacciHeapNode():
    myKey(-1),
    myData(NULL),
    degree(0),
    mark(false),
    child(NULL),
    parent(NULL) {
    previous = next = this; // doubly linked circular list
  }

  //! Constructor.
  /*!
    \param d the data stored in the node
    \param k the key associated to data
   */
  FibonacciHeapNode(Data d, Key k):
    myKey(k),
    myData(d),
    degree(0),
    mark(false),
    child(NULL),
    parent(NULL) {
    previous = next = this; // doubly linked circular list
  }

  //! Check if the node is single.
  /*!
    \return true if the node is single, false otherwise
   */
  bool isSingle() const {
    return (this == this->next);
  }

  //! Inserts a new node after this node.
  /*!
    For example: given 1->2->3->4->1, insert a->b->c->d->a after node 3:
    result: 1->2->3->a->b->c->d->4->1
   */
  void insert(FibonacciHeapNode<Data,Key>* other) {

    if (!other) return;

    this->next->previous = other->previous;
    other->previous->next = this->next;

    this->next = other;
    other->previous = this;

  }

  //! Removes the node from the list.
  void remove() {
    this->previous->next = this->next;
    this->next->previous = this->previous;
    this->next = this->previous = this;
  }

  //! Add a child to the node.
  /*!
    Fibonacci-Heap-Link(other,current) operation.

    \param other a child node to link to the current one
   */
  void addChild(FibonacciHeapNode<Data,Key>* other) {
    if (!child)
      child = other;
    else
      child->insert(other);
    other->parent = this;
    other->mark = false;
    degree++;
    //count += other->count;
  }

  //! Remove a child node.
  /*!
    \param other a child node to remove from current one
   */
  void removeChild(FibonacciHeapNode<Data,Key>* other) {
    if (other->parent!=this)
      throw string ("Trying to remove a child from a non-parent");
    if (other->isSingle()) {
      if (child != other)
        throw string ("Trying to remove a non-child");
      child = NULL;
    } else {
      if (child == other)
        child = other->next;
      other->remove(); // from list of children
    }
    other->parent=NULL;
    other->mark = false;
    degree--;
    //count -= other->count;
  }


  //! Overload of '<<'.
  /*!
    \param out an output stream
    \param n a Fibonacci node

    \return an output stream
   */
  friend ostream& operator<< (ostream& out, const FibonacciHeapNode& n) {
    return (out << n.myData << ":" << n.myKey);
  }

  //! Print the tree of nodes starting from the current one acting as the root.
  /*!
    \param out an output stream
   */
  void printTree(ostream& out) const {
    out << myData << ":" << myKey << ":" << degree << ":" << mark;
    if (child) {
      out << "(";
      const FibonacciHeapNode<Data,Key>* n=child;
      do {
        if (n==this)
          throw string("Illegal pointer - node is child of itself");
        n->printTree(out);
        out << " ";
        n = n->next;
      } while (n!=child);
      out << ")";
    }
  }

  //! Print every tree of nodes on the same level as the current one
  /*!
    \param out an output stream
   */
  void printAll(ostream& out) const {
    const FibonacciHeapNode<Data,Key>* n=this;
    do {
      n->printTree(out);
      out << " ";
      n = n->next;
    } while (n!=this);
    out << endl;
  }

public:

  //! Return node's key.
  /*!
    \return key
   */
  Key key() const { return myKey; }

  //! Return node's data.
  /*!
    \return data
   */
  Data data() const { return myData; }

  template <typename D, typename K> friend class FibonacciHeap;
};


//! \brief Template class for a Fibonacci Heap data structure.
/*!
  This class implements a Fibonacci heap data structure which
  is a min-heap of Fibonacci Nodes sorted by Key

 See http://en.wikipedia.org/wiki/Fibonacci_heap
 and http://www.cse.yorku.ca/~aaw/Jason/FibonacciHeapAlgorithm.html
 for references.
 */
template <typename Data, typename Key> class FibonacciHeap {

  typedef FibonacciHeapNode<Data,Key>* PNode;

  PNode rootWithMinKey; //!< a circular d-list of nodes
  uint count;           //!< total number of elements in heap
  uint maxDegree;       //!< maximum degree (=child count) of a root in the  circular d-list

protected:

  //! Insert a node to the heap.
  /*!
    \param newNode a pointer to the node to be added to the heap
    
    \return a pointer to the newly added node
   */
  PNode insertNode(PNode newNode) {

    if (!rootWithMinKey) { // insert the first myKey to the heap:
      rootWithMinKey = newNode;
    } else {
      rootWithMinKey->insert(newNode);  // insert the root of new tree to the list of roots
      if (newNode->key() < rootWithMinKey->key())  rootWithMinKey = newNode;
    }

    return newNode;

  }

public:

  //! Constructor.
  FibonacciHeap():
    rootWithMinKey(NULL), count(0), maxDegree(0) {}

  //! Destructor.
  ~FibonacciHeap() {

    while(this->empty() == false ) {
      deletemin();
    }

  }

  //! Check if the heap is empty.
  /*!
    \return true if heap is empty, false otherwise
   */
  bool empty() const {return count==0;}

  //! Return the node associated with the minimum key.
  /*!
    \return the minimum Fibonnaci node of the heap
   */
  PNode minimum() const {
    if (!rootWithMinKey)
      throw string("no minimum element");
    return rootWithMinKey;
  }

  //! Print the roots in an output stream.
  /*!
    \param out an output stream
   */
  void printRoots(ostream& out) const {
    out << "maxDegree=" << maxDegree << "  count=" << count << "  roots=";
    if (rootWithMinKey)
      rootWithMinKey->printAll(out);
    else
      out << endl;
  }
  
  //! Fibonacci-Heap union procedure.
  /*!
    Merge a Fibonnacci heap with the current one.
    
    \param other a second Fibonnacci heap to merge with the heap
   */
  void merge (const FibonacciHeap& other) {  
    rootWithMinKey->insert(other.rootWithMinKey);
    if (!rootWithMinKey || (other.rootWithMinKey && other.rootWithMinKey->key() < rootWithMinKey->key()))
      this->rootWithMinKey = other.rootWithMinKey;
    count += other.count;
  }

  //! Insert a node to the heap.
  /*!
    \param d data of the node to be added to the heap
    \param k value of the node to be added to the heap

    \return a pointer to the newly added node
   */
  PNode insert (Data d, Key k) {
    count++;
    // create a new tree with a single myKey:
    return insertNode(new FibonacciHeapNode<Data,Key>(d,k));
  }

  //! Remove the minimum node of the heap, i.e. the Fibonacci Heap Extract Min procedure.
  void deletemin() {  
    if (!rootWithMinKey) throw string("trying to remove from an empty heap");
    count--;

    /// Phase 1: Make all the removed root's children new roots:
    // Make all children of root new roots:
    if (rootWithMinKey->child) {

      PNode c = rootWithMinKey->child;

      do {

        c->parent = NULL;
        c = c->next;

      } while (c!=rootWithMinKey->child);

      rootWithMinKey->child = NULL; // removed all children
      rootWithMinKey->insert(c);

    }

    /// Phase 2-a: handle the case where we delete the last myKey:
    if (rootWithMinKey->next == rootWithMinKey) {

      if (count!=0) throw string ("Internal error: should have 0 keys");
      rootWithMinKey = NULL;
      return;

    }

    /// Phase 2: merge roots with the same degree:
    vector<PNode> degreeRoots ((maxDegree+250)); // make room for a new degree
    fill (degreeRoots.begin(), degreeRoots.end(), (PNode)NULL);
    maxDegree = 0;
    PNode currentPointer = rootWithMinKey->next;
    uint currentDegree;
    do {

      currentDegree = currentPointer->degree;

      PNode current = currentPointer;
      currentPointer = currentPointer->next;
      while (degreeRoots[currentDegree] != NULL) { // merge the two roots with the same degree:
        PNode other = degreeRoots[currentDegree]; // another root with the same degree
        if (current->key() > other->key())
          swap(other,current);
        // now current->key() <= other->key() - make other a child of current:
        other->remove(); // remove from list of roots
        current->addChild(other);

        degreeRoots[currentDegree]=NULL;
        currentDegree++;
        if (currentDegree >= degreeRoots.size())
          degreeRoots.push_back((PNode)NULL);
      }
      // keep the current root as the first of its degree in the degrees array:
      degreeRoots[currentDegree] = current;

    } while (currentPointer != rootWithMinKey);

    /// Phase 3: remove the current root, and calculate the new rootWithMinKey:
    delete rootWithMinKey;
    rootWithMinKey = NULL;

    unsigned int newMaxDegree=0;
    for (unsigned int d=0; d<degreeRoots.size(); ++d) {
      if (degreeRoots[d]) {
        degreeRoots[d]->next = degreeRoots[d]->previous = degreeRoots[d];
        insertNode(degreeRoots[d]);
        if (d>newMaxDegree)
          newMaxDegree = d;
      }
    }
    maxDegree=newMaxDegree;

  }

  //! Decrease the key of a node to a given value.
  /*!
    \param node a node of which key is to be decreased
    \param newKey the new key given to the node
   */
  void decreaseKey(PNode node, Key newKey) {

    if (newKey > node->myKey)
      throw string("Trying to decrease key to a greater key");

    // Update the key and possibly the min key:
    node->myKey = newKey;

    // Check if the new key violates the heap invariant:
    PNode parent = node->parent;

    if (parent == NULL ) { // root node - just make sure the minimum is correct
      if (newKey < rootWithMinKey->key())
        rootWithMinKey = node;
      return; // heap invariant not violated - nothing more to do
    } else if (parent->key() <= newKey) {
      return; // heap invariant not violated - nothing more to do
    }

    for(;;) {
      parent->removeChild(node);
      insertNode(node);

      if (!parent->parent) { // parent is a root - nothing more to do
        break;
      } else if (!parent->mark) {  // parent is not a root and is not marked - just mark it
        parent->mark = true;
        break;
      } else {
        node = parent;
        parent = parent->parent;
        continue;
      }
    };
  }

  //! Remove a node from the heap.
  /*!
    \param node the node to remove from the heap
    \param minusInfinity a key lower to every other key in the heap
   */
  void remove(PNode node, Key minusInfinity) {
    if (minusInfinity >= minimum()->key())
      throw string("2nd argument to remove must be a key that is smaller than all other keys");
    decreaseKey(node, minusInfinity);
    deletemin();
  }

  uint getCount() {
	  return count;
  }

};

#endif /* FIBOHEAP_HPP_ */
