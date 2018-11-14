/**************************************************************************
 * Copyright 2018 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_HASH_SET_HPP
#define NEXTCASH_HASH_SET_HPP

#include "hash.hpp"

#include <vector>


namespace NextCash
{
    // Can only contain one item for each hash.
    class HashSet
    {
    public:

        HashSet();
        ~HashSet();

        unsigned int size() const { return mItems.size(); }

        void getHashList(HashList &pList) const;

        bool contains(const Hash &pHash);

        // Returns true if the item was inserted.
        bool insert(HashObject *pObject);

        // Returns true if the item was removed. Deletes item.
        bool remove(const Hash &pHash);
        bool removeAt(unsigned int pOffset);

        // Returns the item with the specified hash.
        // Return NULL if not found.
        HashObject *get(const NextCash::Hash &pHash);

        // Returns item and doesn't delete it.
        HashObject *getAndRemove(const NextCash::Hash &pHash);
        HashObject *getAndRemoveAt(unsigned int pOffset);

        void clear();
        void clearNoDelete(); // Doesn't delete items.

        typedef std::vector<HashObject *>::iterator Iterator;
        typedef std::vector<HashObject *>::const_iterator ConstIterator;

        Iterator begin() { return mItems.begin(); }
        Iterator end() { return mItems.end(); }

        Iterator eraseDelete(Iterator &pIterator)
        {
            delete *pIterator;
            return mItems.erase(pIterator);
        }
        Iterator eraseNoDelete(Iterator &pIterator) { return mItems.erase(pIterator); }

        static bool test();

    private:

        std::vector<HashObject *> mItems;

        // If item is not found, give item after where it should be.
        static const uint8_t RETURN_ITEM_AFTER = 0x01;
        // Specified hash is after the last item of list.
        static const uint8_t BELONGS_AT_END = 0x02;
        // The specified item was found.
        static const uint8_t WAS_FOUND = 0x04;

        // Return the iterator for the item matching.
        // Updates flags based on results.
        Iterator binaryFind(const Hash &pHash, uint8_t &pFlags);

        HashSet(const HashSet &pCopy);
        HashSet &operator = (const HashSet &pRight);

    };
}

#endif
