#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <iterator>
#include <execution>
#include <iostream>
#include <string_view>

#include "search_server.h"
#include "log_duration.h"
#include "process_queries.h"

using namespace std;


SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text)) {
}

SearchServer::SearchServer(string_view stop_words)
    : SearchServer(SplitIntoWords(string{stop_words})) {
}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status,
                               const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    
    doc_storage_.emplace_back(string{document});
    const auto words = SplitIntoWordsNoStop(doc_storage_.back());

    const double inv_word_count = 1.0 / words.size();
    for (auto word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    for (auto& [word, freq]: document_to_word_freqs_[document_id]) {
            word_to_document_freqs_.at(word).erase(document_id);
    }
    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(
    execution::sequenced_policy policy, 
    int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(execution::parallel_policy policy, int document_id) {
    vector<string_view> words;
    words.reserve(document_to_word_freqs_[document_id].size());
    transform(
        document_to_word_freqs_[document_id].begin(),
        document_to_word_freqs_[document_id].end(),
        back_inserter(words),
        [](const auto& entry) {
            return entry.first;
        });
    for_each(
        policy,
        words.begin(),
        words.end(),
        [document_id, this](auto str) {
                word_to_document_freqs_.at(str).erase(document_id);
        });
    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(execution::seq, raw_query, status);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
    return FindTopDocuments(execution::seq, raw_query, DocumentStatus::ACTUAL);
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static map<string_view, double> empty;
    if (document_to_word_freqs_.count(document_id) == 0) {
        return empty;
    } else {
        return document_to_word_freqs_.at(document_id);
    }
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query,
                                                                       int document_id) const {                          
    auto query = ParseQueryNoDuplicates(raw_query);
    
    if (any_of(query.minus_words.begin(),
               query.minus_words.end(),
               [this, document_id](const auto& word) {
                   return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
               })) {
        return {{}, documents_.at(document_id).status};
    }
    
    vector<string_view> matched_words;
    for (auto word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    
return {matched_words, documents_.at(document_id).status};
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(
    execution::sequenced_policy policy, 
    string_view raw_query,
    int document_id) const {
    return MatchDocument(raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(
    execution::parallel_policy policy, 
    string_view raw_query,
    int document_id) const {
    if ((document_id < 0) || (documents_.count(document_id) == 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    auto query = ParseQueryBasic(raw_query);
    
    if (any_of(query.minus_words.begin(),
               query.minus_words.end(),
               [this, document_id](auto word) {
                   return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
               })) {
        return {{}, documents_.at(document_id).status};
    }
    
    vector<string_view> matched_words(query.plus_words.size());
    auto last = copy_if(
        policy,
        query.plus_words.begin(),
        query.plus_words.end(),
        matched_words.begin(),
        [&matched_words, this, document_id](auto word) {
            return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
        });
    
    sort(matched_words.begin(), last);
    last = unique(matched_words.begin(), last);
    matched_words.erase(last, matched_words.end());
    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(string_view word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string_view> words;
    for (auto word : SplitIntoWordsView(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string{word} + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view word) const {
    if (word.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
   
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQueryNoDuplicates(string_view query_text) const{
    auto query = ParseQueryBasic(query_text);
    query.EraseDuplicates();
    return query;
}

SearchServer::Query SearchServer::ParseQueryBasic(string_view query_text) const{
    Query query;
    for (auto word : SplitIntoWordsView(query_text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            } else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    return query;
}

    double SearchServer::ComputeWordInverseDocumentFreq(string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
