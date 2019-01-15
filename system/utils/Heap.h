#ifndef HEAP_H
#define HEAP_H

#include <vector>
#include <stddef.h>
using namespace std;

template <class KeyT, class ValT>
struct qelem {
    KeyT key;
    ValT val;
    int pos;

    qelem()
    {
    }

    qelem(KeyT key, ValT val)
    {
        this->key = key;
        this->val = val;
    }

    bool operator<(const qelem o) const
    {
        return key < o.key;
    }
};

template <class KeyT, class ValT>
class heap {
public:
    typedef qelem<KeyT, ValT> elemT;

private:
    vector<elemT*> elements;

public:
    heap()
    {
        elements.push_back(NULL);
    }

    //=================================

    inline int size()
    {
        return elements.size() - 1;
    }

    inline int getLeftChildIndex(int index)
    {
        return 2 * index;
    }

    inline int getRightChildIndex(int index)
    {
        return 2 * index + 1;
    }

    inline int getParentIndex(int index)
    {
        return index / 2;
    }

    elemT* getLeftChild(int index)
    {
        return elements[2 * index];
    }

    elemT* getRightChild(int index)
    {
        return elements[2 * index + 1];
    }

    elemT* getParent(int index)
    {
        return elements[index / 2];
    }

    //=================================

    void add(elemT& newElement)
    {
        elements.push_back(NULL);
        int index = elements.size() - 1;
        while (index > 1 && !(*getParent(index) < newElement)) {
            elemT* father = getParent(index);
            elements[index] = father;
            father->pos = index;
            index = getParentIndex(index);
        }
        elements[index] = &newElement;
        newElement.pos = index;
    }

    void fix(elemT& ele) //decreaseKey
    {
        int pos = ele.pos;
        while (pos > 1) {
            elemT* father = getParent(pos);
            if (*father < ele)
                break;
            elemT* tmp = elements[father->pos];
            elements[father->pos] = elements[pos];
            elements[pos] = tmp;
            int temp = pos;
            pos = father->pos;
            father->pos = temp;
        }
        ele.pos = pos;
    }

    elemT& peek()
    {
        return *elements[1];
    }

    void fixHeap()
    {
        elemT* root = elements[1];
        int lastIndex = elements.size() - 1;
        int index = 1;
        bool more = true;
        while (more) {
            int childIndex = getLeftChildIndex(index);
            if (childIndex <= lastIndex) {
                elemT* child = getLeftChild(index);
                if (getRightChildIndex(index) <= lastIndex && *getRightChild(index) < *child) {
                    childIndex = getRightChildIndex(index);
                    child = getRightChild(index);
                }
                if (*child < *root) {
                    elements[index] = child;
                    child->pos = index;
                    index = childIndex;
                } else {
                    more = false;
                }
            } else {
                more = false;
            }
        }
        elements[index] = root;
        root->pos = index;
    }

    elemT* remove()
    {
        elemT* minimum = elements[1];
        int lastIndex = elements.size() - 1;
        elemT* last = elements[lastIndex];
        elements.resize(lastIndex);
        if (lastIndex > 1) {
            elements[1] = last;
            last->pos = 1;
            fixHeap();
        }
        return minimum;
    }
};

/*//sample code
 * int main(int argc, char **argv) {

	heap<int, int> hp;

	qelem<int, int> en1(9, 1);
	hp.add(en1);

	qelem<int, int> en2(8, 2);
	hp.add(en2);

	qelem<int, int> en3(7, 3);
	hp.add(en3);

	qelem<int, int> en4(6, 4);
	hp.add(en4);

	cout<<"hp.peek()=("<<hp.peek().key<<", "<<hp.peek().val<<")"<<endl;
	cout<<"hp.size()="<<hp.size()<<endl;

	en1.key=3;
	hp.fix(en1);

	cout<<"hp.peek()=("<<hp.peek().key<<", "<<hp.peek().val<<")"<<endl;
	cout<<"hp.size()="<<hp.size()<<endl;

	return 0;
}
*/

#endif
