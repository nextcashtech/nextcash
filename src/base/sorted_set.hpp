/**************************************************************************
 * Copyright 2018 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_SORTED_SET_HPP
#define NEXTCASH_SORTED_SET_HPP

#include <cstdint>
#include <vector>


namespace NextCash
{
    class SortedObject
    {
    public:

        virtual ~SortedObject() {}

        // Returns criteria for sort.
        //   < 0 when this object is less than pRight.
        //   = 0 when this object is equal to pRight.
        //   > 0 when this object is more than pRight.
        virtual int compare(const SortedObject *pRight) const = 0;

        // Returns true if this object has the same "value" as pRight.
        // Objects that have the same value will not be inserted.
        // Objects with the same "value" must have the same "sort".
        // Note: Objects can have the same "sort", but not be equal.
        //   (i.e. compare can return 0 while == returns false and the object can be inserted)
        virtual bool valueEquals(const SortedObject *pRight) const { return false; }

    };

    class SortedSet
    {
    public:

        SortedSet() {}
        ~SortedSet();

        unsigned int size() const { return mItems.size(); }

        void reserve(unsigned int pSize) { mItems.reserve(pSize); }

        bool contains(const SortedObject &pMatching);

        // Returns true if the item was inserted.
        bool insert(SortedObject *pObject, bool pAllowDuplicateSorts = false);

        // Returns true if the item was removed. Deletes item.
        bool remove(const SortedObject &pMatching);

        // Returns the item with the specified hash.
        // Return NULL if not found.
        SortedObject *get(const SortedObject &pMatching);

        // Returns item and doesn't delete it.
        SortedObject *getAndRemove(const SortedObject &pMatching);

        void clear();
        void clearNoDelete(); // Doesn't delete items.

        typedef std::vector<SortedObject *>::iterator Iterator;

        Iterator begin() { return mItems.begin(); }
        Iterator end() { return mItems.end(); }
        Iterator find(const SortedObject &pMatching);// Return iterator to first matching item.

        Iterator eraseDelete(Iterator &pIterator)
        {
            delete *pIterator;
            return mItems.erase(pIterator);
        }
        Iterator eraseNoDelete(Iterator &pIterator) { return mItems.erase(pIterator); }

        static bool test();

    protected:

        std::vector<SortedObject *> mItems;

        // If item is not found, give item after where it should be.
        static const uint8_t RETURN_INSERT_POSITION = 0x01;
        // Another item with the same sort value was found.
        static const uint8_t SORT_WAS_FOUND = 0x02;
        // Another item with the same value was found.
        static const uint8_t VALUE_WAS_FOUND = 0x04;

        // if RETURN_INSERT_POSITION:
        //   return the iterator for the item after the last matching item.
        //   or the item in the position the item specified belongs.
        // else:
        //   return the iterator for the first item matching.
        //   or "end" if no item matching.
        //
        // Sets SORT_WAS_FOUND in pFlags if a matching item was found.
        Iterator binaryFind(const SortedObject &pMatching, uint8_t &pFlags);

        // if RETURN_INSERT_POSITION:
        //   increment iterator forward to the item after the last matching item.
        // else:
        //   increment iterator forward to the last matching item.
        Iterator moveForward(Iterator pIterator, const SortedObject &pMatching, uint8_t &pFlags);

        // Decrement iterator backward to the first matching item.
        Iterator moveBackward(Iterator pIterator, const SortedObject &pMatching, uint8_t &pFlags);

        // Search for matching value from specified iterator and set VALUE_WAS_FOUND if found.
        void searchBackward(Iterator pIterator, const SortedObject &pMatching, uint8_t &pFlags);
        void searchForward(Iterator pIterator, const SortedObject &pMatching, uint8_t &pFlags);

        SortedSet(const SortedSet &pCopy);
        SortedSet &operator = (const SortedSet &pRight);

    };
}

#endif
