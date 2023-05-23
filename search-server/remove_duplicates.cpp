#include <set>
#include <string>
#include <iostream>
#include <map>

#include "remove_duplicates.h"
#include "search_server.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    set<set<string>> document_word_sets;
    vector<int> marked_for_deletion;
    for (int id: search_server) {
        auto word_freqs_in_document = search_server.GetWordFrequencies(id);
        set<string> document_word_set;
        for (auto& [word, freq]: word_freqs_in_document) {
            document_word_set.insert(word);
        }
        if (document_word_sets.count(document_word_set) != 0) {
            marked_for_deletion.push_back(id);
            cout << "Found duplicate document id "s << id << endl;
        } else {
            document_word_sets.insert(document_word_set);
        }
    }
   for (int id: marked_for_deletion) {
            search_server.RemoveDocument(id);
   }
}