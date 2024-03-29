#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <string>
#include <execution>
#include <deque>

#include "document.h"
#include "string_processing.h"
#include "log_duration.h"
#include "concurrent_map.h"


const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(std::string_view stop_words);
    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, std::string_view document, DocumentStatus status,
        const std::vector<int>& ratings);
    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy policy, int document_id);
    void RemoveDocument(std::execution::parallel_policy policy, int document_id);
    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query,
        DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query,
        DocumentStatus status) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query,
        DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    int GetDocumentCount() const;
    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        std::execution::parallel_policy policy, std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        std::execution::sequenced_policy policy, std::string_view raw_query, int document_id) const;
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    
    std::deque<std::string> doc_storage_;
    const std::set<std::string, std::less<>> stop_words_;
    std::map<int , std::map<std::string_view, double>> document_to_word_freqs_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
        
        void EraseDuplicates() {
            std::sort(plus_words.begin(), plus_words.end());
            auto last_p = std::unique(plus_words.begin(), plus_words.end());
            plus_words.erase(last_p, plus_words.end());
            
            std::sort(minus_words.begin(), minus_words.end());
            auto last_m = std::unique(minus_words.begin(), minus_words.end());
            minus_words.erase(last_m, minus_words.end());
        }
    };
    
    bool IsStopWord(std::string_view word) const;
    static bool IsValidWord(std::string_view word);
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string& text) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);
    QueryWord ParseQueryWord(std::string_view text) const;
    Query ParseQueryNoDuplicates(std::string_view text) const;
    Query ParseQueryBasic(std::string_view text) const;
    double ComputeWordInverseDocumentFreq(std::string_view word) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(
        std::execution::sequenced_policy policy, 
        const Query& query,
        DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(
        std::execution::parallel_policy policy, 
        const Query& query,
        DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
    throw std::invalid_argument("Some of stop words are invalid");
    }
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(
    ExecutionPolicy policy, 
    std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    using namespace std;
    const auto query = ParseQueryNoDuplicates(raw_query);
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);
    sort(matched_documents.begin(), matched_documents.end(),
         [](const Document& lhs, const Document& rhs) {
             if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
                 return lhs.rating > rhs.rating;
             } else {
                 return lhs.relevance > rhs.relevance;
             }
         });
    
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

 template <typename ExecutionPolicy>
 std::vector<Document> SearchServer::FindTopDocuments(
    ExecutionPolicy policy, 
    std::string_view raw_query, 
    DocumentStatus status) const{
    return FindTopDocuments(policy, raw_query, 
                            [status](int document_id, DocumentStatus document_status, int rating) {
                                return document_status == status;
                            });
 }

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(
    ExecutionPolicy policy, std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
    }
    
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    std::execution::sequenced_policy policy, 
    const SearchServer::Query& query, 
    DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (auto word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    
    for (auto word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    std::execution::parallel_policy policy, 
    const SearchServer::Query& query,
    DocumentPredicate document_predicate) const {
    static constexpr int BUCKET_COUNT = 1000;
    ConcurrentMap<int, double> par_document_to_relevance(BUCKET_COUNT);
    for_each(policy,
             query.plus_words.begin(),
             query.plus_words.end(), 
             [&](std::string_view word) {
                 if (word_to_document_freqs_.count(word) != 0) {
                     const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                     for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                         const auto& document_data = documents_.at(document_id);
                         if (document_predicate(document_id, document_data.status, document_data.rating)) {
                             par_document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                         }
                     }
                 }
             });

    for_each(policy,
             query.minus_words.begin(),
             query.minus_words.end(),
             [&](std::string_view word) {
                 if (word_to_document_freqs_.count(word) != 0) {
                     for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                         par_document_to_relevance.Erase(document_id);
                     }
                 }
             });

    std::map<int, double> document_to_relevance = par_document_to_relevance.BuildOrdinaryMap();
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}