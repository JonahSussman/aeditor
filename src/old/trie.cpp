#include <bits/stdc++.h>

class Trie {
  struct Node {
    char letter;
    Node* children;

    Node() {
      children = new Node[27];
    }

    ~Node() {
      delete[] children;
    }
  };

  Node* root;

  Trie() {
    root = new Node;
  }

  void insert() {

  }
};

int main() {

}