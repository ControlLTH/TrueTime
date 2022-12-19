/*
 * Copyright (c) 2016 Lund University
 *
 * Written by Anton Cervin, Dan Henriksson and Martin Ohlin,
 * Department of Automatic Control LTH, Lund University, Sweden.
 *   
 * This file is part of TrueTime 2.0.
 *
 * TrueTime 2.0 is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * TrueTime 2.0 is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TrueTime 2.0. If not, see <http://www.gnu.org/licenses/>
 */

#ifndef LINKEDLIST_H
#define LINKEDLIST_H

class Node;

/**
 * List class
 */
class List {
  Node *pHead, *pTail;
  int (*compFcn)(Node *, Node *); // should return 1 if val(n1) < val(n2)
                                  // and -1 otherwise
public:
  char name[MAXCHARS];
  Node *getFirst();
  Node *getLast();
  void appendNode(Node *pNode);
  void insertAfter(Node *pNode, Node *pAfter);
  void insertBefore(Node *pNode, Node *pBefore);
  void insertSorted(Node *pNode);
  void removeNode(Node *pNode);
  void deleteNode(Node *pNode);
  int length();
  void print();

  List(const char *n, int (*compFcn)(Node *, Node *));
  ~List();
};

/**
 * Node class
 */
class Node {
public:
  Node* pNext;
  Node* pPrev;
  List* myList;
  
  Node *getNext();
  Node *getPrev();
  void remove();
  void moveToList(List* newlist);
  
  virtual void print() = 0;
  Node(); // constructor
  virtual ~Node(){}
};

#endif
