/*
 *
    NOT FINISHED YET
    NOT TESTED YET

    DO NOT USE THIS HEADER FILE UNTIL THIS MESSAGE REMOVED.


    Sample Code:
    symspell::SymSpell symSpell;
    symSpell.CreateDictionaryEntry("erhan", 1);
    symSpell.CreateDictionaryEntry("orhan", 2);
    symSpell.CreateDictionaryEntry("ayhan", 3);
    vector<symspell::SuggestItem*> items;
    symSpell.Lookup("ozhan", symspell::Verbosity::Top, items);
    std::cout << "Count : " << items.size() << std::endl;
    if (items.size() > 0)
    {
        auto itemsEnd = items.cend();

        for(auto it = items.cbegin(); it != itemsEnd; ++it)
            std::cout << "Item : '" << (*it)->term << "' Count : " << std::to_string((*it)->count) << " Distance : " << std::to_string((*it)->distance) << std::endl;
    }

    Todo List:
    1- Change std::string to char*
    2- Change levenshtein functions
    3- Improve performance
 *
 */


#ifndef SYMSPELL6_H
#define SYMSPELL6_H

#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <string>
#include <cstring>
#include <exception>
#include <limits>
#include <stdio.h>
#include <algorithm>
#include <queue>

using namespace std;

namespace symspell {
#define defaultMaxEditDistance 2
#define defaultPrefixLength 7
#define defaultCountThreshold 1
#define defaultInitialCapacity 16
#define defaultCompactLevel 5

#ifdef _MSC_VER
    typedef __int8 int8_t;
    typedef unsigned __int8 u_int8_t;

    typedef __int32 int32_t;
    typedef unsigned __int32 u_int32_t;

    typedef __int64 int64_t;
    typedef unsigned __int64 u_int64_t;
#endif

    /*
 * Copied from https://github.com/PierreBoyeau/levenshtein_distance
 * ########## BEGIN ##########
 */
    int mini(int a, int b, int c){
        return(min(a, min(b,c)));
    }

    int levenshtein_dist(string const& word1, string const& word2){
        ///
        ///  Please use lower-case strings
        /// word1 : first word
        /// word2 : second word
        /// getPath : bool. If True, sequence of operations to do to go from
        ///           word1 to word2
        ///
        int size1 = word1.size(), size2 = word2.size();
        int suppr_dist, insert_dist, subs_dist;
        int* dist = new int[(size1+1)*(size2+1)];

        for(int i=0; i<size1+1; ++i)
            dist[(size2+1)*i] = i;
        for(int j=0; j<size2+1; ++j)
            dist[j] = j;
        for(int i=1; i<size1+1; ++i){
            for(int j=1; j<size2+1; ++j){
                suppr_dist = dist[(size2+1)*(i-1)+j] + 1;
                insert_dist = dist[(size2+1)*i+j-1] + 1;
                subs_dist = dist[(size2+1)*(i-1)+j-1];
                if(word1[i-1]!=word2[j-1]){  // word indexes are implemented differently.
                    subs_dist += 1;
                }
                dist[(size2+1)*i+j] = mini(suppr_dist, insert_dist, subs_dist);
            }
        }

        // --------------------------------------------------------
        int res = dist[(size1+1)*(size2+1) - 1];
        delete dist;
        return(res);
    }

    int dl_dist(string const& word1, string const& word2){
        /// Damerau-Levenshtein distance
        ///  Please use lower-case strings
        /// word1 : first word
        /// word2 : second word
        ///
        int size1 = word1.size(), size2 = word2.size();
        int suppr_dist, insert_dist, subs_dist, val;
        int* dist = new int[(size1+1)*(size2+1)];

        for(int i=0; i<size1+1; ++i)
            dist[(size2+1)*i] = i;
        for(int j=0; j<size2+1; ++j)
            dist[j] = j;
        for(int i=1; i<size1+1; ++i){
            for(int j=1; j<size2+1; ++j){
                suppr_dist = dist[(size2+1)*(i-1)+j] + 1;
                insert_dist = dist[(size2+1)*i+j-1] + 1;
                subs_dist = dist[(size2+1)*(i-1)+j-1];
                if(word1[i-1]!=word2[j-1])  // word indexes are implemented differently.
                    subs_dist += 1;
                val = mini(suppr_dist, insert_dist, subs_dist);
                if(((i>=2) && (j>=2)) && ((word1[i-1]==word2[j-2]) && (word1[i-2]==word2[j-1])))
                    val = min(dist[(size2+1)*(i-2)+j-2]+1, val);
                dist[(size2+1)*i+j] = val;
            }
        }

        int res = dist[(size1+1)*(size2+1) - 1];
        delete dist;
        return(res);
    }

    /*
     * ########## END ##########
     */

    class SuggestionStage;
    template<typename T> class ChunkArray;
    enum class Verbosity
    {
        /// <summary>Top suggestion with the highest term frequency of the suggestions of smallest edit distance found.</summary>
        Top,
        /// <summary>All suggestions of smallest edit distance found, suggestions ordered by term frequency.</summary>
        Closest,
        /// <summary>All suggestions within maxEditDistance, suggestions ordered by edit distance
        /// , then by term frequency (slower, no early termination).</summary>
        All
    };

    class EditDistance {
        public:
            /// <summary>Wrapper for third party edit distance algorithms.</summary>

            /// <summary>Supported edit distance algorithms.</summary>
            enum class DistanceAlgorithm {
                /// <summary>Levenshtein algorithm.</summary>
                Levenshtein,
                /// <summary>Damerau optimal string alignment algorithm.</summary>
                DamerauOSA
            };

            EditDistance(DistanceAlgorithm algorithm) {
                this->algorithm = algorithm;
                switch (algorithm) {
                    case DistanceAlgorithm::DamerauOSA: this->distanceComparer = dl_dist; break;
                    case DistanceAlgorithm::Levenshtein: this->distanceComparer = levenshtein_dist; break;
                    default: throw std::invalid_argument("Unknown distance algorithm.");
                }
            }

            int Compare(string const& string1, string const& string2, int maxDistance) {
                return this->distanceComparer(string1, string2); // todo: max distance
            }

        private:
            DistanceAlgorithm algorithm;
            std::function<int(string const&, string const&)>distanceComparer;
    };

    class SuggestItem
    {
        public:
            /// <summary>The suggested correctly spelled word.</summary>
            string term;
            /// <summary>Edit distance between searched for word and suggestion.</summary>
            u_int8_t distance = 0;
            /// <summary>Frequency of suggestion in the dictionary (a measure of how common the word is).</summary>
            int64_t count = 0;

            SuggestItem() { }
            SuggestItem(string const& term, int32_t distance, int64_t count)
            {
                this->term = term;
                this->distance = distance;
                this->count = count;
            }

            int8_t CompareTo(SuggestItem const& other)
            {
                // order by distance ascending, then by frequency count descending
                if (this->distance == other.distance)
                {
                    if (other.count == this->count)
                        return 0;
                    else if (other.count > this->count)
                        return -1;
                    return 1;
                }

                if (other.distance == this->distance)
                    return 0;
                else if (other.distance > this->distance)
                    return -1;
                return 1;
            }

            bool operator == (const SuggestItem &ref) const
            {
                return this->term == ref.term;
            }

            std::size_t GetHashCode()
            {
                return std::hash<std::string>{}(term);
            }

            string ToString()
            {
                return "{" + term + ", " + std::to_string(distance) + ", " + std::to_string(count) + "}";
            }

            SuggestItem& ShallowCopy()
            {
                SuggestItem item;
                item.count = this->count;
                item.distance = this->distance;
                item.term = this->term;

                return item;
            }
    };

    template<typename T>
    class ChunkArray
    {
        public:
            vector<vector<T>> Values; //todo: use pointer array
            uint32_t Count;

            ChunkArray()
            { }

            void Reserve(size_t initialCapacity)
            {
                size_t chunks = (initialCapacity + ChunkSize - 1) / ChunkSize;
                Values.reserve(chunks);
                for (size_t i = 0; i < chunks; ++i)
                {
                    Values[i].reserve(ChunkSize);
                }
            }

            size_t Add(T & value)
            {
                if (Count == Capacity())
                {
                    vector<vector<T>> newValues;
                    newValues.reserve(Values.size() + 1);

                    // only need to copy the list of array blocks, not the data in the blocks
                    std::copy(Values.begin(), Values.end(), back_inserter(newValues));
                    newValues[Values.size()].reserve(ChunkSize);
                    Values = newValues;
                }
                Values[Row(Count)][Col(Count)] = value;
                Count++;
                return Count - 1;
            }

            void Clear()
            {
                Count = 0;
            }

            T& at(size_t index)
            {
                return Values[Row(index)][Col(index)];
            }

            void set(size_t index, T &value)
            {
                Values[Row(index)][Col(index)] = value;
            }

        private:
            const int32_t ChunkSize = 4096; //this must be a power of 2, otherwise can't optimize Row and Col functions
            const int32_t DivShift = 12; // number of bits to shift right to do division by ChunkSize (the bit position of ChunkSize)
            int Row(uint32_t index) { return index >> DivShift; } // same as index / ChunkSize
            int32_t Col(uint32_t index) { return index & (ChunkSize - 1); } //same as index % ChunkSize
            int32_t Capacity() { return Values.size() * ChunkSize; }
    };


    class SuggestionStage
    {
        public:
            class Node;
            class Entry;

            unordered_map<uint32_t, Entry*> Deletes;
            unordered_map<uint32_t, Entry*>::const_iterator DeletesEnd;

            ChunkArray<Node> Nodes;
            SuggestionStage(size_t initialCapacity)
            {
                Deletes.reserve(initialCapacity);
                Nodes.Reserve(initialCapacity * 2);
            }
            size_t DeleteCount() { return Deletes.size(); }
            size_t NodeCount() { return Nodes.Count; }
            void Clear()
            {
                Deletes.clear();
                Nodes.Clear();

                DeletesEnd = Deletes.cend();
            }

            void Add(int deleteHash, string suggestion)
            {
                auto deletesFinded = Deletes.find(deleteHash);
                Entry* entry = nullptr;
                if (deletesFinded == DeletesEnd) {
                    entry = new Entry;
                    entry->count;
                    entry->first = -1;
                }
                else
                    entry = deletesFinded->second;

                int next = entry->first;
                ++entry->count;
                entry->first = Nodes.Count;
                Deletes[deleteHash] = entry;
                Node item;
                item.next = next;
                item.suggestion = suggestion;
                Nodes.Add(item);
            }

            void CommitTo(unordered_map<int32_t, vector<string>> & permanentDeletes)
            {
                auto permanentDeletesEnd = permanentDeletes.cend();
                for(auto it = Deletes.cbegin(); it != DeletesEnd; ++it)
                {
                    auto permanentDeletesFinded = permanentDeletes.find(it->first);
                    vector<string>* suggestions = nullptr;
                    size_t i;
                    if (permanentDeletesFinded != permanentDeletesEnd)
                    {
                        suggestions = &permanentDeletesFinded->second;
                        i = suggestions->size();
                        vector<string> newSuggestions;
                        newSuggestions.reserve(suggestions->size() + it->second->count);

                        std::copy(suggestions->begin(), suggestions->end(), back_inserter(newSuggestions));
                        permanentDeletes[it->first] = newSuggestions;
                    }
                    else
                    {
                        i = 0;
                        suggestions = new vector<string>;
                        suggestions->reserve(it->second->count);
                        permanentDeletes[it->first] = *suggestions;
                    }

                    int next = it->second->first;
                    while (next >= 0)
                    {
                        auto node = Nodes.at(next);
                        (*suggestions)[i] = node.suggestion;
                        next = node.next;
                        ++i;
                    }
                }
            }

        public:
            class Node
            {
                public:
                    string suggestion;
                    int32_t next;
            };

            class Entry
            {
                public:
                    int32_t count;
                    int32_t first;
            };
    };


    class SymSpell {
        public:
            SymSpell(int32_t initialCapacity = defaultInitialCapacity, int32_t maxDictionaryEditDistance = defaultMaxEditDistance, int32_t prefixLength = defaultPrefixLength, int32_t countThreshold = defaultCountThreshold, int32_t compactLevel = defaultCompactLevel)
            {
                if (initialCapacity < 0) throw std::invalid_argument("initialCapacity");
                if (maxDictionaryEditDistance < 0) throw std::invalid_argument("maxDictionaryEditDistance");
                if (prefixLength < 1 || prefixLength <= maxDictionaryEditDistance) throw std::invalid_argument("prefixLength");
                if (countThreshold < 0) throw std::invalid_argument("countThreshold");
                if (compactLevel > 16) throw std::invalid_argument("compactLevel");

                this->initialCapacity = initialCapacity;
                this->words.reserve(initialCapacity);
                this->deletes.reserve(initialCapacity);
                this->maxDictionaryEditDistance = maxDictionaryEditDistance;
                this->prefixLength = prefixLength;
                this->countThreshold = countThreshold;
                if (compactLevel > 16) compactLevel = 16;
                this->compactMask = (std::numeric_limits<uint32_t>::max() >> (3 + compactLevel)) << 2;
                this->deletesEnd = this->deletes.cend();
                this->wordsEnd = this->words.cend();
                this->belowThresholdWordsEnd = this->belowThresholdWords.cend();
            }

            bool CreateDictionaryEntry(string const& key, int64_t count, SuggestionStage * staging = nullptr)
            {
                if (count <= 0)
                {
                    if (this->countThreshold > 0) return false; // no point doing anything if count is zero, as it can't change anything
                    count = 0;
                }

                int64_t countPrevious = -1;
                auto belowThresholdWordsFinded = belowThresholdWords.find(key);
                auto wordsFinded = words.find(key);

                // look first in below threshold words, update count, and allow promotion to correct spelling word if count reaches threshold
                // threshold must be >1 for there to be the possibility of low threshold words
                if (countThreshold > 1 && belowThresholdWordsFinded != belowThresholdWordsEnd)
                {
                    countPrevious = belowThresholdWordsFinded->second;
                    // calculate new count for below threshold word
                    count = (std::numeric_limits<int64_t>::max() - countPrevious > count) ? countPrevious + count : std::numeric_limits<int64_t>::max();
                    // has reached threshold - remove from below threshold collection (it will be added to correct words below)
                    if (count >= countThreshold)
                    {
                        belowThresholdWords.erase(key);
                        belowThresholdWordsEnd = belowThresholdWords.cend();
                    }
                    else
                    {
                        belowThresholdWords[key] = count;
                        belowThresholdWordsEnd = belowThresholdWords.cend();
                        return false;
                    }
                }
                else if (wordsFinded != wordsEnd)
                {
                    countPrevious = wordsFinded->second;
                    // just update count if it's an already added above threshold word
                    count = (std::numeric_limits<int64_t>::max() - countPrevious > count) ? countPrevious + count : std::numeric_limits<int64_t>::max();
                    words[key] = count;
                    return false;
                }
                else if (count < CountThreshold())
                {
                    // new or existing below threshold word
                    belowThresholdWords[key] = count;
                    belowThresholdWordsEnd = belowThresholdWords.cend();
                    return false;
                }

                // what we have at this point is a new, above threshold word
                words[key] = count;

                //edits/suggestions are created only once, no matter how often word occurs
                //edits/suggestions are created only as soon as the word occurs in the corpus,
                //even if the same term existed before in the dictionary as an edit from another word
                if (key.size() > maxDictionaryWordLength)
                    maxDictionaryWordLength = key.size();

                //create deletes
                unordered_set<string> edits;
                EditsPrefix(key, edits);
                // if not staging suggestions, put directly into main data structure
                if (staging != nullptr)
                {
                    auto editsEnd = edits.cend();

                    for(auto it = edits.cbegin(); it != editsEnd; ++it)
                    {
                        staging->Add(this->GetStringHash(*it), key);
                    }
                }
                else
                {
                    auto editsEnd = edits.cend();
                    for(auto it = edits.cbegin(); it != editsEnd; ++it)
                    {
                        int deleteHash = this->GetStringHash(*it);
                        auto deletesFinded = deletes.find(deleteHash);
                        vector<string>* suggestions;
                        if (deletesFinded != deletesEnd)
                        {
                            suggestions = &deletesFinded->second;
                            vector<string> newSuggestions;
                            newSuggestions.reserve(suggestions->size() + 1);
                            copy(suggestions->begin(), suggestions->end(), back_inserter(newSuggestions));
                            deletes[deleteHash] = *suggestions = newSuggestions;
                            deletesEnd = deletes.cend();
                        }
                        else
                        {
                            suggestions = new vector<string>;
                            suggestions->resize(1);
                            deletes[deleteHash] = *suggestions;
                        }
                        (*suggestions)[suggestions->size() - 1] = key;
                    }
                }
                return true;
            }

            void EditsPrefix(string key, unordered_set<string>& hashSet)
            {
                if (key.size() <= maxDictionaryEditDistance) hashSet.insert("");
                if (key.size() > prefixLength) key = key.substr(0, prefixLength);
                hashSet.insert(key);
                Edits(key, 0, hashSet);
            }

            void Edits(string const& searchWord, int32_t editDistance, unordered_set<string> & deleteWords)
            {
                unordered_set<string>::const_iterator deleteWordsEnd = deleteWords.cend();
                queue<string> wordQueue;
                wordQueue.push(searchWord);

                while (!wordQueue.empty())
                {
                    auto word = wordQueue.front();
                    wordQueue.pop();
                    size_t wordLen = word.size();

                    ++editDistance;
                    if (wordLen > 1)
                    {
                        for (size_t i = 0; i < wordLen; ++i)
                        {
                            char* tmp = new char[wordLen];
                            std::memcpy(tmp, word.c_str(), i);
                            std::memcpy(tmp + i, word.c_str() + i + 1, wordLen - 1 - i);
                            tmp[wordLen - 1] = '\0';

                            if (deleteWords.find(tmp) == deleteWordsEnd)
                            {
                                deleteWords.insert(tmp);
                                deleteWordsEnd = deleteWords.cend();
                                //recursion, if maximum edit distance not yet reached
                                if (editDistance < maxDictionaryEditDistance)
                                {
                                    //Edits(tmp, editDistance, deleteWords);
                                    wordQueue.push(tmp);
                                }

                                //std::cout << tmp << std::endl;
                            }

                            delete[] tmp;
                        }
                    }
                }
            }

            int GetStringHash(string const& s)
            {
                int len = s.size();
                int lenMask = len;
                if (lenMask > 3) lenMask = 3;

                u_int32_t hash = 2166136261;
                for (size_t i = 0; i < len; ++i)
                {
                    hash ^= s[i];
                    hash *= 16777619;
                }

                hash &= this->compactMask;
                hash |= (u_int32_t)lenMask;
                return (int32_t)hash;
            }

            void PurgeBelowThresholdWords()
            {
                belowThresholdWords.clear();
                belowThresholdWordsEnd = belowThresholdWords.cend();
            }

            void CommitStaged(SuggestionStage staging)
            {
                staging.CommitTo(deletes);
            }

            void Lookup(string const& input, Verbosity verbosity, vector<SuggestItem*> & items)
            {
                this->Lookup(input, verbosity, this->maxDictionaryEditDistance, false, items);
            }

            void Lookup(string const& input, Verbosity verbosity, int32_t maxEditDistance, vector<SuggestItem*> & items)
            {
                this->Lookup(input, verbosity, maxEditDistance, false, items);
            }

            void Lookup(string const& input, Verbosity verbosity, int32_t maxEditDistance, bool includeUnknown, vector<SuggestItem*> & suggestions)
            {
                //verbosity=Top: the suggestion with the highest term frequency of the suggestions of smallest edit distance found
                //verbosity=Closest: all suggestions of smallest edit distance found, the suggestions are ordered by term frequency
                //verbosity=All: all suggestions <= maxEditDistance, the suggestions are ordered by edit distance, then by term frequency (slower, no early termination)

                // maxEditDistance used in Lookup can't be bigger than the maxDictionaryEditDistance
                // used to construct the underlying dictionary structure.
                if (maxEditDistance > MaxDictionaryEditDistance())  throw std::invalid_argument("maxEditDistance");
                long suggestionCount = 0;
                size_t suggestionsLen = suggestions.size();
                auto wordsFinded = words.find(input);
                int inputLen = input.size();
                // early exit - word is too big to possibly match any words
                if (inputLen - maxEditDistance > maxDictionaryWordLength)
                {
                    if (includeUnknown && (suggestionsLen == 0))
                        suggestions.push_back(new SuggestItem(input, maxEditDistance + 1, 0));

                    return;
                }

                // quick look for exact match

                if (wordsFinded != wordsEnd)
                {
                    suggestionCount = wordsFinded->second;
                    suggestions.push_back(new SuggestItem(input, 0, suggestionCount));
                    suggestionsLen = suggestions.size();
                    // early exit - return exact match, unless caller wants all matches
                    if (verbosity != Verbosity::All)
                    {
                        if (includeUnknown && (suggestionsLen == 0))
                        {
                            suggestions.push_back(new SuggestItem(input, maxEditDistance + 1, 0));
                            suggestionsLen = suggestions.size();
                        }

                        return;
                    }
                }

                //early termination, if we only want to check if word in dictionary or get its frequency e.g. for word segmentation
                if (maxEditDistance == 0)
                {
                    if (includeUnknown && (suggestionsLen == 0))
                    {
                        suggestions.push_back(new SuggestItem(input, maxEditDistance + 1, 0));
                        suggestionsLen = suggestions.size();
                    }

                    return;
                }

                // deletes we've considered already
                unordered_set<string> hashset1;
                // suggestions we've considered already
                unordered_set<string> hashset2;
                // we considered the input already in the word.TryGetValue above
                hashset2.insert(input);

                int maxEditDistance2 = maxEditDistance;
                int candidatePointer = 0;
                vector<string> candidates;
                candidates.reserve(32);

                //add original prefix
                int inputPrefixLen = inputLen;
                if (inputPrefixLen > prefixLength)
                {
                    inputPrefixLen = prefixLength;
                    candidates.push_back(input.substr(0, inputPrefixLen));
                }
                else
                {
                    candidates.push_back(input);
                }
                
                size_t candidatesLen = candidates.size();
                EditDistance* distanceComparer = new EditDistance(this->distanceAlgorithm);
                while (candidatePointer < candidatesLen)
                {
                    string candidate = candidates[candidatePointer++];
                    int candidateLen = candidate.size();
                    int lengthDiff = inputPrefixLen - candidateLen;

                    //save some time - early termination
                    //if canddate distance is already higher than suggestion distance, than there are no better suggestions to be expected
                    if (lengthDiff > maxEditDistance2)
                    {
                        // skip to next candidate if Verbosity.All, look no further if Verbosity.Top or Closest
                        // (candidates are ordered by delete distance, so none are closer than current)
                        if (verbosity == Verbosity::All) continue;
                        break;
                    }

                    auto deletesFinded = deletes.find(GetStringHash(candidate));
                    vector<string>* dictSuggestions = nullptr;

                    //read candidate entry from dictionary
                    if (deletesFinded != deletesEnd)
                    {
                        dictSuggestions = &deletesFinded->second;
                        size_t dictSuggestionsLen = dictSuggestions->size();
                        //iterate through suggestions (to other correct dictionary items) of delete item and add them to suggestion list
                        for (int i = 0; i < dictSuggestionsLen; ++i)
                        {
                            string suggestion = dictSuggestions->at(i);
                            int suggestionLen = suggestion.size();
                            if (suggestion == input) continue;
                            if ((abs(suggestionLen - inputLen) > maxEditDistance2) // input and sugg lengths diff > allowed/current best distance
                                    || (suggestionLen < candidateLen) // sugg must be for a different delete string, in same bin only because of hash collision
                                    || (suggestionLen == candidateLen && suggestion != candidate)) // if sugg len = delete len, then it either equals delete or is in same bin only because of hash collision
                                continue;
                            auto suggPrefixLen = min(suggestionLen, prefixLength);
                            if (suggPrefixLen > inputPrefixLen && (suggPrefixLen - candidateLen) > maxEditDistance2) continue;

                            //True Damerau-Levenshtein Edit Distance: adjust distance, if both distances>0
                            //We allow simultaneous edits (deletes) of maxEditDistance on on both the dictionary and the input term.
                            //For replaces and adjacent transposes the resulting edit distance stays <= maxEditDistance.
                            //For inserts and deletes the resulting edit distance might exceed maxEditDistance.
                            //To prevent suggestions of a higher edit distance, we need to calculate the resulting edit distance, if there are simultaneous edits on both sides.
                            //Example: (bank==bnak and bank==bink, but bank!=kanb and bank!=xban and bank!=baxn for maxEditDistance=1)
                            //Two deletes on each side of a pair makes them all equal, but the first two pairs have edit distance=1, the others edit distance=2.
                            int distance = 0;
                            int _min = 0;
                            if (candidateLen == 0)
                            {
                                //suggestions which have no common chars with input (inputLen<=maxEditDistance && suggestionLen<=maxEditDistance)
                                distance = max(inputLen, suggestionLen);
                                if (distance > maxEditDistance2 || !hashset2.insert(suggestion).second) continue;
                            }
                            else if (suggestionLen == 1)
                            {
                                if (input.find(suggestion[0]) < 0) distance = inputLen; else distance = inputLen - 1;
                                if (distance > maxEditDistance2 || !hashset2.insert(suggestion).second) continue;
                            }
                            else
                                //number of edits in prefix ==maxediddistance  AND no identic suffix
                                //, then editdistance>maxEditDistance and no need for Levenshtein calculation
                                //      (inputLen >= prefixLength) && (suggestionLen >= prefixLength)
                                if ((prefixLength - maxEditDistance == candidateLen)
                                        && (((_min = std::min(inputLen, suggestionLen) - prefixLength) > 1)
                                            && (input.substr(inputLen + 1 - _min) != suggestion.substr(suggestionLen + 1 - _min)))
                                        || ((_min > 0) && (input[inputLen - _min] != suggestion[suggestionLen - _min])
                                            && ((input[inputLen - _min - 1] != suggestion[suggestionLen - _min])
                                                || (input[inputLen - _min] != suggestion[suggestionLen - _min - 1]))))
                                {
                                    continue;
                                }
                                else
                                {
                                    // DeleteInSuggestionPrefix is somewhat expensive, and only pays off when verbosity is Top or Closest.
                                    if ((verbosity != Verbosity::All && !DeleteInSuggestionPrefix(candidate, candidateLen, suggestion, suggestionLen))
                                            || !hashset2.insert(suggestion).second) continue;
                                    distance = distanceComparer->Compare(input, suggestion, maxEditDistance2);
                                    if (distance < 0) continue;
                                }

                            //save some time
                            //do not process higher distances than those already found, if verbosity<All (note: maxEditDistance2 will always equal maxEditDistance when Verbosity.All)
                            if (distance <= maxEditDistance2)
                            {
                                suggestionCount = words[suggestion];
                                SuggestItem* si = new SuggestItem(suggestion, distance, suggestionCount);
                                if (suggestionsLen > 0)
                                {
                                    switch (verbosity)
                                    {
                                        case Verbosity::Closest:
                                        {
                                            //we will calculate DamLev distance only to the smallest found distance so far
                                            if (distance < maxEditDistance2)
                                            {
                                                suggestions.clear();
                                                suggestionsLen = suggestions.size();
                                            }
                                            break;
                                        }
                                        case Verbosity::Top:
                                        {
                                            if (distance < maxEditDistance2 || suggestionCount > suggestions[0]->count)
                                            {
                                                maxEditDistance2 = distance;
                                                suggestions[0] = si;
                                            }
                                            continue;
                                        }
                                        case Verbosity::All:
                                        {
                                            break;
                                        }
                                    }
                                }
                                if (verbosity != Verbosity::All) maxEditDistance2 = distance;
                                suggestions.push_back(si);
                                suggestionsLen = suggestions.size();
                            }
                        }//end foreach
                    }//end if

                    //add edits
                    //derive edits (deletes) from candidate (input) and add them to candidates list
                    //this is a recursive process until the maximum edit distance has been reached
                    if ((lengthDiff < maxEditDistance) && (candidateLen <= prefixLength))
                    {
                        //save some time
                        //do not create edits with edit distance smaller than suggestions already found
                        if (verbosity != Verbosity::All && lengthDiff >= maxEditDistance2) continue;

                        for (int i = 0; i < candidateLen; i++)
                        {
                            char* tmp = new char[candidateLen];
                            std::memcpy(tmp, candidate.c_str(), i);
                            std::memcpy(tmp + i, candidate.c_str() + i + 1, candidateLen - 1 - i);
                            tmp[candidateLen - 1] = '\0';

                            if (hashset1.insert(tmp).second)
                            {
                                candidates.push_back(tmp);
                                candidatesLen = candidates.size();
                            }
                            else
                                delete[] tmp;
                        }
                    }
                }//end while

                //sort by ascending edit distance, then by descending word frequency
                if (suggestionsLen > 1)
                    std::sort (suggestions.begin(), suggestions.end());
                
                
                //cleaning
                delete distanceComparer;

            }//end if

            /// <summary>Maximum edit distance for dictionary precalculation.</summary>
            size_t MaxDictionaryEditDistance() { return this->maxDictionaryEditDistance; }

            /// <summary>Length of prefix, from which deletes are generated.</summary>
            size_t PrefixLength() { return this->prefixLength; }

            /// <summary>Length of longest word in the dictionary.</summary>
            size_t MaxLength() { return this->maxDictionaryWordLength; }

            /// <summary>Count threshold for a word to be considered a valid word for spelling correction.</summary>
            int64_t CountThreshold()  { return this->countThreshold; }

            /// <summary>Number of unique words in the dictionary.</summary>
            size_t WordCount()  { return this->words.size(); }

            /// <summary>Number of word prefixes and intermediate word deletes encoded in the dictionary.</summary>
            size_t EntryCount() { return this->deletes.size(); }

        private:
            int32_t initialCapacity;
            int32_t maxDictionaryEditDistance;
            int32_t prefixLength; //prefix length  5..7
            int64_t countThreshold; //a treshold might be specifid, when a term occurs so frequently in the corpus that it is considered a valid word for spelling correction
            uint32_t compactMask;
            EditDistance::DistanceAlgorithm distanceAlgorithm = EditDistance::DistanceAlgorithm::DamerauOSA;
            size_t maxDictionaryWordLength; //maximum dictionary term length

            // Dictionary that contains a mapping of lists of suggested correction words to the hashCodes
            // of the original words and the deletes derived from them. Collisions of hashCodes is tolerated,
            // because suggestions are ultimately verified via an edit distance function.
            // A list of suggestions might have a single suggestion, or multiple suggestions.
            unordered_map<int32_t, vector<string>> deletes;
            unordered_map<int32_t, vector<string>>::const_iterator deletesEnd;

            // Dictionary of unique correct spelling words, and the frequency count for each word.
            unordered_map<string, int64_t> words;
            unordered_map<string, int64_t>::const_iterator wordsEnd;

            // Dictionary of unique words that are below the count threshold for being considered correct spellings.
            unordered_map<string, int64_t> belowThresholdWords;
            unordered_map<string, int64_t>::const_iterator belowThresholdWordsEnd;

            bool DeleteInSuggestionPrefix(string const& del, int deleteLen, string const& suggestion, int suggestionLen)
            {
                if (deleteLen == 0) return true;
                if (prefixLength < suggestionLen) suggestionLen = prefixLength;
                int j = 0;
                for (int i = 0; i < deleteLen; i++)
                {
                    char delChar = del[i];
                    while (j < suggestionLen && delChar != suggestion[j]) j++;
                    if (j == suggestionLen) return false;
                }
                return true;
            }
    };
}

#endif // SYMSPELL6_H
