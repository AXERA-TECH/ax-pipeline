// Copyright (C) 2013 Paul Ilardi (http://github.com/CodePi)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, unconditionally.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
#pragma once
#include <map>
#include <stdexcept>
#include <vector>
#include <memory>

namespace codepi
{

    // MultikeyMap: Similar to std::multimap, but with two keys

    template <typename Key1, typename Key2, typename ValType>
    class MultikeyMap
    {
    public:
        // basic map entry
        struct Entry
        {
            Key1 key1;
            Key2 key2;
            ValType val;
        };

        // Constructor
        MultikeyMap(std::initializer_list<Entry> list)
        {
            for (auto &e : list)
            {
                insert(e);
            }
        }

        // Typedefs used later
        typedef std::shared_ptr<Entry> EntryPtr;
        typedef typename std::multimap<Key1, EntryPtr>::iterator iterator1;
        typedef typename std::multimap<Key2, EntryPtr>::iterator iterator2;
        typedef typename std::multimap<std::pair<Key1, Key2>, EntryPtr>::iterator iterator;

        // Insert value into map with two keys
        void insert(const Key1 &key1, const Key2 &key2, const ValType &val)
        {
            insert(EntryPtr(new Entry{key1, key2, val}));
        }

        // Insert value into map with two keys
        void insert(const Key1 &key1, const Key2 &key2, ValType &&val)
        {
            insert(EntryPtr(new Entry{key1, key2, std::move(val)}));
        }

        // Insert entry into map
        void insert(Entry &&entry)
        {
            insert(EntryPtr(new Entry(std::move(entry))));
        }

        // Insert entry into map
        void insert(const Entry &entry)
        {
            insert(EntryPtr(new Entry(entry)));
        }

        void insert(EntryPtr ep)
        {
            map1.insert(std::make_pair(ep->key1, ep));
            map2.insert(std::make_pair(ep->key2, ep));
            mapAll.insert(std::make_pair(std::make_pair(ep->key1, ep->key2), ep));
        }

        // gets a list of all entries matching key1
        std::vector<EntryPtr> get1(const Key1 &key1)
        {
            std::vector<EntryPtr> vec;
            iterator1 i;
            for (i = map1.lower_bound(key1); i != map1.upper_bound(key1); i++)
            {
                vec.push_back(i->second);
            }
            return vec;
        }

        // gets a list of all entries matching key2
        std::vector<EntryPtr> get2(const Key2 &key2)
        {
            std::vector<EntryPtr> vec;
            iterator2 i;
            for (i = map2.lower_bound(key2); i != map2.upper_bound(key2); i++)
            {
                vec.push_back(i->second);
            }
            return vec;
        }

        // gets a list of all entries matching key1 and key2
        std::vector<EntryPtr> get(const Key1 &key1, const Key1 &key2)
        {
            std::vector<EntryPtr> vec;
            iterator i;
            auto keys = std::make_pair(key1, key2);
            for (i = mapAll.lower_bound(keys); i != mapAll.upper_bound(keys); i++)
            {
                vec.push_back(i->second);
            }
            return vec;
        }

        // erases entry matching EntryPtr
        void erase_entry(EntryPtr e)
        {
            // remove entry from map1
            iterator1 i1 = map1.lower_bound(e->key1);
            while (i1 != map1.upper_bound(e->key1))
            {
                if (i1->second == e)
                    i1 = map1.erase(i1);
                else
                    i1++;
            }

            // remove entry from map2
            iterator2 i2 = map2.lower_bound(e->key2);
            while (i2 != map2.upper_bound(e->key2))
            {
                if (i2->second == e)
                    i2 = map2.erase(i2);
                else
                    i2++;
            }

            // remove entry from mapAll
            auto keys = std::make_pair(e->key1, e->key2);
            iterator i3 = mapAll.lower_bound(keys);
            while (i3 != mapAll.upper_bound(keys))
            {
                if (i3->second == e)
                    i3 = mapAll.erase(i3);
                else
                    i3++;
            }
        }

        // erases each entry in list
        void erase_entry_list(const std::vector<EntryPtr> &list)
        {
            for (size_t i = 0; i < list.size(); i++)
                erase_entry(list[i]);
        }

        // erase of all entries matching key
        void erase1(const Key1 &key1) { erase_entry_list(get1(key1)); }
        void erase2(const Key2 &key2) { erase_entry_list(get2(key2)); }
        void erase(const Key1 &key1, const Key2 &key2) { erase_entry_list(get(key1, key2)); }

        // return number of entries matching key
        int count1(const Key1 &key1) { return map1.count(key1); }
        int count2(const Key2 &key2) { return map2.count(key2); }
        int count(const Key1 &key1, const Key2 &key2)
        {
            return mapAll.count(std::make_pair(key1, key2));
        }

        // Implementation of standard map methods
        size_t size() { return mapAll.size(); }
        bool empty() { return size() == 0; }
        iterator begin() { return mapAll.begin(); }
        iterator end() { return mapAll.end(); }
        iterator1 begin1() { return map1.begin(); }
        iterator1 end1() { return map1.end(); }
        iterator2 begin2() { return map2.begin(); }
        iterator2 end2() { return map2.end(); }
        void clear()
        {
            map1.clear();
            map2.clear();
            mapAll.clear();
        }

        bool contain(Key1 key)
        {
            auto item = map1.find(key);

            if (item != map1.end())
            {
                return true;
            }
            return false;
        }

        bool contain(Key2 key)
        {
            auto item = map2.find(key);

            if (item != map2.end())
            {
                return true;
            }
            return false;
        }

    private:
        // Maps key1 to keypair
        std::multimap<Key1, EntryPtr> map1;
        // Maps key2 to keypair
        std::multimap<Key2, EntryPtr> map2;
        // Maps key2 to keypair
        std::multimap<std::pair<Key1, Key2>, EntryPtr> mapAll;
    };

} // end namespace codepi