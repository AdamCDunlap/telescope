/**
 * @file LinkedList.h
 *
 * @brief Header and implementation of LinkedList<T>
 *
 * @author Ilya Skriblovsky
 * @date 2009
 *
 * LinkedList<T> is implemented as template and fully defined in this
 * header file
 *
 * $Id: LinkedList.h 174 2011-02-28 17:17:05Z mitrandir $
 *
 */

#ifndef __LINKED_LIST_H
#define __LINKED_LIST_H

#include <stdio.h>

#include <assert.h>

//namespace LightCore
//{

/// Implements the linked list of elements of type T
/**
 * Elements are stored by value
 * 
 * @warning FIXME: If T is class type, it must have default constructor
 */
template <class T>
class LinkedList
{
    private:
        /// Single node of linked list
        /**
         * Contains the payload and pointers to previous and next nodes
         */
        struct Item
        {
            Item* prev; ///< Pointer to previous node(0 if first node)
            Item* next; ///< Pointer to next node (0 if last node)
            T value;    ///< The payload of the node
        };

        Item* m_head; ///< Pointer to list's "head", the first element
        Item* m_tail; ///< Pointer to list's "tail", the last element
        int m_size;   ///< Current number of elements in list

        /// Returns n'th Item object, counting from list's head
        /**
         * This method starts from \c m_head and steps to next element for \c
         * index times. Not effective.
         *
         * Index must be less than \c m_size, otherwise this method will fail
         * on null pointer access.
         *
         * @param[in] index Index of Node to be returned
         * @return          Pointer to index'th node in list
         */
        Item* itemByIndex(int index)
        {
            Item *cur = m_head;
            while (index)
            {
                assert(cur != 0);
                cur = cur->next;
                index--;
            }

            return cur;
        }

        /// Removes node from the list
        /**
         * Removes item from the linked list. Calls destructor for item, so
         * destructor for payload object will be called as well.
         *
         * @param[in] item Item to delete
         */
        void removeItem(Item* item)
        {
            assert(item != 0);
            assert(m_size > 0);

            if (item->next)
                item->next->prev = item->prev;
            else
                m_tail = item->prev;

            if (item->prev)
                item->prev->next = item->next;
            else
                m_head = item->next;

            delete item;

            m_size--;
        }

        /// Searches for item with specified value
        /**
         * @note Comparsion is performed with == operator
         *
         * @param[in] value value to search in list
         * @return          Pointer to first node with specified value or NULL
         *                  if there is no such value in list
         */
        Item* findItem(const T& value)
        {
            Item *cur = m_head;

            while (cur)
            {
                if (cur->value == value)
                    return cur;

                cur = cur->next;
            }

            return 0;
        }

    public:
        /// Iterator for linked list
        /**
         * Can be used for iterating over linked list in STL-like way:
         * @code
         * for (LinkedList<T>::Iterator i = list.head(); i; ++i)
         *     doSomething(*i);
         * @endcode
         *
         * @note It was intended called with capital I to show that it is not
         * fully compilant with STL iterators.
         */
        class Iter
        {
            private:
                /// LinkedList<T> needs to create Iter with private constructor Iter(Item*)
                friend class LinkedList<T>;

                /// The only field - pointer to list node
                Item *item;

            public:
                /// Constructor
                Iter(Item *item): item(item) { }

                /// Casting to bool
                /**
                 * @return true if Iterator pointing to valid list node
                 */
                operator bool()
                {
                    return item != 0;
                }

                /// Returns value of referenced list node
                T& operator *()
                {
                    assert(item != 0);
                    return item->value;
                }

                /// Returns pointer to value of referenced list node
                T* operator ->()
                {
                    assert(item != 0);
                    return &item->value;
                }

                /// Moves iterator to next item
                /**
                 * If iterator points to last item, it will became invalid ((bool)iter == false)
                 */
                Iter& operator++ ()
                {
                    assert(item != 0);
                    item = item->next;
                    return *this;
                }

                /// Moves iterator to previous item
                /**
                 * If iterator points to first item, it will became invalid ((bool)iter == false)
                 */
                Iter& operator-- ()
                {
                    assert(item != 0);
                    item = item->prev;
                    return *this;
                }

                /// Moves iterator to next item
                /**
                 * If iterator points to last item, it will became invalid ((bool)iter == false)
                 *
                 * @note Prefix form is more effective
                 */
                Iter operator++ (int)
                {
                    assert(item != 0);
                    Iter tmp(*this);
                    item = item->next;
                    return tmp;
                }

                /// Moves iterator to previous item
                /**
                 * If iterator points to first item, it will became invalid ((bool)iter == false)
                 *
                 * @note Prefix form is more effective
                 */
                Iter operator-- (int)
                {
                    assert(item != 0);
                    Iter tmp(*this);
                    item = item->prev;
                    return tmp;
                }

                Iter& operator += (int n)
                {
                    for (int i = 0; i < n; ++i)
                    {
                        assert(item != 0);
                        item = item->next;
                    }
                    return *this;
                }
        };

        /// Creates an empty linked list
        LinkedList()
        {
            m_head = m_tail = 0;
            m_size = 0;
        }

        /// Creates a copy of linked list
        LinkedList(const LinkedList<T>& other)
        {
            m_head = m_tail = 0;
            m_size = 0;

            for (Iter i = other.head(); i; ++i)
                append(*i);
        }

        /// Deletes the linked list
        /**
         * Calls clear(). It calls destructors for all items in list.
         */
        ~LinkedList()
        {
            clear();
        }

        /// Returns current number of items in list
        int size() const
        { return m_size; }

        /// Returns iterator pointing to first element in list
        /**
         * @return Iterator pointing to head (first) element in list. If list
         *                  is empty, returned iterator is invalid.
         */
        Iter head() const
        {
            return Iter(m_head);
        }

        /// Returns iterator pointing to last element in list
        /**
         * @return Iterator pointing to tail (last) element in list. If list
         *                  is empty, returned iterator is invalid.
         */
        Iter tail() const
        {
            return Iter(m_tail);
        }

        /// Add value to head (beginning) of list
        void prepend(const T& value)
        {
            Item* newItem = new Item;
            newItem->value = value;
            newItem->prev = 0;
            newItem->next = m_head;

            if (m_head)
                m_head->prev = newItem;
            else
                m_tail = newItem;

            m_head = newItem;

            m_size++;
        }

        /// Add value to tail (end) of list
        void append(const T& value)
        {
            Item* newItem = new Item;
            newItem->value = value;
            newItem->next = 0;
            newItem->prev = m_tail;

            if (m_tail)
                m_tail->next = newItem;
            else
                m_head = newItem;

            m_tail = newItem;

            m_size++;
        }

        /// Add value before n'th element
        void insert(int n, const T& value)
        {
            Item *h = m_head;
            for (int i = 0; i < n; ++i)
                h = h->next;

            Item* newItem = new Item;
            newItem->value = value;
            newItem->next = h;

            if (h)
            {
                newItem->prev = h->prev;
                if (h->prev)
                {
                    h->prev->next = newItem;
                    h->prev = newItem;
                }
                else
                {
                    m_head->prev = newItem;
                    m_head = newItem;
                }
            }
            else
            {
                newItem->prev = m_tail;

                if (m_tail)
                    m_tail->next = newItem;
                else
                    m_head = newItem;

                m_tail = newItem;
            }

            m_size++;
        }

        /// Removes all items from the list
        /**
         * Destructors are called for each item
         */
        void clear()
        {
            while (m_head)
                removeItem(m_head);
        }

        /// Removes n'th item from the list
        /**
         * @param[in] index Index of item to remove. Must be
         *      0 <= index < size(), otherwise method will fail.
         */
        void remove(int index)
        {
            removeItem(itemByIndex(index));
        }

        /// Removes item from the list by iterator
        /**
         * @param[in] iter Iterator pointint to the item to be deleted. Should
         *                 not be invalid.
         */
        void remove(Iter iter)
        {
            removeItem(iter.item);
        }

        /// Removes all items equal to given value
        /**
         * Comparsion is done with == operator.
         */
        void removeByValue(const T& value)
        {
            Item *item;
            do
            {
                item = findItem(value);
                if (item)
                    removeItem(item);
            } while (item);
        }

        /// Checks whether the list contains given value
        /**
         * Comparsion is done with == operator
         */
        bool contains(const T& value)
        {
            return findItem(value) != 0;
        }

        /// Access value by it's index
        /**
         * \see operator[](int)
         */
        inline const T& operator [](int index) const
        {
            assert(index >= 0 && index < m_size);

            Item *cur = m_head;

            while (index)
            {
                cur = cur->next;
                index--;
            }

            return cur->value;
        }

        /// Access value by it's index
        /**
         * This method will iterate from head to n'th element, so it is not
         * effective.
         */
        inline T& operator [](int index)
        {
            assert(index >= 0 && index < m_size);

            Item *cur = m_head;

            while (index)
            {
                cur = cur->next;
                index--;
            }

            return cur->value;
        }
};

//}

#endif
